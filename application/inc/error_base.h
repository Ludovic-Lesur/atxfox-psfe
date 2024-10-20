/*
 * error_base.h
 *
 *  Created on: 12 mar. 2022
 *      Author: Ludo
 */

#ifndef __ERROR_BASE_H__
#define __ERROR_BASE_H__

// Peripherals.
#include "adc.h"
#include "flash.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "rcc.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
// Utils.
#include "math.h"
#include "parser.h"
#include "string.h"
#include "terminal.h"
// Components.
#include "st7066u.h"
#include "td1208.h"
#include "trcs.h"
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
	ERROR_BASE_ADC = 0x0100,
	ERROR_BASE_FLASH = (ERROR_BASE_ADC + ADC_ERROR_BASE_LAST),
	ERROR_BASE_IWDG = (ERROR_BASE_FLASH + FLASH_ERROR_BASE_LAST),
	ERROR_BASE_LPTIM = (ERROR_BASE_IWDG + IWDG_ERROR_BASE_LAST),
	ERROR_BASE_LPUART = (ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	ERROR_BASE_RCC = (ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
	ERROR_BASE_RTC = (ERROR_BASE_RCC + RCC_ERROR_BASE_LAST),
	ERROR_BASE_TIM_ADC_SAMPLING = (ERROR_BASE_RTC + RTC_ERROR_BASE_LAST),
	ERROR_BASE_TIM21 = (ERROR_BASE_TIM_ADC_SAMPLING + TIM_ERROR_BASE_LAST),
	ERROR_BASE_USART_TD1208 = (ERROR_BASE_TIM21 + TIM_ERROR_BASE_LAST),
	// Utils.
	ERROR_BASE_MATH = (ERROR_BASE_USART_TD1208 + USART_ERROR_BASE_LAST),
	ERROR_BASE_PARSER = (ERROR_BASE_MATH + MATH_ERROR_BASE_LAST),
	ERROR_BASE_STRING = (ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST),
	ERROR_BASE_TERMINAL = (ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	// Components.
	ERROR_BASE_ST7066U = (ERROR_BASE_TERMINAL + TERMINAL_ERROR_BASE_LAST),
	ERROR_BASE_TD1208 = (ERROR_BASE_ST7066U + ST7066U_ERROR_BASE_LAST),
	ERROR_BASE_TRCS = (ERROR_BASE_TD1208 + TD1208_ERROR_BASE_LAST),
	// Middleware.
    ERROR_BASE_ANALOG = (ERROR_BASE_TRCS + TRCS_ERROR_BASE_LAST),
    ERROR_BASE_HMI = (ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST),
    ERROR_BASE_SERIAL = (ERROR_BASE_HMI + HMI_ERROR_BASE_LAST),
    ERROR_BASE_SIGFOX = (ERROR_BASE_SERIAL + SERIAL_ERROR_BASE_LAST),
	// Last base value.
	ERROR_BASE_LAST = (ERROR_BASE_SIGFOX + SIGFOX_ERROR_BASE_LAST)
} ERROR_base_t;

#endif /* __ERROR_BASE_H__ */
