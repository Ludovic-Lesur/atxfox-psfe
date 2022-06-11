/*
 * trcs.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
 */

#ifndef __TRCS_H__
#define __TRCS_H__

#include "adc.h"

/*** TRCS macros ***/

#define TRCS_IOUT_ERROR_VALUE	0xFFFFFFFF

/*** TRCS structures ***/

typedef enum {
	TRCS_SUCCESS = 0,
	TRCS_ERROR_OVERFLOW,
	TRCS_ERROR_BASE_ADC = 0x0100,
	TRCS_ERROR_BASE_LAST = (TRCS_ERROR_BASE_ADC + ADC_ERROR_BASE_LAST)
} TRCS_status_t;

typedef enum {
	TRCS_RANGE_LOW = 0,
	TRCS_RANGE_MIDDLE,
	TRCS_RANGE_HIGH,
	TRCS_RANGE_NONE,
	TRCS_RANGE_LAST
} TRCS_range_t;

/*** TRCS functions ***/

void TRCS_init(void);
TRCS_status_t TRCS_task(void);
void TRCS_set_bypass_flag(unsigned char bypass_state);
void TRCS_get_range(volatile TRCS_range_t* range);
void TRCS_get_iout(volatile unsigned int* iout_ua);
void TRCS_off(void);

#endif /* __TRCS_H__ */
