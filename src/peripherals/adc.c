/*
 * adc.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"

#include "adc_reg.h"
#include "gpio.h"
#include "lptim.h"
#include "mapping.h"
#include "rcc_reg.h"

/*** ADC local macros ***/

#define ADC_TIMEOUT_COUNT					1000000
#define ADC_CHANNEL_VREFINT					17
#define ADC_CHANNEL_TEMPERATURE_SENSOR		18
#define ADC_MCU_SUPPLY_VOLTAGE_MV_DEFAULT	3400

/*** ADC local structures ***/

typedef struct {
	unsigned int adc_mcu_supply_voltage_mv;
} ADC_Context;

/*** ADC local global variables ***/

static ADC_Context adc_ctx;

/*** ADC functions ***/

/* INIT ADC1 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void ADC1_Init(void) {
	// Init context.
	adc_ctx.adc_mcu_supply_voltage_mv = ADC_MCU_SUPPLY_VOLTAGE_MV_DEFAULT;
	// Configure analog GPIOs */
	GPIO_Configure(&GPIO_BANDGAP, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_VOLTAGE_SENSE, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_CURRENT_SENSE, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Enable peripheral clock */
	RCC -> APB2ENR |= (0b1 << 9); // ADCEN='1'.
	// Disable ADC before configure it.
	ADC1 -> CR = 0; // ADEN='0'.
	// Enable ADC voltage regulator.
	ADC1 -> CR |= (0b1 << 28);
	LPTIM1_DelayMilliseconds(5);
	// ADC configuration.
	ADC1 -> CFGR2 &= ~(0b11 << 30); // Reset bits 30-31.
	ADC1 -> CFGR2 |= (0b01 << 30); // Use (PCLK2/2) as ADCCLK = SYSCLK/2 (see RCC_Init() function).
	ADC1 -> CFGR1 &= (0b1 << 13); // Single conversion mode.
	ADC1 -> CFGR1 &= ~(0b11 << 0); // Data resolution = 12 bits (RES='00').
	// ADC calibration.
	ADC1 -> CR |= (0b1 << 31); // ADCAL='1'.
	while (((ADC1 -> CR) & (0b1 << 31)) != 0); // Wait until calibration is done.
	// Wake-up temperature sensor and internal voltage reference.
	ADC1 -> CCR |= (0b11 << 22); // TSEN='1' and VREFEN='1'.
	LPTIM1_DelayMilliseconds(10); // Wait al least 3ms (see p.55 of STM32L031x4/6 datasheet).
	// Enable ADC peripheral.
	ADC1 -> CR |= (0b1 << 0); // ADEN='1'.
	while (((ADC1 -> ISR) & (0b1 << 0)) == 0); // Wait for ADC to be ready (ADRDY='1').
}

/* PERFORM A SINGLE ADC CONVERSION.
 * @param adc_channel:			Channel to convert.
 * @param adc_result_12bits:	Pointer to int that will contain ADC raw result on 12 bits.
 * @return:						None.
 */
void ADC1_SingleConversion(unsigned char adc_channel, unsigned int* adc_result_12bits) {
	// Select input channel.
	ADC1 -> CHSELR &= 0xFFF80000; // Reset all bits.
	ADC1 -> CHSELR |= (0b1 << adc_channel);
	// Read raw supply voltage.
	ADC1 -> CR |= (0b1 << 2); // ADSTART='1'.
	unsigned int loop_count = 0;
	while (((ADC1 -> ISR) & (0b1 << 2)) == 0) {
		// Wait end of conversion ('EOC='1') or timeout.
		loop_count++;
		if (loop_count > ADC_TIMEOUT_COUNT) return;
	}
	(*adc_result_12bits) = (ADC1 -> DR);
}

/* COMPUTE MCU SUPPLY VOLTAGE WITH INTERNAL REFERENCE.
 * @param supply_voltage_mv:	Pointer to int that will contain MCU supply voltage in mV.
 * @return:						None.
 */
void ADC1_GetMcuVoltage(unsigned int* supply_voltage_mv) {
	// Set sampling time (see p.89 of STM32L031x4/6 datasheet).
	ADC1 -> SMPR &= ~(0b111 << 0); // Reset bits 0-2.
	ADC1 -> SMPR |= (0b110 << 0); // Sampling time for internal voltage reference is 10us typical, 79.5*(1/ADCCLK) = 9.94us for SYSCLK = 16MHz;
	// Read raw supply voltage.
	unsigned int raw_supply_voltage_12bits = 0;
	ADC1_SingleConversion(ADC_CHANNEL_VREFINT, &raw_supply_voltage_12bits);
	// Compute supply voltage according to MCU factory calibration (see p.301 of RM0377 datasheet).
	adc_ctx.adc_mcu_supply_voltage_mv = (VREFINT_VCC_CALIB_MV * VREFINT_CAL) / (raw_supply_voltage_12bits);
	(*supply_voltage_mv) = adc_ctx.adc_mcu_supply_voltage_mv;
}

/* COMPUTE MCU TEMPERATURE WITH INTERNAL SENSOR.
 * @param mcu_temperature_degrees:	Pointer to byte that will contain MCU temperature in degrees.
 * @return:	None.
 */
void ADC1_GetMcuTemperatureComp1(unsigned char* mcu_temperature_degrees) {
	// Set sampling time (see p.88 of STM32L031x4/6 datasheet).
	ADC1 -> SMPR |= (0b111 << 0); // Sampling time for temperature sensor must be greater than 10us, 160.5*(1/ADCCLK) = 20us for ADCCLK = SYSCLK/2 = 8MHz;
	// Read raw temperature.
	int raw_temp_sensor_12bits = 0;
	ADC1_SingleConversion(ADC_CHANNEL_TEMPERATURE_SENSOR, &raw_temp_sensor_12bits);
	// Compute temperature according to MCU factory calibration (see p.301 and p.847 of RM0377 datasheet).
	int raw_temp_calib_mv = (raw_temp_sensor_12bits * adc_ctx.adc_mcu_supply_voltage_mv) / (TS_VCC_CALIB_MV) - TS_CAL1; // Equivalent raw measure for calibration power supply (VCC_CALIB).
	int temp_calib_degrees = raw_temp_calib_mv * ((int)(TS_CAL2_TEMP-TS_CAL1_TEMP));
	temp_calib_degrees = (temp_calib_degrees) / ((int)(TS_CAL2 - TS_CAL1));
	temp_calib_degrees += TS_CAL1_TEMP;
	// Convert to 1-complement value.
	(*mcu_temperature_degrees) = 0;
	if (temp_calib_degrees < 0) {
		(*mcu_temperature_degrees) |= 0x80;
		unsigned char temperature_abs = (-1) * (temp_calib_degrees);
		(*mcu_temperature_degrees) |= (temperature_abs & 0x7F);
	}
	else {
		(*mcu_temperature_degrees) = (temp_calib_degrees & 0x7F);
	}
}
