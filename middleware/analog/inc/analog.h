/*
 * analog.h
 *
 *  Created on: 09 oct. 2024
 *      Author: Ludo
 */

#ifndef __ANALOG_H__
#define __ANALOG_H__

#include "adc.h"
#include "tim.h"
#include "trcs.h"
#include "types.h"

/*** ANALOG structures ***/

/*!******************************************************************
 * \enum ANALOG_status_t
 * \brief ANALOG driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    ANALOG_SUCCESS = 0,
    ANALOG_ERROR_NULL_PARAMETER,
    ANALOG_ERROR_CHANNEL,
    ANALOG_ERROR_CALIBRATION_MISSING,
    // Low level drivers errors.
    ANALOG_ERROR_BASE_ADC = 0x0100,
    ANALOG_ERROR_BASE_TIM = (ANALOG_ERROR_BASE_ADC + ADC_ERROR_BASE_LAST),
    ANALOG_ERROR_BASE_TRCS = (ANALOG_ERROR_BASE_TIM + TIM_ERROR_BASE_LAST),
    // Last base value.
    ANALOG_ERROR_BASE_LAST = (ANALOG_ERROR_BASE_TRCS + TRCS_ERROR_BASE_LAST)
} ANALOG_status_t;

/*!******************************************************************
 * \enum ANALOG_channel_t
 * \brief ANALOG channels list.
 *******************************************************************/
typedef enum {
    ANALOG_CHANNEL_VMCU_MV = 0,
    ANALOG_CHANNEL_TMCU_DEGREES,
    ANALOG_CHANNEL_VOUT_MV,
    ANALOG_CHANNEL_IOUT_MV,
    ANALOG_CHANNEL_IOUT_UA,
    ANALOG_CHANNEL_LAST
} ANALOG_channel_t;

/*!******************************************************************
 * \enum ANALOG_iout_range_t
 * \brief Current measurement ranges list.
 *******************************************************************/
typedef enum {
    ANALOG_IOUT_RANGE_NONE = 0,
    ANALOG_IOUT_RANGE_LOW,
    ANALOG_IOUT_RANGE_MIDDLE,
    ANALOG_IOUT_RANGE_HIGH,
    ANALOG_IOUT_RANGE_BYPASS,
    ANALOG_IOUT_RANGE_LAST
} ANALOG_iout_range_t;

/*** ANALOG functions ***/

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_init(void)
 * \brief Init analog measurements driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_init(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_de_init(void)
 * \brief Release analog measurements driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_de_init(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_start_trcs(void)
 * \brief Start output current measurements.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_start_trcs(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_stop_trcs(void)
 * \brief Stop output current measurement.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_stop_trcs(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_process(void)
 * \brief Process ANALOG driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_process(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_read_channel(ANALOG_channel_t channel, int32_t* analog_data)
 * \brief Get analog channel data.
 * \param[in]   channel: Channel to read.
 * \param[out]  analog_data: Pointer to integer that will contain the result.
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_read_channel(ANALOG_channel_t channel, int32_t* analog_data);

/*!******************************************************************
 * \fn uint8_t ANALOG_get_bypass_switch_state(void)
 * \brief Get the bypass switch state.
 * \param[in]   none
 * \param[out]  none
 * \retval      Bypass switch state.
 *******************************************************************/
uint8_t ANALOG_get_bypass_switch_state(void);

/*!******************************************************************
 * \fn ANALOG_iout_range_t ANALOG_get_iout_range(void)
 * \brief Get the current measurement range.
 * \param[in]   none
 * \param[out]  none
 * \retval      Active current measurement range.
 *******************************************************************/
ANALOG_iout_range_t ANALOG_get_iout_range(void);

/*******************************************************************/
#define ANALOG_exit_error(base) { ERROR_check_exit(analog_status, ANALOG_SUCCESS, base) }

/*******************************************************************/
#define ANALOG_stack_error(base) { ERROR_check_stack(analog_status, ANALOG_SUCCESS, base) }

/*******************************************************************/
#define ANALOG_stack_exit_error(base, code) { ERROR_check_stack_exit(analog_status, ANALOG_SUCCESS, base, code) }

#endif /* __ANALOG_H__ */
