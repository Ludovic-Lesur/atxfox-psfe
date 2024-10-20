/*
 * trcs_driver_flags.h
 *
 *  Created on: 17 oct. 2024
 *      Author: Ludo
 */

#ifndef __TRCS_DRIVER_FLAGS_H__
#define __TRCS_DRIVER_FLAGS_H__

#include "adc.h"
#include "tim.h"

/*** TRCS driver compilation flags ***/

#define TRCS_DRIVER_GPIO_ERROR_BASE_LAST    0
#define TRCS_DRIVER_TIMER_ERROR_BASE_LAST   TIM_ERROR_BASE_LAST
#define TRCS_DRIVER_ADC_ERROR_BASE_LAST     ADC_ERROR_BASE_LAST

#define TRCS_DRIVER_ADC_RANGE_MV            3300

#endif /* __TRCS_DRIVER_FLAGS_H__ */
