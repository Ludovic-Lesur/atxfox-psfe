/*
 * nvic_priority.h
 *
 *  Created on: 06 oct. 2024
 *      Author: Ludo
 */

#ifndef __NVIC_PRIORITY_H__
#define __NVIC_PRIORITY_H__

/*!******************************************************************
 * \enum NVIC_priority_list_t
 * \brief NVIC interrupt priorities list.
 *******************************************************************/
typedef enum {
    // Common.
    NVIC_PRIORITY_CLOCK = 0,
    NVIC_PRIORITY_CLOCK_CALIBRATION = 1,
    NVIC_PRIORITY_DELAY = 2,
    NVIC_PRIORITY_RTC = 3,
    // TD1208.
    NVIC_PRIORITY_TD1208_UART = 0,
    // ADC sampling.
    NVIC_PRIORITY_ADC_SAMPLING_TIMER = 1,
    // TRCS bypass switch.
    NVIC_PRIORITY_TRCS_BYPASS = 2,
    // Log interface
    NVIC_PRIORITY_LOG = 3,
} NVIC_priority_list_t;

#endif /* __NVIC_PRIORITY_H__ */
