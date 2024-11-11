/*
 * serial.h
 *
 *  Created on: 19 oct. 2024
 *      Author: Ludo
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "analog.h"
#include "psfe_flags.h"
#include "terminal.h"
#include "types.h"

/*** SERIAL structures ***/

/*!******************************************************************
 * \enum SERIAL_status_t
 * \brief SERIAL driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    SERIAL_SUCCESS = 0,
    // Low level drivers errors.
    SERIAL_ERROR_BASE_TERMINAL = 0x0100,
    SERIAL_ERROR_BASE_ANALOG = (SERIAL_ERROR_BASE_TERMINAL + TERMINAL_ERROR_BASE_LAST),
    // Last base value.
    SERIAL_ERROR_BASE_LAST = (SERIAL_ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST)
} SERIAL_status_t;

#ifdef PSFE_SERIAL_MONITORING

/*** SERIAL functions ***/

/*!******************************************************************
 * \fn SERIAL_status_t SERIAL_init(void)
 * \brief Init serial monitoring driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SERIAL_status_t SERIAL_init(void);

/*!******************************************************************
 * \fn SERIAL_status_t SERIAL_de_init(void)
 * \brief Release serial monitoring driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SERIAL_status_t SERIAL_de_init(void);

/*!******************************************************************
 * \fn SERIAL_status_t SERIAL_start(void)
 * \brief Start serial monitoring monitoring.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SERIAL_status_t SERIAL_start(void);

/*!******************************************************************
 * \fn SERIAL_status_t SERIAL_stop(void)
 * \brief Stop serial monitoring monitoring.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SERIAL_status_t SERIAL_stop(void);

/*!******************************************************************
 * \fn SERIAL_status_t SERIAL_process(void)
 * \brief Process SERIAL driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
SERIAL_status_t SERIAL_process(void);

/*******************************************************************/
#define SERIAL_exit_error(base) { ERROR_check_exit(serial_status, SERIAL_SUCCESS, base) }

/*******************************************************************/
#define SERIAL_stack_error(base) { ERROR_check_stack(serial_status, SERIAL_SUCCESS, base) }

/*******************************************************************/
#define SERIAL_stack_exit_error(base, code) { ERROR_check_stack_exit(serial_status, SERIAL_SUCCESS, base, code) }

#endif /*** PSFE_SERIAL_MONITORING ***/

#endif /* __SERIAL_H__ */
