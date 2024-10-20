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
    // Analog measurements
    NVIC_PRIORITY_TRCS_TIMER = 1,
    NVIC_PRIORITY_ANALOG_TIMER = 2,
    // HMI.
    NVIC_PRIORITY_HMI_TIMER = 3,
    // Log interface
    NVIC_PRIORITY_SERIAL = 3,
} NVIC_priority_list_t;

#endif /* __NVIC_PRIORITY_H__ */
