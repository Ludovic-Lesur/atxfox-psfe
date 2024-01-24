/*
 * trcs.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
 */

#ifndef __TRCS_H__
#define __TRCS_H__

#include "adc.h"
#include "exti.h"
#include "math.h"

/*** TRCS structures ***/

/*!******************************************************************
 * \enum TRCS_status_t
 * \brief TRCS driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	TRCS_SUCCESS = 0,
	TRCS_ERROR_NULL_PARAMETER,
	TRCS_ERROR_OVERFLOW,
	// Low level drivers errors.
	TRCS_ERROR_BASE_ADC1 = 0x0100,
	TRCS_ERROR_BASE_MATH = (TRCS_ERROR_BASE_ADC1 + ADC_ERROR_BASE_LAST),
	// Last base value.
	TRCS_ERROR_BASE_LAST = (TRCS_ERROR_BASE_MATH + MATH_ERROR_BASE_LAST)
} TRCS_status_t;

/*!******************************************************************
 * \enum TRCS_range_t
 * \brief TRCS current measurement ranges.
 *******************************************************************/
typedef enum {
	TRCS_RANGE_LOW = 0,
	TRCS_RANGE_MIDDLE,
	TRCS_RANGE_HIGH,
	TRCS_RANGE_NONE,
	TRCS_RANGE_LAST
} TRCS_range_t;

/*** TRCS functions ***/

/*!******************************************************************
 * \fn void TRCS_init(void)
 * \brief Init TRCS driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TRCS_init(void);

/*!******************************************************************
 * \fn void TRCS_de_init(void)
 * \brief Turn TRCS board off.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TRCS_de_init(void);

/*!******************************************************************
 * \fn TRCS_status_t TRCS_process(uint32_t process_period_ms)
 * \brief Process TRCS driver.
 * \param[in]  	process_period_ms: Function call period in milliseconds.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TRCS_status_t TRCS_process(uint32_t process_period_ms);

/*!******************************************************************
 * \fn uint8_t TRCS_get_bypass_switch_state(void)
 * \brief Get TRCS bypass switch state.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Bypass switch state.
 *******************************************************************/
uint8_t TRCS_get_bypass_switch_state(void);

/*!******************************************************************
 * \fn TRCS_status_t TRCS_get_range(volatile TRCS_range_t* range)
 * \brief Get TRCS current range.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		TRCS range value.
 *******************************************************************/
TRCS_range_t TRCS_get_range(void);

/*!******************************************************************
 * \fn TRCS_status_t TRCS_get_iout(volatile uint32_t* iout_ua)
 * \brief Get TRCS measured output current.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Measured output current in uA.
 *******************************************************************/
uint32_t TRCS_get_iout(void);

/*******************************************************************/
#define TRCS_exit_error(error_base) { if (trcs_status != TRCS_SUCCESS) { status = error_base + trcs_status; goto errors; } }

/*******************************************************************/
#define TRCS_stack_error(void) { if (trcs_status != TRCS_SUCCESS) { ERROR_stack_add(ERROR_BASE_TRCS + trcs_status); } }

/*******************************************************************/
#define TRCS_stack_exit_error(error_code) { if (trcs_status != TRCS_SUCCESS) { ERROR_stack_add(ERROR_BASE_TRCS + trcs_status); status = error_code; goto errors; } }

#endif /* __TRCS_H__ */
