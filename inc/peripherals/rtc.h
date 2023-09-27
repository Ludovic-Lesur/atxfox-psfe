/*
 * rtc.h
 *
 *  Created on: 16 aug. 2020
 *      Author: Ludo
 */

#ifndef __RTC_H__
#define __RTC_H__

#include "types.h"

/*** RTC macros ***/

// RTC wake-up timer period. Value must be lower than the watchdog period (~27s).
#define RTC_WAKEUP_PERIOD_SECONDS	10

/*** RTC structures ***/

/*!******************************************************************
 * \enum RTC_status_t
 * \brief RTC driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	RTC_SUCCESS = 0,
	RTC_ERROR_NULL_PARAMETER,
	RTC_ERROR_INITIALIZATION_MODE,
	RTC_ERROR_WAKEUP_TIMER_REGISTER_ACCESS,
	// Last base value.
	RTC_ERROR_BASE_LAST = 0x0100
} RTC_status_t;

/*** RTC functions ***/

/*!******************************************************************
 * \fn RTC_status_t RTC_init(void)
 * \brief Init RTC peripheral.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
RTC_status_t RTC_init(void);

/*!******************************************************************
 * \fn volatile uint8_t RTC_get_wakeup_timer_flag(void)
 * \brief Read RTC wakeup timer interrupt flag.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		RTC wakeup timer flag.
 *******************************************************************/
volatile uint8_t RTC_get_wakeup_timer_flag(void);

/*!******************************************************************
 * \fn void RTC_clear_wakeup_timer_flag(void)
 * \brief Clear RTC wakeup timer interrupt flag.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void RTC_clear_wakeup_timer_flag(void);

/*!******************************************************************
 * \fn uint32_t RTC_get_time_seconds(void)
 * \brief Read MCU operating time in seconds.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Current time in seconds.
 *******************************************************************/
uint32_t RTC_get_time_seconds(void);

/*******************************************************************/
#define RTC_exit_error(error_base) { if (rtc_status != RTC_SUCCESS) { status = (error_base + rtc_status); goto errors; } }

/*******************************************************************/
#define RTC_stack_error(void) { if (rtc_status != RTC_SUCCESS) { ERROR_stack_add(ERROR_BASE_RTC + rtc_status); } }

/*******************************************************************/
#define RTC_stack_exit_error(error_code) { if (rtc_status != RTC_SUCCESS) { ERROR_stack_add(ERROR_BASE_RTC + rtc_status); status = error_code; goto errors; } }

#endif /* __RTC_H__ */
