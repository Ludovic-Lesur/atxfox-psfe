/*
 * trcs.c
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludovic
 */

#include "trcs.h"

#include "adc.h"
#include "exti.h"
#include "gpio.h"
#include "mapping.h"
#include "math.h"
#include "mode.h"
#include "nvic.h"
#include "psfe.h"

/*** TRCS local macros ***/

#define TRCS_ADC_SAMPLE_BUFFER_LENGTH		10

#define TRCS_RANGE_RECOVERY_DELAY_MS		100		// Recovery time when switching ranges.
#define TRCS_RANGE_STABILIZATION_DELAY_MS	100		// Delay applied after switching operation.

#define TRCS_RANGE_UP_THRESHOLD_12BITS		3900	// Range will be increased if ADC result is higher.
#define TRCS_RANGE_DOWN_THRESHOLD_12BITS	30		// Range will be decreased if ADC result is lower.

#define TRCS_LT6105_VOLTAGE_GAIN			59		// LT6105 with 100R and 5.9k resistors.

static const unsigned int trcs_low_range_resistor_mohms[PSFE_NUMBER_OF_BOARDS] = {52000, 52000, 53000, 51000, 53000, 53000, 50000, 53000, 52000, 50000};
static const unsigned int trcs_middle_range_resistor_mohms[PSFE_NUMBER_OF_BOARDS] = {510, 510, 510, 510, 530, 520, 500, 540, 520, 500};
static const unsigned int trcs_high_range_resistor_mohms[PSFE_NUMBER_OF_BOARDS] = 	{6, 5, 6, 6, 6, 6, 5, 7, 6, 5};

/*** TRCS local structures ***/

typedef enum {
	TRCS_RANGE_INDEX_LOW = 0,
	TRCS_RANGE_INDEX_MIDDLE,
	TRCS_RANGE_INDEX_HIGH,
	TRCS_RANGE_INDEX_LAST
} TRCS_RangeIndex;

typedef struct {
	const TRCS_Range range;
	const GPIO* gpio;
	const unsigned int resistor_mohm;
	unsigned char switch_request_pending;
	unsigned int switch_timer_ms;
} TRCS_RangeInfo;

typedef struct {
	unsigned char trcs_current_range_idx;
	unsigned char trcs_previous_range_idx;
	unsigned char trcs_switch_pending;
	unsigned int trcs_iout_12bits_buf[TRCS_ADC_SAMPLE_BUFFER_LENGTH];
	unsigned char trcs_iout_12bits_buf_idx;
	unsigned int trcs_iout_12bits;
	unsigned int trcs_iout_ua;
	unsigned char trcs_bypass_flag;
} TRCS_Context;

/*** TRCS local global variables ***/

static volatile TRCS_Context trcs_ctx;
static TRCS_RangeInfo trcs_range_table[TRCS_RANGE_INDEX_LAST] = {{TRCS_RANGE_LOW, &GPIO_TRCS_RANGE_LOW, trcs_low_range_resistor_mohms[PSFE_BOARD_INDEX], 0, 0},
															     {TRCS_RANGE_MIDDLE, &GPIO_TRCS_RANGE_MIDDLE, trcs_middle_range_resistor_mohms[PSFE_BOARD_INDEX], 0, 0},
															     {TRCS_RANGE_HIGH, &GPIO_TRCS_RANGE_HIGH, trcs_high_range_resistor_mohms[PSFE_BOARD_INDEX], 0, 0}};

/*** TRCS local functions ***/

/* UPDATE RAW ADC RESULT.
 * @param:	None.
 * @return:	None.
 */
