/*
 * trcs.c
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
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

static const uint32_t trcs_low_range_resistor_mohms[PSFE_NUMBER_OF_BOARDS] = {52000, 52000, 53000, 51000, 53000, 53000, 50000, 53000, 52000, 50000};
static const uint32_t trcs_middle_range_resistor_mohms[PSFE_NUMBER_OF_BOARDS] = {510, 510, 510, 510, 530, 520, 500, 540, 520, 500};
static const uint32_t trcs_high_range_resistor_mohms[PSFE_NUMBER_OF_BOARDS] = 	{6, 5, 6, 6, 6, 6, 5, 7, 6, 5};

/*** TRCS local structures ***/

typedef enum {
	TRCS_RANGE_INDEX_LOW = 0,
	TRCS_RANGE_INDEX_MIDDLE,
	TRCS_RANGE_INDEX_HIGH,
	TRCS_RANGE_INDEX_LAST
} TRCS_range_index_t;

typedef struct {
	const TRCS_range_t range;
	const GPIO_pin_t* gpio;
	const uint32_t resistor_mohm;
	uint8_t switch_request_pending;
	uint32_t switch_timer_ms;
} TRCS_range_info_t;

typedef struct {
	uint8_t current_range_idx;
	uint8_t previous_range_idx;
	uint8_t switch_pending;
	uint32_t iout_12bits_buf[TRCS_ADC_SAMPLE_BUFFER_LENGTH];
	uint8_t iout_12bits_buf_idx;
	uint32_t iout_12bits;
	uint32_t iout_ua;
	uint8_t bypass_flag;
} TRCS_context_t;

/*** TRCS local global variables ***/

static volatile TRCS_context_t trcs_ctx;
static TRCS_range_info_t trcs_range_table[TRCS_RANGE_INDEX_LAST] = {{TRCS_RANGE_LOW, &GPIO_TRCS_RANGE_LOW, trcs_low_range_resistor_mohms[PSFE_BOARD_INDEX], 0, 0},
															      	{TRCS_RANGE_MIDDLE, &GPIO_TRCS_RANGE_MIDDLE, trcs_middle_range_resistor_mohms[PSFE_BOARD_INDEX], 0, 0},
																	{TRCS_RANGE_HIGH, &GPIO_TRCS_RANGE_HIGH, trcs_high_range_resistor_mohms[PSFE_BOARD_INDEX], 0, 0}};

/*** TRCS local functions ***/

/* UPDATE RAW ADC RESULT.
 * @param:			None.
 * @return status:	Function execution status.
 */
static TRCS_status_t _TRCS_update_adc_data(void) {
	// Local variables.
	TRCS_status_t status = TRCS_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	MATH_status_t math_status = MATH_SUCCESS;
	// Add sample
	adc1_status = ADC1_get_data(ADC_DATA_INDEX_IOUT_12BITS, (uint32_t*) &trcs_ctx.iout_12bits_buf[trcs_ctx.iout_12bits_buf_idx]);
	ADC1_status_check(TRCS_ERROR_BASE_ADC);
	// Update average.
	math_status = MATH_average_u32((uint32_t*) trcs_ctx.iout_12bits_buf, TRCS_ADC_SAMPLE_BUFFER_LENGTH, (uint32_t*) &trcs_ctx.iout_12bits);
	MATH_status_check(TRCS_ERROR_BASE_MATH);
	// Manage index.
	trcs_ctx.iout_12bits_buf_idx++;
	if (trcs_ctx.iout_12bits_buf_idx >= TRCS_ADC_SAMPLE_BUFFER_LENGTH) {
		trcs_ctx.iout_12bits_buf_idx = 0;
	}
errors:
	return status;
}

/* COMPUTE OUTPUT CURRENT IN MICRO-AMPS.
 * @param:			None.
 * @return status:	Function execution status.
 */
