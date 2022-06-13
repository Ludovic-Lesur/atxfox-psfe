/*
 * adc.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef __ADC_H__
#define __ADC_H__

#include "lptim.h"

/*** ADC macros ***/

#define ADC_REF191_VOLTAGE_MV	2048

/*** ADC structures ***/

typedef enum {
	ADC_SUCCESS = 0,
	ADC_ERROR_CALIBRATION,
	ADC_ERROR_DATA_INDEX,
	ADC_ERROR_TIMEOUT,
	ADC_ERROR_BASE_LPTIM = 0x0100,
	ADC_ERROR_BASE_LAST = (ADC_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST)
} ADC_status_t;

typedef enum {
	ADC_DATA_INDEX_VOUT_MV = 0,
	ADC_DATA_INDEX_VMCU_MV,
	ADC_DATA_INDEX_IOUT_12BITS,
	ADC_DATA_INDEX_REF191_12BITS,
	ADC_DATA_INDEX_LAST
} ADC_data_index_t;

/*** ADC functions ***/

ADC_status_t ADC1_init(void);
ADC_status_t ADC1_perform_measurements(void);
ADC_status_t ADC1_get_data(ADC_data_index_t data_idx, unsigned int* data);
void ADC1_get_tmcu(signed char* tmcu_degrees);

#define ADC1_status_check(error_base) { if (adc1_status != ADC_SUCCESS) { status = error_base + adc1_status; goto errors; }}
#define ADC1_error_check() { ERROR_status_check(adc1_status, ADC_SUCCESS, ERROR_BASE_ADC1); }
#define ADC1_error_check_print() { ERROR_status_check_print(adc1_status, ADC_SUCCESS, ERROR_BASE_ADC1); }

#endif /* __ADC_H__ */
