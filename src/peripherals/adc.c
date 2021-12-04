/*
 * adc.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"

#include "adc_reg.h"
#include "filter.h"
#include "gpio.h"
#include "lptim.h"
#include "mapping.h"
#include "psfe.h"
#include "rcc_reg.h"

/*** ADC local macros ***/

#define ADC_TIMEOUT_COUNT					1000000
#define ADC_CHANNEL_REF191					9
#define ADC_CHANNEL_VOUT					8
#define ADC_CHANNEL_IOUT					0
#define ADC_CHANNEL_VREFINT					17
#define ADC_CHANNEL_TMCU					18

#define ADC_MEDIAN_FILTER_LENGTH			9
#define ADC_CENTER_AVERAGE_LENGTH			3

#define ADC_FULL_SCALE_12BITS				4095

#define ADC_VMCU_DEFAULT_MV					3400

/*** ADC local structures ***/

typedef struct {
	unsigned int adc_data[ADC_DATA_IDX_MAX];
	unsigned char adc_tmcu_degrees_comp1;
	signed char adc_tmcu_degrees_comp2;
} ADC_Context;

/*** ADC local global variables ***/

static ADC_Context adc_ctx;

/*** ADC local functions ***/

/* PERFORM A SINGLE ADC CONVERSION.
 * @param adc_channel:			Channel to convert.
 * @param adc_result_12bits:	Pointer to int that will contain ADC raw result on 12 bits.
 * @return:						None.
 */
static void ADC1_SingleConversion(unsigned char adc_channel, unsigned int* adc_result_12bits) {
	// Select input channel.
	ADC1 -> CHSELR &= 0xFFF80000; // Reset all bits.
	ADC1 -> CHSELR |= (0b1 << adc_channel);
	// Read raw supply voltage.
	ADC1 -> ISR |= 0x0000089F; // Clear all flags.
	ADC1 -> CR |= (0b1 << 2); // ADSTART='1'.
	unsigned int loop_count = 0;
	while (((ADC1 -> ISR) & (0b1 << 2)) == 0) {
		// Wait end of conversion ('EOC='1') or timeout.
		loop_count++;
		if (loop_count > ADC_TIMEOUT_COUNT) return;
	}
	(*adc_result_12bits) = (ADC1 -> DR);
}

/* PERFORM SEVERAL CONVERSIONS FOLLOWED BY A MEDIAN FIILTER.
 * @param adc_channel:			Channel to convert.
 * @param adc_result_12bits:	Pointer to int that will contain ADC filtered result on 12 bits.
 * @return:						None.
 */
static void ADC1_FilteredConversion(unsigned char adc_channel, unsigned int* adc_result_12bits) {
	// Perform all conversions.
	unsigned int adc_sample_buf[ADC_MEDIAN_FILTER_LENGTH] = {0x00};
	unsigned char idx = 0;
	for (idx=0 ; idx<ADC_MEDIAN_FILTER_LENGTH ; idx++) {
		ADC1_SingleConversion(adc_channel, &(adc_sample_buf[idx]));
	}
	// Apply median filter.
	(*adc_result_12bits) = FILTER_ComputeMedianFilter(adc_sample_buf, ADC_MEDIAN_FILTER_LENGTH, ADC_CENTER_AVERAGE_LENGTH);
}

