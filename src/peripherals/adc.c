/*
 * adc.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"

#include "adc_reg.h"
#include "gpio_reg.h"
#include "rcc_reg.h"
#include "tim.h"

/*** ADC functions ***/

/* INIT ADC1 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void ADC1_Init(void) {

	/* Enable peripheral clock */
	RCC -> APB2ENR |= (0b1 << 9); // ADCEN='1'.

	/* Configure analog GPIOs */
	RCC -> IOPENR |= (0b11 << 0); // IOPxEN='1'.
	GPIOA -> MODER |= (0b11 << 0); // Select analog mode.
	GPIOB -> MODER |= (0b1111 << 0); // Select analog mode.

	/* Disable ADC before configure it */
	ADC1 -> CR = 0; // ADEN='0'.

	/* Enable ADC voltage regulator */
	ADC1 -> CR |= (0b1 << 28);
	TIM22_WaitMilliseconds(5);

	/* ADC configuration */
	ADC1 -> CFGR2 &= ~(0b11 << 30); // Reset bits 30-31.
	ADC1 -> CFGR2 |= (0b01 << 30); // Use (PCLK2/2) as ADCCLK = SYSCLK/2 (see RCC_Init() function).
	ADC1 -> CFGR1 &= (0b1 << 13); // Single conversion mode.
	ADC1 -> CFGR1 &= ~(0b11 << 0); // Data rsolution = 12 bits (RES='00').

	/* ADC calibration */
	ADC1 -> CR |= (0b1 << 31); // ADCAL='1'.
	while (((ADC1 -> CR) & (0b1 << 31)) != 0); // Wait until calibration is done.

	/* Enable ADC peripheral */
	ADC1 -> CR |= (0b1 << 0); // ADEN='1'.
	while (((ADC1 -> ISR) & (0b1 << 0)) == 0); // Wait for ADC to be ready (ADRDY='1').
}

/* PERFORM INTERNAL ADC MEASUREMENTS.
 * @param:	None.
 * @return:	None.
 */
void ADC1_GetChannel12Bits(unsigned char channel, unsigned int* channel_result_12bits) {

	/* Enable ADC peripheral */
	RCC -> APB2ENR |= (0b1 << 9); // ADCEN='1'.
	ADC1 -> CR |= (0b1 << 0); // ADEN='1'.
	while (((ADC1 -> ISR) & (0b1 << 0)) == 0); // Wait for ADC to be ready (ADRDY='1').

	/* Select channel */
	ADC1 -> CHSELR = 0;
	ADC1 -> CHSELR |= (0b1 << channel);

	/* Set sampling time (see p.89 of STM32L031x4/6 datasheet) */
	ADC1 -> SMPR |= (0b111 << 0); // Sampling time for temperature sensor must be greater than 10µs, 160.5*(1/ADCCLK) = 20µs for SYSCLK = 16MHz;

	/* Read raw result */
	ADC1 -> CR |= (0b1 << 2); // ADSTART='1'.
	while (((ADC1 -> ISR) & (0b1 << 2)) == 0); // Wait end of conversion ('EOC='1').
	(*channel_result_12bits) = (ADC1 -> DR);
}
