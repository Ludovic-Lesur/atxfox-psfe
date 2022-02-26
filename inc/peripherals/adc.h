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
	ADC_DATA_IDX_LAST
} ADC_data_index_t;

/*** ADC functions ***/

void ADC1_init(void);
void ADC1_perform_measurements(void);
void ADC1_get_data(ADC_data_index_t data_idx, volatile unsigned int* data);
void ADC1_get_tmcu_comp2(volatile signed char* tmcu_degrees);
void ADC1_get_tmcu_comp1(volatile unsigned char* tmcu_degrees);

#endif /* ADC_H */