/* UPDATE ADC CALIBRATION VALUE WITH EXTERNAL BANDGAP.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_Calibrate(void) {
	ADC1_FilteredConversion(ADC_CHANNEL_REF191, &adc_ctx.adc_data[ADC_DATA_IDX_REF191_12BITS]);
}

/* COMPUTE OUTPUT VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_ComputeVout(void) {
	// Get raw result.
	unsigned int vout_12bits = 0;
	ADC1_FilteredConversion(ADC_CHANNEL_VOUT, &vout_12bits);
	// Convert to mV using bandgap result.
	adc_ctx.adc_data[ADC_DATA_IDX_VOUT_MV] = (ADC_REF191_VOLTAGE_MV * vout_12bits * psfe_vout_voltage_divider_ratio[PSFE_BOARD_INDEX]) / (adc_ctx.adc_data[ADC_DATA_IDX_REF191_12BITS]);
}

/* COMPUTE OUTPUT VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_ComputeIout(void) {
	// Get raw result.
	ADC1_FilteredConversion(ADC_CHANNEL_IOUT, &adc_ctx.adc_data[ADC_DATA_IDX_IOUT_12BITS]);
}

/* COMPUTE MCU SUPPLY VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_ComputeVmcu(void) {
	// Retrieve supply voltage from bandgap result.
	adc_ctx.adc_data[ADC_DATA_IDX_VMCU_MV] = (ADC_REF191_VOLTAGE_MV * ADC_FULL_SCALE_12BITS) / (adc_ctx.adc_data[ADC_DATA_IDX_REF191_12BITS]);
}

/* COMPUTE MCU TEMPERATURE THANKS TO INTERNAL VOLTAGE REFERENCE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_ComputeTmcu(void) {
	// Read raw temperature.
	int raw_temp_sensor_12bits = 0;
	ADC1_FilteredConversion(ADC_CHANNEL_TMCU, &raw_temp_sensor_12bits);
	// Compute temperature according to MCU factory calibration (see p.301 and p.847 of RM0377 datasheet).
	int raw_temp_calib_mv = (raw_temp_sensor_12bits * adc_ctx.adc_data[ADC_DATA_IDX_VMCU_MV]) / (TS_VCC_CALIB_MV) - TS_CAL1; // Equivalent raw measure for calibration power supply (VCC_CALIB).
	int temp_calib_degrees = raw_temp_calib_mv * ((int)(TS_CAL2_TEMP-TS_CAL1_TEMP));
	temp_calib_degrees = (temp_calib_degrees) / ((int)(TS_CAL2 - TS_CAL1));
	adc_ctx.adc_tmcu_degrees_comp2 = temp_calib_degrees + TS_CAL1_TEMP;
	// Convert to 1-complement value.
	adc_ctx.adc_tmcu_degrees_comp1 = 0;
	if (adc_ctx.adc_tmcu_degrees_comp2 < 0) {
		adc_ctx.adc_tmcu_degrees_comp1 |= 0x80;
		unsigned char temperature_abs = (-1) * (adc_ctx.adc_tmcu_degrees_comp2);
		adc_ctx.adc_tmcu_degrees_comp1 |= (temperature_abs & 0x7F);
	}
	else {
		adc_ctx.adc_tmcu_degrees_comp1 = (adc_ctx.adc_tmcu_degrees_comp2 & 0x7F);
	}
}

/*** ADC functions ***/

/* INIT ADC1 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void ADC1_Init(void) {
	// Configure analog GPIOs */
	GPIO_Configure(&GPIO_BANDGAP, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_VOLTAGE_SENSE, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_CURRENT_SENSE, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Init context.
	adc_ctx.adc_data[ADC_DATA_IDX_REF191_12BITS] = 0;
	unsigned char data_idx = 0;
	for (data_idx=0 ; data_idx<ADC_DATA_IDX_MAX ; data_idx++) adc_ctx.adc_data[data_idx] = 0;
	adc_ctx.adc_data[ADC_DATA_IDX_VMCU_MV] = ADC_VMCU_DEFAULT_MV;
	adc_ctx.adc_tmcu_degrees_comp2 = 0;
	adc_ctx.adc_tmcu_degrees_comp1 = 0;
	// Enable peripheral clock.
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
	// Sampling time for temperature sensor must be greater than 10us, 160.5*(1/ADCCLK) = 20us for ADCCLK = SYSCLK/2 = 8MHz.
	ADC1 -> SMPR |= (0b111 << 0);
	// Enable ADC peripheral.
	ADC1 -> CR |= (0b1 << 0); // ADEN='1'.
	while (((ADC1 -> ISR) & (0b1 << 0)) == 0); // Wait for ADC to be ready (ADRDY='1').
	// Calibrate ADC for first time.
	ADC1_Calibrate();
}

/* PERFORM INTERNAL ADC MEASUREMENTS.
 * @param:	None.
 * @return:	None.
 */
void ADC1_PerformMeasurements(void) {
	// Calibrate with bandgap.
	ADC1_Calibrate();
	// Perform measurements.
	ADC1_ComputeVout();
	ADC1_ComputeIout();
	ADC1_ComputeVmcu();
	ADC1_ComputeTmcu();
}

/* GET ADC DATA.
 * @param adc_data_idx:		Index of the data to retrieve.
 * @param data:				Pointer that will contain ADC data.
 * @return:					None.
 */
void ADC1_GetData(ADC_DataIndex adc_data_idx, volatile unsigned int* data) {
	(*data) = adc_ctx.adc_data[adc_data_idx];
}

/* GET MCU TEMPERATURE.
 * @param tmcu_degrees:	Pointer to signed value that will contain MCU temperature in degrees (2-complement).
 * @return:				None.
 */
void ADC1_GetTmcuComp2(volatile signed char* tmcu_degrees) {
	(*tmcu_degrees) = adc_ctx.adc_tmcu_degrees_comp2;
}

/* GET MCU TEMPERATURE.
 * @param tmcu_degrees:	Pointer to unsigned value that will contain MCU temperature in degrees (1-complement).
 * @return:				None.
 */
void ADC1_GetTmcuComp1(volatile unsigned char* tmcu_degrees) {
	(*tmcu_degrees) = adc_ctx.adc_tmcu_degrees_comp1;
}