static TRCS_status_t _TRCS_compute_iout(void) {
	// Local variables.
	TRCS_status_t status = TRCS_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	uint32_t ref191_12bits = 0;
	uint32_t resistor_mohm = 0;
	uint64_t num = 0;
	uint64_t den = 0;
	// Get bandgap measurement.
	adc1_status = ADC1_get_data(ADC_DATA_INDEX_REF191_12BITS, &ref191_12bits);
	ADC1_status_check(TRCS_ERROR_BASE_ADC);
	// Convert to uA.
	num = (uint64_t) trcs_ctx.iout_12bits;
	num *= ADC_REF191_VOLTAGE_MV;
	num *= 1000000;
	resistor_mohm = trcs_range_table[trcs_ctx.current_range_idx].resistor_mohm;
	den = (uint64_t) ref191_12bits;
	den *= TRCS_LT6105_VOLTAGE_GAIN;
	den *= resistor_mohm;
	trcs_ctx.iout_ua = (num / den);
errors:
	return status;
}

/*** TRCS functions ***/

/* INIT ALL GPIOs CONTROLLING RANGE RELAYS.
 * @param:	None.
 * @return:	None.
 */
void TRCS_init(void) {
	// Local variables.
	uint8_t idx = 0;
	// Init GPIOs.
	GPIO_configure(&GPIO_TRCS_RANGE_LOW, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_TRCS_RANGE_MIDDLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_TRCS_RANGE_HIGH, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_TRCS_BYPASS, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Set board in high range by default to ensure power continuity.
	GPIO_write(&GPIO_TRCS_RANGE_HIGH, 1);
	GPIO_write(&GPIO_TRCS_RANGE_MIDDLE, 0);
	GPIO_write(&GPIO_TRCS_RANGE_LOW, 0);
	// Init context.
	trcs_ctx.current_range_idx = TRCS_RANGE_INDEX_HIGH;
	trcs_ctx.previous_range_idx = TRCS_RANGE_INDEX_HIGH;
	trcs_ctx.switch_pending = 0;
	trcs_ctx.iout_ua = 0;
	for (idx=0 ; idx<TRCS_ADC_SAMPLE_BUFFER_LENGTH ; idx++) trcs_ctx.iout_12bits_buf[idx] = 0;
	trcs_ctx.iout_12bits_buf_idx = 0;
	trcs_ctx.iout_12bits = 0;
	trcs_ctx.bypass_flag = GPIO_read(&GPIO_TRCS_BYPASS);
	// Enable bypass switch interrupt.
	EXTI_configure_gpio(&GPIO_TRCS_BYPASS, EXTI_TRIGGER_ANY_EDGE);
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_4_15, 2);
	NVIC_enable_interrupt(NVIC_INTERRUPT_EXTI_4_15);
}

/* TRCS BOARD CONTROL FUNCTION.
 * @param trcs_current_ma:	Current measured by TRCS board in mA.
 * @param bypass_status:	Bypass switch status.
 * @return status:			Function execution status.
 */