static void TRCS_UpdateAdcResult(void) {
	// Add sample
	ADC1_GetData(ADC_DATA_IDX_IOUT_12BITS, &trcs_ctx.trcs_iout_12bits_buf[trcs_ctx.trcs_iout_12bits_buf_idx]);
	// Update average.
	trcs_ctx.trcs_iout_12bits = MATH_ComputeAverage((unsigned int*) trcs_ctx.trcs_iout_12bits_buf, TRCS_ADC_SAMPLE_BUFFER_LENGTH);
	// Manage index.
	trcs_ctx.trcs_iout_12bits_buf_idx++;
	if (trcs_ctx.trcs_iout_12bits_buf_idx >= TRCS_ADC_SAMPLE_BUFFER_LENGTH) {
		trcs_ctx.trcs_iout_12bits_buf_idx = 0;
	}
}

/* COMPUTE OUTPUT CURRENT IN MICRO-AMPS.
 * @param:	None.
 * @return:	None.
 */
static void TRCS_ComputeIout(void) {
	// Local variables.
	unsigned int ref191_12bits = 0;
	unsigned int resistor_mohm = 0;
	unsigned long long num = 0;
	unsigned long long den = 0;
	// Get bandgap measurement.
	ADC1_GetData(ADC_DATA_IDX_REF191_12BITS, &ref191_12bits);
	// Convert to uA.
	num = (unsigned long long) trcs_ctx.trcs_iout_12bits;
	num *= ADC_REF191_VOLTAGE_MV;
	num *= 1000000;
	resistor_mohm = trcs_range_table[trcs_ctx.trcs_current_range_idx].resistor_mohm;
	den = (unsigned long long) ref191_12bits;
	den *= TRCS_LT6105_VOLTAGE_GAIN;
	den *= resistor_mohm;
	trcs_ctx.trcs_iout_ua = (num / den);
}

/*** TRCS functions ***/

/* INIT ALL GPIOs CONTROLLING RANGE RELAYS.
 * @param:	None.
 * @return:	None.
 */
