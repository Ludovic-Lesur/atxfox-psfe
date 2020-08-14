/*
 * trcs.c
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludovic
 */

#include "trcs.h"

#include "adc.h"
#include "atxfox.h"
#include "filter.h"
#include "gpio.h"
#include "lptim.h"
#include "mapping.h"
#include "tim.h"

/*** TRCS local macros ***/

#define TRCS_NUMBER_OF_RANGES				3

#define TRCS_RANGE_RECOVERY_TIME_MS			100		// Recovery time when switching ranges.
#define TRCS_RANGE_STABILIZATION_TIME_MS	100 	// Delay applied after switching operation.

#define TRCS_RANGE_UP_THRESHOLD_12BITS		3900	// Range will be increased if ADC result is higher.
#define TRCS_RANGE_DOWN_THRESHOLD_12BITS	30		// Range will be decreased if ADC result is lower.

#define TRCS_RANGE_SWITCH_CONFIRMATION		10		// Number of valid measures to validate a range switch operation.

#define TRCS_LT6105_VOLTAGE_GAIN			59		// LT6105 with 100R and 5.9k resistors.

#define TRCS_MEDIAN_FILTER_LENGTH			31		// Length of the median filter window in samples.
#define TRCS_CENTER_AVERAGE_LENGTH			5

const unsigned int trcs_resistor_mohm_table[ATXFOX_NUMBER_OF_BOARDS][TRCS_NUMBER_OF_RANGES] = {
	{52000, 510, 6}, // Board 1.0.1 - calibrated.
	{52000, 510, 5}, // Board 1.0.2 - calibrated.
	{53000, 510, 6}, // Board 1.0.3 - calibrated.
	{51000, 510, 6}, // Board 1.0.4 - calibrated.
	{53000, 530, 6}, // Board 1.0.5 - calibrated.
	{53000, 520, 6}, // Board 1.0.6 - calibrated.
	{50000, 500, 5}, // Board 1.0.7 - calibrated.
	{53000, 540, 7}, // Board 1.0.8 - calibrated.
	{52000, 520, 6}, // Board 1.0.9 - calibrated.
	{50000, 500, 5}  // Board 1.0.10 - calibrated.
};

/*** TRCS local structures ***/

typedef enum {
	TRCS_RANGE_NONE,
	TRCS_RANGE_LOW,
	TRCS_RANGE_MIDDLE,
	TRCS_RANGE_HIGH
} TRCS_Range;

typedef enum {
	TRCS_STATE_OFF,
	TRCS_STATE_HIGH,
	TRCS_STATE_CONFIRM_MIDDLE,
	TRCS_STATE_MIDDLE,
	TRCS_STATE_CONFIRM_LOW,
	TRCS_STATE_LOW
} TRCS_State;

typedef struct {
	TRCS_Range trcs_current_range;
	TRCS_State trcs_state;
	unsigned char trcs_range_up_counter;
	unsigned char trcs_range_down_counter;
	unsigned int trcs_adc_sample_buf[TRCS_MEDIAN_FILTER_LENGTH];
	unsigned char trcs_current_ua_buf_idx;
	unsigned int trcs_average_current_ua;
} TRCS_Context;

/*** TRCS local global variables ***/

static TRCS_Context trcs_ctx;

/*** TRCS local functions ***/

/* CONTROL ALL RELAYS TO ACTIVATE A GIVEN CURRENT RANGE.
 * @param trcs_range:	Range to activate (see TRCS_Range structure).
 * @return:				None.
 */
