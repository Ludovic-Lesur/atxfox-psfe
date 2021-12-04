/*
 * adc.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef ADC_H
#define ADC_H

/*** ADC macros ***/

#define ADC_REF191_VOLTAGE_MV	2048

/*** ADC structures ***/

typedef enum {
	ADC_DATA_IDX_VOUT_MV = 0,
	ADC_DATA_IDX_VMCU_MV,
	ADC_DATA_IDX_IOUT_12BITS,
	ADC_DATA_IDX_REF191_12BITS,
	ADC_DATA_IDX_MAX
} ADC_DataIndex;

/*** ADC functions ***/

void ADC1_Init(void);
void ADC1_PerformMeasurements(void);
void ADC1_GetData(ADC_DataIndex adc_data_idx, volatile unsigned int* data);
void ADC1_GetTmcuComp2(volatile signed char* tmcu_degrees);
void ADC1_GetTmcuComp1(volatile unsigned char* tmcu_degrees);

#endif /* ADC_H */
