/*
 * adc.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"

#include "adc_reg.h"
#include "gpio.h"
#include "mapping.h"
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
	GPIO_Configure(GPIO_BANDGAP, Analog, OpenDrain, LowSpeed, NoPullUpNoPullDown);
	GPIO_Configure(GPIO_VOLTAGE_SENSE, Analog, OpenDrain, LowSpeed, NoPullUpNoPullDown);
	GPIO_Configure(GPIO_CURRENT_SENSE, Analog, OpenDrain, LowSpeed, NoPullUpNoPullDown);

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

	/* Wake-up internal voltage reference */
	ADC1 -> CCR |= (0b1 << 22); // VREFEN='1'.
	TIM22_WaitMilliseconds(10); // Wait al least 3ms (see p.55 of STM32L031x4/6 datasheet).

	/* Enable ADC peripheral */
	ADC1 -> CR |= (0b1 << 0); // ADEN='1'.
	while (((ADC1 -> ISR) & (0b1 << 0)) == 0); // Wait for ADC to be ready (ADRDY='1').
}

/* COMPUTE MCU SUPPLY VOLTAGE WITH INTERNAL REFERENCE.
 * @param supply_voltage_mv:	Pointer to int that will contain MCU supply voltage in mV.
 * @return:						None.
 */
void ADC_GetSupplyVoltageMv(unsigned int* supply_voltage_mv) {

	/* Select ADC_IN17 input channel*/
	ADC1 -> CHSELR = 0;
	ADC1 -> CHSELR |= (0b1 << 17);

	/* Set sampling time (see p.89 of STM32L031x4/6 datasheet) */
	ADC1 -> SMPR &= ~(0b111 << 0); // Reset bits 0-2.
	ADC1 -> SMPR |= (0b110 << 0); // Sampling time for internal voltage reference is 10�s typical, 79.5*(1/ADCCLK) = 9.94�s for SYSCLK = 16MHz;

	/* Read raw supply voltage */
	ADC1 -> CR |= (0b1 << 2); // ADSTART='1'.
	while (((ADC1 -> ISR) & (0b1 << 2)) == 0); // Wait end of conversion ('EOC='1').
	unsigned int raw_supply_voltage_12bits = (ADC1 -> DR);

	/* Compute temperature according to MCU factory calibration (see p.301 of RM0377 datasheet) */
	(*supply_voltage_mv) = (VREFINT_VCC_CALIB_MV * VREFINT_CAL) / (raw_supply_voltage_12bits);
}

/* PERFORM INTERNAL ADC MEASUREMENTS.
 * @param:	None.
 * @return:	None.
 */
void ADC1_GetChannel12Bits(unsigned char channel, unsigned int* channel_result_12bits) {

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
