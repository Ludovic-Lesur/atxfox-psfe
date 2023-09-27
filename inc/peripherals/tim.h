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

/*!******************************************************************
 * \enum TIM_status_t
 * \brief TIM driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	TIM_SUCCESS = 0,
	TIM_ERROR_NULL_PARAMETER,
	TIM_ERROR_INTERRUPT_TIMEOUT,
	// Last base value.
	TIM_ERROR_BASE_LAST = 0x0100
} TIM_status_t;

/*!******************************************************************
 * \fn TIM_completion_irq_cb_t
 * \brief TIM completion callback.
 *******************************************************************/
typedef void (*TIM_completion_irq_cb_t)(void);

/*** TIM functions ***/

/*!******************************************************************
 * \fn void TIM2_init(void)
 * \brief Init TIM2 peripheral for ADC sampling.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM2_init(uint32_t period_ms, TIM_completion_irq_cb_t irq_callback);

/*!******************************************************************
 * \fn void TIM2_de_init(void)
 * \brief Release TIM2 peripheral.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM2_de_init(void);

/*!******************************************************************
 * \fn void TIM2_start(void)
 * \brief Start TIM2.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM2_start(void);

/*!******************************************************************
 * \fn void TIM2_stop(void)
 * \brief Stop TIM2.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM2_stop(void);

/*!******************************************************************
 * \fn void TIM21_init(void)
 * \brief Init TIM21 peripheral for LSI frequency measurement.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM21_init(void);

/*!******************************************************************
 * \fn void TIM21_de_init(void)
 * \brief Release TIM21 peripheral.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM21_de_init(void);

/*!******************************************************************
 * \fn TIM_status_t TIM21_measure_lsi_frequency(uint32_t* lsi_frequency_hz)
 * \brief Compute effective LSI clock frequency with HSI accuracy.
 * \param[in]  	none
 * \param[out] 	lsi_frequency_hz: Effective LSI clock frequency in Hz.
 * \retval		Function execution status.
 *******************************************************************/
TIM_status_t TIM21_measure_lsi_frequency(uint32_t* lsi_frequency_hz);

/*******************************************************************/
#define TIM2_exit_error(error_base) { if (tim2_status != TIM_SUCCESS) { status = (error_base + tim2_status); goto errors; } }

/*******************************************************************/
#define TIM2_stack_error(void) { if (tim2_status != TIM_SUCCESS) { ERROR_stack_add(ERROR_BASE_TIM2 + tim2_status); } }

/*******************************************************************/
#define TIM2_stack_exit_error(error_code) { if (tim2_status != TIM_SUCCESS) { ERROR_stack_add(ERROR_BASE_TIM2 + tim2_status); status = error_code; goto errors; } }

/*******************************************************************/
#define TIM21_exit_error(error_base) { if (tim21_status != TIM_SUCCESS) { status = (error_base + tim21_status); goto errors; } }

/*******************************************************************/
#define TIM21_stack_error(void) { if (tim21_status != TIM_SUCCESS) { ERROR_stack_add(ERROR_BASE_TIM21 + tim21_status); } }

/*******************************************************************/
#define TIM21_stack_exit_error(error_code) { if (tim21_status != TIM_SUCCESS) { ERROR_stack_add(ERROR_BASE_TIM21 + tim21_status); status = error_code; goto errors; } }

#endif /* __TIM_H__ */
