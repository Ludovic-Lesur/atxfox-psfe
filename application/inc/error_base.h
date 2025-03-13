/*
 * error_base.h
 *
 *  Created on: 12 mar. 2022
 *      Author: Ludo
 */

#ifndef __ERROR_BASE_H__
#define __ERROR_BASE_H__

// Peripherals.
#include "iwdg.h"
#include "lptim.h"
#include "rcc.h"
#include "rtc.h"
// Middleware.
#include "analog.h"
#include "hmi.h"
#include "serial.h"
#include "sigfox.h"

/*** ERROR structures ***/

/*!******************************************************************
 * \enum ERROR_base_t
 * \brief Board error bases.
 *******************************************************************/
typedef enum {
    SUCCESS = 0,
    // Peripherals.
    ERROR_BASE_IWDG = ERROR_BASE_STEP,
    ERROR_BASE_LPTIM = (ERROR_BASE_IWDG + IWDG_ERROR_BASE_LAST),
    ERROR_BASE_RCC = (ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
    ERROR_BASE_RTC = (ERROR_BASE_RCC + RCC_ERROR_BASE_LAST),
    // Middleware.
    ERROR_BASE_ANALOG = (ERROR_BASE_RTC + RTC_ERROR_BASE_LAST),
    ERROR_BASE_HMI = (ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST),
    ERROR_BASE_SERIAL = (ERROR_BASE_HMI + HMI_ERROR_BASE_LAST),
    ERROR_BASE_SIGFOX = (ERROR_BASE_SERIAL + SERIAL_ERROR_BASE_LAST),
    // Last base value.
    ERROR_BASE_LAST = (ERROR_BASE_SIGFOX + SIGFOX_ERROR_BASE_LAST),
} ERROR_base_t;

#endif /* __ERROR_BASE_H__ */
