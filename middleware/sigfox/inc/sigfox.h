/*
 * sigfox.h
 *
 *  Created on: 19 oct. 2024
 *      Author: Ludo
 */

#ifndef __SIGFOX_H__
#define __SIGFOX_H__

#include "analog.h"
#include "math.h"
#include "psfe_flags.h"
#include "td1208.h"

/*** SIGFOX structures ***/

/*!******************************************************************
 * \enum SIGFOX_status_t
 * \brief SIGFOX driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    SIGFOX_SUCCESS = 0,
    SIGFOX_ERROR_NULL_PARAMETER,
    SIGFOX_ERROR_EP_ID_NOT_READ,
    // Low level drivers errors.
    SIGFOX_ERROR_BASE_TD1208 = 0x0100,
    SIGFOX_ERROR_BASE_ANALOG = (SIGFOX_ERROR_BASE_TD1208 + TD1208_ERROR_BASE_LAST),
    SIGFOX_ERROR_BASE_MATH = (SIGFOX_ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST),
    // Last base value.
    SIGFOX_ERROR_BASE_LAST = (SIGFOX_ERROR_BASE_MATH + MATH_ERROR_BASE_LAST)
} SIGFOX_status_t;

#ifdef PSFE_SIGFOX_MONITORING

/*** SIGFOX functions ***/

/*!******************************************************************
 * \fn SIGFOX_status_t SIGFOX_init(void)
 * \brief Init Sigfox driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_status_t SIGFOX_init(void);

/*!******************************************************************
 * \fn SIGFOX_status_t SIGFOX_de_init(void)
 * \brief Release Sigfox driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_status_t SIGFOX_de_init(void);

/*!******************************************************************
 * \fn SIGFOX_status_t SIGFOX_start(void)
 * \brief Start Sigfox monitoring.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_status_t SIGFOX_start(void);

/*!******************************************************************
 * \fn SIGFOX_status_t SIGFOX_stop(void)
 * \brief Stop Sigfox monitoring.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_status_t SIGFOX_stop(void);

/*!******************************************************************
 * \fn SIGFOX_status_t SIGFOX_process(void)
 * \brief Process SIGFOX driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_status_t SIGFOX_process(void);

/*!******************************************************************
 * \fn SIGFOX_status_t SIGFOX_get_ep_id(uint8_t* sigfox_ep_id)
 * \brief Get Sigfox EP ID.
 * \param[in]   none
 * \param[out]  sigfox_ep_id: Pointer to byte array that will contain the Sigfox EP ID.
 * \retval      Function execution status.
 *******************************************************************/
SIGFOX_status_t SIGFOX_get_ep_id(uint8_t* sigfox_ep_id);

/*******************************************************************/
#define SIGFOX_exit_error(base) { ERROR_check_exit(sigfox_status, SIGFOX_SUCCESS, base) }

/*******************************************************************/
#define SIGFOX_stack_error(base) { ERROR_check_stack(sigfox_status, SIGFOX_SUCCESS, base) }

/*******************************************************************/
#define SIGFOX_stack_exit_error(base, code) { ERROR_check_stack_exit(sigfox_status, SIGFOX_SUCCESS, base, code) }

#endif /*** PSFE_SIGFOX_MONITORING ***/

#endif /* __SIGFOX_H__ */
