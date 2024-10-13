/*
 * st7066u_driver_flags.h
 *
 *  Created on: 11 oct. 2024
 *      Author: Ludo
 */

#ifndef __ST7066U_DRIVER_FLAGS_H__
#define __ST7066U_DRIVER_FLAGS_H__

#include "lptim.h"

/*** ST7066U driver compilation flags ***/

#define ST7066U_DRIVER_GPIO_ERROR_BASE_LAST     0
#define ST7066U_DRIVER_DELAY_ERROR_BASE_LAST    LPTIM_ERROR_BASE_LAST

#define ST7066U_DRIVER_SCREEN_WIDTH             8
#define ST7066U_DRIVER_SCREEN_HEIGHT            2

#endif /* __ST7066U_DRIVER_FLAGS_H__ */
