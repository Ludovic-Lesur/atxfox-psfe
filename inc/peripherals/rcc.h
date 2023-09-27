/*
 * rcc.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef __RCC_H__
#define __RCC_H__

#include "flash.h"
#include "types.h"

/*** RCC macros ***/

#define RCC_LSI_FREQUENCY_HZ		38000
#define RCC_HSI_FREQUENCY_KHZ		16000
#define RCC_SYSCLK_FREQUENCY_KHZ	16000

/*** RCC structures ***/

/*!******************************************************************
 * \enum RCC_status_t
 * \brief RCC driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	RCC_SUCCESS = 0,
	RCC_ERROR_NULL_PARAMETER,
	RCC_ERROR_HSI_READY,
	RCC_ERROR_HSI_SWITCH,
	RCC_ERROR_LSI_MEASUREMENT,
	// Low level drivers errors.
	RCC_ERROR_BASE_FLASH = 0x0100,
	// Last base value.
	RCC_ERROR_BASE_LAST = (RCC_ERROR_BASE_FLASH + FLASH_ERROR_BASE_LAST)
} RCC_status_t;

/*** RCC functions ***/

/*!******************************************************************
 * \fn void RCC_init(void)
 * \brief Init MCU default clock tree.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
RCC_status_t RCC_init(void);

/*!******************************************************************
 * \fn RCC_status_t RCC_switch_to_hsi(void)
 * \brief Switch system clock to 16MHz HSI.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
RCC_status_t RCC_switch_to_hsi(void);

/*!******************************************************************
 * \fn RCC_status_t RCC_measure_lsi_frequency(uint32_t* lsi_frequency_hz)
 * \brief Compute effective LSI clock frequency with HSI accuracy.
 * \param[in]  	none
 * \param[out] 	lsi_frequency_hz: Effective LSI clock frequency in Hz.
 * \retval		Function execution status.
 *******************************************************************/
RCC_status_t RCC_measure_lsi_frequency(uint32_t* lsi_frequency_hz);

/*******************************************************************/
#define RCC_exit_error(error_base) { if (rcc_status != RCC_SUCCESS) { status = (error_base + rcc_status); goto errors; } }

/*******************************************************************/
#define RCC_stack_error(void) { if (rcc_status != RCC_SUCCESS) { ERROR_stack_add(ERROR_BASE_RCC + rcc_status); } }

/*******************************************************************/
#define RCC_stack_exit_error(error_code) { if (rcc_status != RCC_SUCCESS) { ERROR_stack_add(ERROR_BASE_RCC + rcc_status); status = error_code; goto errors; } }

#endif /* __RCC_H__ */
