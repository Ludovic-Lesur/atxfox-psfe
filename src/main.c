/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"
#include "rcc.h"
#include "tim.h"
#include "usart.h"

/*** Main macros ***/

#define ATXFE_LOOP_PERIOD_SECONDS	1

/*** Main structures ***/

typedef struct {
	unsigned int atxfe_loop_start_time_seconds;
	unsigned int atxfe_bandgap_12bits;
	unsigned int atxfe_voltage_sense_12bits;
	unsigned int atxfe_current_sense_12bits;
} ATXFE_Context;

/*** Main global variables ***/

ATXFE_Context atxfe_ctx;

/*** Main function ***/

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {

	/* Init clock */
	RCC_Init();

	/* Init peripherals */
	TIM21_Init();
	TIM22_Init();
	ADC1_Init();
	USART2_Init();

	/* Init context */
	atxfe_ctx.atxfe_loop_start_time_seconds = 0;
	atxfe_ctx.atxfe_bandgap_12bits = 0;
	atxfe_ctx.atxfe_voltage_sense_12bits = 0;
	atxfe_ctx.atxfe_current_sense_12bits = 0;

	/* Main loop */
	while (1) {

		// Store loop start time.
		atxfe_ctx.atxfe_loop_start_time_seconds = TIM22_GetSeconds();

		// Get ADC results.
		ADC1_GetChannel12Bits(ADC_BANDGAP_CHANNEL, &atxfe_ctx.atxfe_bandgap_12bits);
		ADC1_GetChannel12Bits(ADC_VOLTAGE_SENSE_CHANNEL, &atxfe_ctx.atxfe_voltage_sense_12bits);
		ADC1_GetChannel12Bits(ADC_CURRENT_SENSE_CHANNEL, &atxfe_ctx.atxfe_current_sense_12bits);

		// Print on UART.
		USART2_SendString("\nBandgap = ");
		USART2_SendValue(atxfe_ctx.atxfe_bandgap_12bits, USART_FORMAT_DECIMAL, 0);
		USART2_SendString("\nVoltage sense = ");
		USART2_SendValue(atxfe_ctx.atxfe_voltage_sense_12bits, USART_FORMAT_DECIMAL, 0);
		USART2_SendString("\nCurrent sense = ");
		USART2_SendValue(atxfe_ctx.atxfe_current_sense_12bits, USART_FORMAT_DECIMAL, 0);

		// Wait to reach 1 second period.
		while (TIM22_GetSeconds() < (atxfe_ctx.atxfe_loop_start_time_seconds + ATXFE_LOOP_PERIOD_SECONDS));
	}

	return 0;
}

