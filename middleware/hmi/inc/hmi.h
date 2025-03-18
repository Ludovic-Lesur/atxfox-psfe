/*
 * hmi.h
 *
 *  Created on: 12 oct. 2024
 *      Author: Ludo
 */

#ifndef __HMI_H__
#define __HMI_H__

#include "analog.h"
#include "error.h"
#include "sigfox.h"
#include "st7066u.h"
#include "strings.h"
#include "tim.h"
#include "types.h"

/*** HMI structures ***/

/*!******************************************************************
 * \enum HMI_status_t
 * \brief HMI driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    HMI_SUCCESS = 0,
    HMI_ERROR_STATE,
    HMI_ERROR_UNIT_SIZE_OVERFLOW,
    // Low level drivers errors.
    HMI_ERROR_BASE_TIM = ERROR_BASE_STEP,
    HMI_ERROR_BASE_STRING = (HMI_ERROR_BASE_TIM + TIM_ERROR_BASE_LAST),
    HMI_ERROR_BASE_ST7066U = (HMI_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
    HMI_ERROR_BASE_SIGFOX = (HMI_ERROR_BASE_ST7066U + ST7066U_ERROR_BASE_LAST),
    HMI_ERROR_BASE_ANALOG = (HMI_ERROR_BASE_SIGFOX + SIGFOX_ERROR_BASE_LAST),
    // Last base value.
    HMI_ERROR_BASE_LAST = (HMI_ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST)
} HMI_status_t;

/*** HMI functions ***/

/*!******************************************************************
 * \fn HMI_status_t HMI_init(void)
 * \brief Init HMI driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_init(void);

/*!******************************************************************
 * \fn HMI_status_t HMI_de_init(void)
 * \brief Release HMI driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_de_init(void);

/*!******************************************************************
 * \fn HMI_status_t HMI_start(void)
 * \brief Start HMI.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_start(void);

/*!******************************************************************
 * \fn HMI_status_t HMI_stop(void)
 * \brief Stop HMI.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_stop(void);

/*******************************************************************/
#define HMI_exit_error(base) { ERROR_check_exit(hmi_status, HMI_SUCCESS, base) }

/*******************************************************************/
#define HMI_stack_error(base) { ERROR_check_stack(hmi_status, HMI_SUCCESS, base) }

/*******************************************************************/
#define HMI_stack_exit_error(base, code) { ERROR_check_stack_exit(hmi_status, HMI_SUCCESS, base, code) }

#endif /* __HMI_H__ */