void TRCS_Init(void) {
	// Init GPIOs.
	GPIO_Configure(&GPIO_TRCS_RANGE_LOW, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TRCS_RANGE_MIDDLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TRCS_RANGE_HIGH, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_TRCS_BYPASS, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Set board in high range by default to ensure power continuity.
	GPIO_Write(&GPIO_TRCS_RANGE_HIGH, 1);
	GPIO_Write(&GPIO_TRCS_RANGE_MIDDLE, 0);
	GPIO_Write(&GPIO_TRCS_RANGE_LOW, 0);
	// Init context.
	unsigned char idx = 0;
	trcs_ctx.trcs_current_range_idx = TRCS_RANGE_INDEX_HIGH;
	trcs_ctx.trcs_previous_range_idx = TRCS_RANGE_INDEX_HIGH;
	trcs_ctx.trcs_switch_pending = 0;
	trcs_ctx.trcs_iout_ua = 0;
	for (idx=0 ; idx<TRCS_ADC_SAMPLE_BUFFER_LENGTH ; idx++) trcs_ctx.trcs_iout_12bits_buf[idx] = 0;
	trcs_ctx.trcs_iout_12bits_buf_idx = 0;
	trcs_ctx.trcs_iout_12bits = 0;
	trcs_ctx.trcs_bypass_flag = GPIO_Read(&GPIO_TRCS_BYPASS);
	// Enable bypass switch interrupt.
	EXTI_ConfigureGpio(&GPIO_TRCS_BYPASS, EXTI_TRIGGER_ANY_EDGE);
	NVIC_SetPriority(NVIC_IT_EXTI_4_15, 2);
	NVIC_EnableInterrupt(NVIC_IT_EXTI_4_15);
}

/* TRCS BOARD CONTROL FUNCTION.
 * @param trcs_current_ma:	Current measured by TRCS board in mA.
 * @param bypass_status:	Bypass switch status.
 * @return:					None.
 */
void TRCS_Task(void) {
	// Increment recovery timer.
	unsigned char idx = 0;
	for (idx=0 ; idx<TRCS_RANGE_INDEX_LAST ; idx++) {
		trcs_range_table[idx].switch_timer_ms += (PSFE_ADC_CONVERSION_PERIOD_MS * trcs_range_table[idx].switch_request_pending);
	}
	// Update ADC result and output current.
	TRCS_UpdateAdcResult();
	TRCS_ComputeIout();
	// Compute range.
	if (trcs_ctx.trcs_bypass_flag != 0) {
		// Keep high range to ensure power continuity when bypassed will be disabled.
		trcs_ctx.trcs_current_range_idx = TRCS_RANGE_INDEX_HIGH;
	}
	else {
		// Check ADC result.
		if (trcs_ctx.trcs_iout_12bits > TRCS_RANGE_UP_THRESHOLD_12BITS) {
			trcs_ctx.trcs_current_range_idx += (!trcs_ctx.trcs_switch_pending);
		}
		if ((trcs_ctx.trcs_iout_12bits < TRCS_RANGE_DOWN_THRESHOLD_12BITS) && (trcs_ctx.trcs_current_range_idx > 0)) {
			trcs_ctx.trcs_current_range_idx -= (!trcs_ctx.trcs_switch_pending);
		}
	}
	// Check if GPIO control is required.
	if (trcs_ctx.trcs_current_range_idx > TRCS_RANGE_INDEX_HIGH) {
		// Disable all range (protection mode).
		TRCS_Off();
	}
	else {
		if (trcs_ctx.trcs_current_range_idx != trcs_ctx.trcs_previous_range_idx) {
			// Enable new range.
			GPIO_Write(trcs_range_table[trcs_ctx.trcs_current_range_idx].gpio, 1);
			// Start off timer.
			trcs_range_table[trcs_ctx.trcs_previous_range_idx].switch_timer_ms = 0;
			trcs_range_table[trcs_ctx.trcs_previous_range_idx].switch_request_pending = 1;
			trcs_ctx.trcs_switch_pending++;
		}
		for (idx=0 ; idx<TRCS_RANGE_INDEX_LAST ; idx++) {
			// Check recovery timer.
			if (trcs_range_table[idx].switch_timer_ms >= TRCS_RANGE_RECOVERY_DELAY_MS) {
				// Disable range.
				GPIO_Write(trcs_range_table[idx].gpio, 0);
			}
			if (trcs_range_table[idx].switch_timer_ms >= (TRCS_RANGE_RECOVERY_DELAY_MS + TRCS_RANGE_STABILIZATION_DELAY_MS)) {
				// Stop timer.
				trcs_range_table[idx].switch_timer_ms = 0;
				trcs_range_table[idx].switch_request_pending = 0;
				trcs_ctx.trcs_switch_pending--;
			}
		}
	}
	// Update previous index.
	trcs_ctx.trcs_previous_range_idx = trcs_ctx.trcs_current_range_idx;
}

/* SET TRCS BYPASS STATUS (CALLED BY EXTI INTERRUPT).
 * @param bypass_state:	Bypass switch state.
 * @return:				None.
 */
void TRCS_SetBypassFlag(unsigned char bypass_state) {
	// Set local flag.
	trcs_ctx.trcs_bypass_flag = bypass_state;
}

/* GET CURRENT TRCS RANGE.
 * @param range:	Pointer that will contain current TRCS range.
 * @return:			None.
 */
void TRCS_GetRange(volatile TRCS_Range* range) {
	(*range) = trcs_range_table[trcs_ctx.trcs_current_range_idx].range;
}

/* GET CURRENT TRCS OUTPUT CURRENT.
 * @param iout_ua:	Pointer that will contain TRCS current in uA.
 * @return:			None.
 */
void TRCS_GetIout(volatile unsigned int* iout_ua) {
	(*iout_ua) = trcs_ctx.trcs_iout_ua;
}

/* SWITCH TRCS BOARD OFF.
 * @param:	None.
 * @return:	None.
 */
void TRCS_Off(void) {
	// Disable all ranges.
	GPIO_Write(&GPIO_TRCS_RANGE_HIGH, 0);
	GPIO_Write(&GPIO_TRCS_RANGE_MIDDLE, 0);
	GPIO_Write(&GPIO_TRCS_RANGE_LOW, 0);
}
