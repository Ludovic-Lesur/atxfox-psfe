/*
 * tim.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef __TIM_H__
#define __TIM_H__

#include "types.h"

/*** TIM structures ***/

typedef enum {
	TIM_SUCCESS = 0,
	TIM_ERROR_INTERRUPT_TIMEOUT,
	TIM_ERROR_BASE_LAST = 0x0100
} TIM_status_t;

/*** TIM functions ***/

void TIM21_init(void);
TIM_status_t TIM21_get_lsi_frequency(uint32_t* lsi_frequency_hz);
void TIM21_disable(void);

void TIM2_init(uint32_t period_ms);
void TIM2_start(void);

void TIM22_init(uint32_t period_ms);
void TIM22_start(void);
void TIM22_stop(void);

#define TIM21_status_check(error_base) { if (tim21_status != TIM_SUCCESS) { status = error_base + tim21_status; goto errors; }}
#define TIM21_error_check() { ERROR_status_check(tim21_status, TIM_SUCCESS, ERROR_BASE_TIM21); }
#define TIM21_error_check_print() { ERROR_status_check_print(tim21_status, TIM_SUCCESS, ERROR_BASE_TIM21); }

#endif /* __TIM_H__ */
