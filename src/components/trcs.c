/*
 * trcs.c
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludovic
 */

#include "trcs.h"

#include "adc.h"
#include "gpio.h"
#include "mapping.h"
#include "tim.h"

/*** TRCS local macros ***/

#define TRCS_BOARD_NUMBER					1		// TRCS 1.0.x to be defined to select values from calibration tables.

#define TRCS_NUMBER_OF_BOARDS				10
#define TRCS_NUMBER_OF_RANGES				3

#define TRCS_RANGE_RECOVERY_TIME_MS			100		// Recovery time when switching ranges.
#define TRCS_RANGE_STABILIZATION_TIME_MS	100 	// Delay applied after switching operation.

#define TRCS_RANGE_UP_THRESHOLD_12BITS		3900	// Range will be increased if ADC result is higher.
#define TRCS_RANGE_DOWN_THRESHOLD_12BITS	30		// Range will be decreased if ADC result is lower.

#define TRCS_RANGE_SWITCH_CONFIRMATION		10		// Number of valid measures to validate a range switch operation.

#define TRCS_AD8219_VOLTAGE_GAIN			60

const unsigned int trcs_resistor_mohm_table[TRCS_NUMBER_OF_BOARDS][TRCS_NUMBER_OF_RANGES] = {
	{50000, 500, 5}, // Board 1.0.1
	{50000, 500, 5}, // Board 1.0.2
	{50000, 500, 5}, // Board 1.0.3
	{50000, 500, 5}, // Board 1.0.4
	{50000, 500, 5}, // Board 1.0.5
	{50000, 500, 5}, // Board 1.0.6
	{50000, 500, 5}, // Board 1.0.7
	{50000, 500, 5}, // Board 1.0.8
	{50000, 500, 5}, // Board 1.0.9
	{50000, 500, 5}  // Board 1.0.10
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
} TRCS_Context;

/*** TRCS local global variables ***/

static TRCS_Context trcs_ctx;

/*** TRCS local functions ***/

/* CONTROL ALL RELAYS TO ACTIVATE A GIVEN CURRENT RANGE.
 * @param trcs_range:	Range to activate (see TRCS_Range structure).
 * @return:				None.
 */
void TRCS_SetRange(TRCS_Range trcs_range) {

	/* Configure relays */
	switch (trcs_range) {

	case TRCS_RANGE_NONE:
		GPIO_Write(GPIO_TRCS_RANGE_LOW, 0);
		GPIO_Write(GPIO_TRCS_RANGE_MIDDLE, 0);
		GPIO_Write(GPIO_TRCS_RANGE_HIGH, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_NONE;
		break;

	case TRCS_RANGE_LOW:
		GPIO_Write(GPIO_TRCS_RANGE_LOW, 1);
		TIM22_WaitMilliseconds(TRCS_RANGE_RECOVERY_TIME_MS);
		GPIO_Write(GPIO_TRCS_RANGE_MIDDLE, 0);
		GPIO_Write(GPIO_TRCS_RANGE_HIGH, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_LOW;
		break;

	case TRCS_RANGE_MIDDLE:
		GPIO_Write(GPIO_TRCS_RANGE_MIDDLE, 1);
		TIM22_WaitMilliseconds(TRCS_RANGE_RECOVERY_TIME_MS);
		GPIO_Write(GPIO_TRCS_RANGE_LOW, 0);
		GPIO_Write(GPIO_TRCS_RANGE_HIGH, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_MIDDLE;
		break;

	case TRCS_RANGE_HIGH:
		GPIO_Write(GPIO_TRCS_RANGE_HIGH, 1);
		TIM22_WaitMilliseconds(TRCS_RANGE_RECOVERY_TIME_MS);
		GPIO_Write(GPIO_TRCS_RANGE_LOW, 0);
		GPIO_Write(GPIO_TRCS_RANGE_MIDDLE, 0);
		trcs_ctx.trcs_current_range = TRCS_RANGE_HIGH;
		break;

	default:
		// Unknwon parameter.
		break;
	}

	/* Wait establishment */
	TIM22_WaitMilliseconds(TRCS_RANGE_STABILIZATION_TIME_MS);
}

/*** TRCS functions ***/

/* INIT ALL GPIOs CONTROLLING RANGE RELAYS.
 * @param:	None.
 * @return:	None.
 */
void TRCS_Init(void) {

	/* Init context */
	trcs_ctx.trcs_current_range = TRCS_RANGE_NONE;
	trcs_ctx.trcs_state = TRCS_STATE_OFF;
	trcs_ctx.trcs_range_down_counter = 0;

	/* Init GPIOs */
	GPIO_Configure(GPIO_TRCS_RANGE_LOW, Output, PushPull, HighSpeed, NoPullUpNoPullDown);
	GPIO_Configure(GPIO_TRCS_RANGE_MIDDLE, Output, PushPull, HighSpeed, NoPullUpNoPullDown);
	GPIO_Configure(GPIO_TRCS_RANGE_HIGH, Output, PushPull, HighSpeed, NoPullUpNoPullDown);
	TRCS_SetRange(TRCS_RANGE_NONE);
}

/* TRCS BOARD CONTROL FUNCTION.
 * @param trcs_current_ma:	Current measured by TRCS board in mA.
 * @param bypass_status:	Bypass switch status.
 * @return:					None.
 */
void TRCS_Task(unsigned int* trcs_current_ua, unsigned char bypass) {

	/* Get raw current sense */
	unsigned int adc_bandgap_result_12bits = 0;
	ADC1_GetChannel12Bits(ADC_BANDGAP_CHANNEL, &adc_bandgap_result_12bits);
	unsigned int adc_channel_result_12bits = 0;
	ADC1_GetChannel12Bits(ADC_ATX_CURRENT_CHANNEL, &adc_channel_result_12bits);

	/* Convert ADC result to µA */
	unsigned long long num = adc_channel_result_12bits;
	num *= ADC_BANDGAP_VOLTAGE_MV;
	num *= 1000000;
	unsigned int resistor_mohm = trcs_resistor_mohm_table[TRCS_BOARD_NUMBER - 1][trcs_ctx.trcs_current_range - 1]; // (-1 tp skip TRCS_RANGE_NONE).
	unsigned long long den = adc_bandgap_result_12bits;
	den *= TRCS_AD8219_VOLTAGE_GAIN;
	den *= resistor_mohm;
	(*trcs_current_ua) = (num) / (den);

	/* Perform state machine */
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