void TRCS_SetRange(TRCS_Range trcs_range) {
	// Configure relays.
	switch (trcs_range) {
	case TRCS_RANGE_NONE:
		GPIO_Write(&GPIO_TRCS_RANGE_LOW, 0);
		GPIO_Write(&GPIO_TRCS_RANGE_MIDDLE, 0);
		GPIO_Write(&GPIO_TRCS_RANGE_HIGH, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_NONE;
		break;
	case TRCS_RANGE_LOW:
		GPIO_Write(&GPIO_TRCS_RANGE_LOW, 1);
		LPTIM1_DelayMilliseconds(TRCS_RANGE_RECOVERY_TIME_MS);
		GPIO_Write(&GPIO_TRCS_RANGE_MIDDLE, 0);
		GPIO_Write(&GPIO_TRCS_RANGE_HIGH, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_LOW;
		break;
	case TRCS_RANGE_MIDDLE:
		GPIO_Write(&GPIO_TRCS_RANGE_MIDDLE, 1);
		LPTIM1_DelayMilliseconds(TRCS_RANGE_RECOVERY_TIME_MS);
		GPIO_Write(&GPIO_TRCS_RANGE_LOW, 0);
		GPIO_Write(&GPIO_TRCS_RANGE_HIGH, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_MIDDLE;
		break;
	case TRCS_RANGE_HIGH:
		GPIO_Write(&GPIO_TRCS_RANGE_HIGH, 1);
		LPTIM1_DelayMilliseconds(TRCS_RANGE_RECOVERY_TIME_MS);
		GPIO_Write(&GPIO_TRCS_RANGE_LOW, 0);
		GPIO_Write(&GPIO_TRCS_RANGE_MIDDLE, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_HIGH;
		break;
	default:
		// Unknwon parameter.
		break;
	}
	// Wait establishment.
	LPTIM1_DelayMilliseconds(TRCS_RANGE_STABILIZATION_TIME_MS);
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
	TRCS_SetRange(TRCS_RANGE_NONE);
	// Init context.
	trcs_ctx.trcs_current_range = TRCS_RANGE_NONE;
	trcs_ctx.trcs_state = TRCS_STATE_OFF;
	trcs_ctx.trcs_range_down_counter = 0;
	unsigned char idx = 0;
	for (idx=0 ; idx<TRCS_MEDIAN_FILTER_LENGTH ; idx++) trcs_ctx.trcs_adc_sample_buf[idx] = 0;
	trcs_ctx.trcs_current_ua_buf_idx = 0;
	trcs_ctx.trcs_average_current_ua = 0;
}

/* TRCS BOARD CONTROL FUNCTION.
 * @param trcs_current_ma:	Current measured by TRCS board in mA.
 * @param bypass_status:	Bypass switch status.
 * @return:					None.
 */
void TRCS_Task(unsigned int adc_bandgap_result_12bits, unsigned int* trcs_current_ua, unsigned char bypass) {
	// Perform measurement.
	unsigned int adc_channel_result_12bits = 0;
	ADC1_GetChannel12Bits(ADC_ATX_CURRENT_CHANNEL, &adc_channel_result_12bits);
	// Store new sample in buffer.
	trcs_ctx.trcs_adc_sample_buf[trcs_ctx.trcs_current_ua_buf_idx] = adc_channel_result_12bits;
	trcs_ctx.trcs_current_ua_buf_idx++;
	if (trcs_ctx.trcs_current_ua_buf_idx >= TRCS_MEDIAN_FILTER_LENGTH) {
		trcs_ctx.trcs_current_ua_buf_idx = 0;
	}
	// Compute median value.
	unsigned int averaged_median_value = FILTER_ComputeMedianFilter(trcs_ctx.trcs_adc_sample_buf, TRCS_MEDIAN_FILTER_LENGTH, TRCS_CENTER_AVERAGE_LENGTH);
	// Convert to uA.
	unsigned long long num = averaged_median_value;
	num *= ADC_BANDGAP_VOLTAGE_MV;
	num *= 1000000;
	unsigned int resistor_mohm = trcs_resistor_mohm_table[(psfe_trcs_number[PSFE_BOARD_NUMBER] - 1)][trcs_ctx.trcs_current_range - 1]; // (-1 to skip TRCS_RANGE_NONE).
	unsigned long long den = adc_bandgap_result_12bits;
	den *= TRCS_LT6105_VOLTAGE_GAIN;
	den *= resistor_mohm;
	trcs_ctx.trcs_average_current_ua = (num / den);
	(*trcs_current_ua) = trcs_ctx.trcs_average_current_ua;
	// Perform state machine.
	switch (trcs_ctx.trcs_state) {
	// Off state.
	case TRCS_STATE_OFF:
		// Check bypass switch.
		if (bypass == 0) {
			// Begin with highest range.
			trcs_ctx.trcs_state = TRCS_STATE_HIGH;
			TRCS_SetRange(TRCS_RANGE_HIGH);
		}
		break;
	// High range state.
	case TRCS_STATE_HIGH:
		// Check last ADC result.
		if ((adc_channel_result_12bits > TRCS_RANGE_UP_THRESHOLD_12BITS) || (bypass != 0)) {
			// Current is too high and there is no higher range -> switch board off.
			trcs_ctx.trcs_state = TRCS_STATE_OFF;
			TRCS_SetRange(TRCS_RANGE_NONE);
		}
		if (adc_channel_result_12bits < TRCS_RANGE_DOWN_THRESHOLD_12BITS) {
			// Middle Range must be confirmed.
			trcs_ctx.trcs_state = TRCS_STATE_CONFIRM_MIDDLE;
		}
		break;
	// Middle range confirmation state.
	case TRCS_STATE_CONFIRM_MIDDLE:
		// Check ADC result as much as required.
		if (adc_channel_result_12bits < TRCS_RANGE_DOWN_THRESHOLD_12BITS) {
			// Middle range must be confirmed.
			trcs_ctx.trcs_range_down_counter++;
			if (trcs_ctx.trcs_range_down_counter >= TRCS_RANGE_SWITCH_CONFIRMATION) {
				// Middle range is confirmed to achieve better accuracy.
				trcs_ctx.trcs_range_down_counter = 0;
				trcs_ctx.trcs_state = TRCS_STATE_MIDDLE;
				TRCS_SetRange(TRCS_RANGE_MIDDLE);
			}
		}
		else {
			// Middle range not confirmed.
			trcs_ctx.trcs_range_down_counter = 0;
			trcs_ctx.trcs_state = TRCS_STATE_HIGH;
		}
		break;
	// Middle range state.
	case TRCS_STATE_MIDDLE:
		// Check bypass switch.
		if (bypass != 0) {
			// Switch TRCS board off.
			trcs_ctx.trcs_state = TRCS_STATE_OFF;
			TRCS_SetRange(TRCS_RANGE_NONE);
		}
		else {
			// Check last ADC result.
			if (adc_channel_result_12bits > TRCS_RANGE_UP_THRESHOLD_12BITS) {
				// Current is too high -> switch to high range
				trcs_ctx.trcs_state = TRCS_STATE_HIGH;
				TRCS_SetRange(TRCS_RANGE_HIGH);
			}
			if (adc_channel_result_12bits < TRCS_RANGE_DOWN_THRESHOLD_12BITS) {
				// Low Range must be confirmed.
				trcs_ctx.trcs_state = TRCS_STATE_CONFIRM_LOW;
			}
		}

		break;
	// Low range confirmation state.
	case TRCS_STATE_CONFIRM_LOW:
		// Check ADC result as much as required.
		if (adc_channel_result_12bits < TRCS_RANGE_DOWN_THRESHOLD_12BITS) {
			// Low range must be confirmed.
			trcs_ctx.trcs_range_down_counter++;
			if (trcs_ctx.trcs_range_down_counter >= TRCS_RANGE_SWITCH_CONFIRMATION) {
				// Middle range is confirmed to achieve better accuracy.
				trcs_ctx.trcs_range_down_counter = 0;
				trcs_ctx.trcs_state = TRCS_STATE_LOW;
				TRCS_SetRange(TRCS_RANGE_LOW);
			}
		}
		else {
			// Low range not confirmed.
			trcs_ctx.trcs_range_down_counter = 0;
			trcs_ctx.trcs_state = TRCS_STATE_MIDDLE;
		}
		break;
	// Low range state.
	case TRCS_STATE_LOW:
		// Chck bypass switch.
		if (bypass != 0) {
			// Switch TRCS board off.
			trcs_ctx.trcs_state = TRCS_STATE_OFF;
			TRCS_SetRange(TRCS_RANGE_NONE);
		}
		else {
			// Check last ADC result.
			if (adc_channel_result_12bits > TRCS_RANGE_UP_THRESHOLD_12BITS) {
				// Current is too high -> switch to high range
				trcs_ctx.trcs_state = TRCS_STATE_HIGH;
				TRCS_SetRange(TRCS_RANGE_HIGH);
			}
		}
		break;
	// Unknwon state.
	default:
		// Switch TRCS board off.
		trcs_ctx.trcs_state = TRCS_STATE_OFF;
		TRCS_SetRange(TRCS_RANGE_NONE);
		break;
	}
}

/* SWITCH TRCS BOARD OFF.
 * @param:	None.
 * @return:	None.
 */
void TRCS_Off(void) {
	trcs_ctx.trcs_state = TRCS_STATE_OFF;
	TRCS_SetRange(TRCS_RANGE_NONE);
}
