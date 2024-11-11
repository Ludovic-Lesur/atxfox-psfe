/*
 * td1208_driver_flags.h
 *
 *  Created on: 14 oct. 2024
 *      Author: Ludo
 */

#ifndef __TD1208_DRIVER_FLAGS_H__
#define __TD1208_DRIVER_FLAGS_H__

#include "lptim.h"
#include "psfe_flags.h"
#include "usart.h"

/*** TD1208 driver compilation flags ***/

#define TD1208_DRIVER_UART_ERROR_BASE_LAST      USART_ERROR_BASE_LAST
#define TD1208_DRIVER_DELAY_ERROR_BASE_LAST     LPTIM_ERROR_BASE_LAST

#ifndef PSFE_SIGFOX_MONITORING
#define TD1208_DRIVER_DISABLE
#endif

#endif /* __TD1208_DRIVER_FLAGS_H__ */