TRCS_status_t TRCS_task(void) {
	// Local variables.
	TRCS_status_t status = TRCS_SUCCESS;
	uint8_t idx = 0;
	// Increment recovery timer.
	for (idx=0 ; idx<TRCS_RANGE_INDEX_LAST ; idx++) {
		trcs_range_table[idx].switch_timer_ms += (PSFE_ADC_CONVERSION_PERIOD_MS * trcs_range_table[idx].switch_request_pending);
	}
	// Update ADC result and output current.
	status = _TRCS_update_adc_data();
	if (status != TRCS_SUCCESS) goto errors;
	status = _TRCS_compute_iout();
	if (status != TRCS_SUCCESS) goto errors;
	// Compute range.
	if (trcs_ctx.bypass_flag != 0) {
		// Keep high range to ensure power continuity when bypassed will be disabled.
		trcs_ctx.current_range_idx = TRCS_RANGE_INDEX_HIGH;
	}
	else {
		// Check ADC result.
		if (trcs_ctx.iout_12bits > TRCS_RANGE_UP_THRESHOLD_12BITS) {
			trcs_ctx.current_range_idx += (!trcs_ctx.switch_pending);
		}
		if ((trcs_ctx.iout_12bits < TRCS_RANGE_DOWN_THRESHOLD_12BITS) && (trcs_ctx.current_range_idx > 0)) {
			trcs_ctx.current_range_idx -= (!trcs_ctx.switch_pending);
		}
	}
	// Check if GPIO control is required.
	if (trcs_ctx.current_range_idx > TRCS_RANGE_INDEX_HIGH) {
		// Disable all range (protection mode).
		TRCS_off();
		status = TRCS_ERROR_OVERFLOW;
		goto errors;
	}
	else {
		if (trcs_ctx.current_range_idx != trcs_ctx.previous_range_idx) {
			// Enable new range.
			GPIO_write(trcs_range_table[trcs_ctx.current_range_idx].gpio, 1);
			// Start off timer.
			trcs_range_table[trcs_ctx.previous_range_idx].switch_timer_ms = 0;
			trcs_range_table[trcs_ctx.previous_range_idx].switch_request_pending = 1;
			trcs_ctx.switch_pending++;
		}
		for (idx=0 ; idx<TRCS_RANGE_INDEX_LAST ; idx++) {
			// Check recovery timer.
			if (trcs_range_table[idx].switch_timer_ms >= TRCS_RANGE_RECOVERY_DELAY_MS) {
				// Disable range.
				GPIO_write(trcs_range_table[idx].gpio, 0);
			}
			if (trcs_range_table[idx].switch_timer_ms >= (TRCS_RANGE_RECOVERY_DELAY_MS + TRCS_RANGE_STABILIZATION_DELAY_MS)) {
				// Stop timer.
				trcs_range_table[idx].switch_timer_ms = 0;
				trcs_range_table[idx].switch_request_pending = 0;
				trcs_ctx.switch_pending--;
			}
		}
	}
	// Update previous index.
	trcs_ctx.previous_range_idx = trcs_ctx.current_range_idx;
errors:
	return status;
}

/* SET TRCS BYPASS STATUS (CALLED BY EXTI INTERRUPT).
 * @param bypass_state:	Bypass switch state.
 * @return:				None.
 */
void TRCS_set_bypass_flag(uint8_t bypass_state) {
	// Set local flag.
	trcs_ctx.bypass_flag = bypass_state;
}

/* GET CURRENT TRCS RANGE.
 * @param range:	Pointer that will contain current TRCS range.
 * @return status:	Function execution status.
 */
TRCS_status_t TRCS_get_range(volatile TRCS_range_t* range) {
	// Local variables.
	TRCS_status_t status = TRCS_SUCCESS;
	// Check parameter.
	if (range == NULL) {
		status = TRCS_ERROR_NULL_PARAMETER;
		goto errors;
	}
	(*range) = trcs_range_table[trcs_ctx.current_range_idx].range;
errors:
	return status;
}

/* GET CURRENT TRCS OUTPUT CURRENT.
 * @param iout_ua:	Pointer that will contain TRCS current in uA.
 * @return status:	Function execution status.
 */
TRCS_status_t TRCS_get_iout(volatile uint32_t* iout_ua) {
	// Local variables.
	TRCS_status_t status = TRCS_SUCCESS;
	// Check parameter.
	if (iout_ua == NULL) {
		status = TRCS_ERROR_NULL_PARAMETER;
		goto errors;
	}
	(*iout_ua) = trcs_ctx.iout_ua;
errors:
	return status;
}

/* SWITCH TRCS BOARD OFF.
 * @param:	None.
 * @return:	None.
 */
void TRCS_off(void) {
	// Disable all ranges.
	GPIO_write(&GPIO_TRCS_RANGE_HIGH, 0);
	GPIO_write(&GPIO_TRCS_RANGE_MIDDLE, 0);
	GPIO_write(&GPIO_TRCS_RANGE_LOW, 0);
}
