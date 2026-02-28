/*
 * analog.h
 *
 *  Created on: 09 oct. 2024
 *      Author: Ludo
 */

#ifndef __ANALOG_H__
#define __ANALOG_H__

#include "adc.h"
#include "error.h"
#include "nvm.h"
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
    ANALOG_ERROR_BOARD_NUMBER,
    ANALOG_ERROR_CHANNEL,
    ANALOG_ERROR_CALIBRATION_MISSING,
    // Low level drivers errors.
    ANALOG_ERROR_BASE_ADC = ERROR_BASE_STEP,
    ANALOG_ERROR_BASE_NVM = (ANALOG_ERROR_BASE_ADC + ADC_ERROR_BASE_LAST),
    ANALOG_ERROR_BASE_TIM = (ANALOG_ERROR_BASE_NVM + NVM_ERROR_BASE_LAST),
    ANALOG_ERROR_BASE_TRCS = (ANALOG_ERROR_BASE_TIM + TIM_ERROR_BASE_LAST),
    // Last base value.
    ANALOG_ERROR_BASE_LAST = (ANALOG_ERROR_BASE_TRCS + TRCS_ERROR_BASE_LAST)
} ANALOG_status_t;

/*!******************************************************************
 * \enum ANALOG_channel_t
 * \brief ANALOG channels list.
 *******************************************************************/
typedef enum {
    ANALOG_CHANNEL_MCU_VOLTAGE_MV = 0,
    ANALOG_CHANNEL_MCU_TEMPERATURE_DEGREES,
    ANALOG_CHANNEL_OUTPUT_VOLTAGE_MV,
    ANALOG_CHANNEL_OUTPUT_CURRENT_MV,
    ANALOG_CHANNEL_OUTPUT_CURRENT_UA,
    ANALOG_CHANNEL_LAST
} ANALOG_channel_t;

/*!******************************************************************
 * \enum ANALOG_output_current_range_t
 * \brief Current measurement ranges list.
 *******************************************************************/
typedef enum {
    ANALOG_OUTPUT_CURRENT_RANGE_NONE = 0,
    ANALOG_OUTPUT_CURRENT_RANGE_LOW,
    ANALOG_OUTPUT_CURRENT_RANGE_MIDDLE,
    ANALOG_OUTPUT_CURRENT_RANGE_HIGH,
    ANALOG_OUTPUT_CURRENT_RANGE_BYPASS,
    ANALOG_OUTPUT_CURRENT_RANGE_LAST
} ANALOG_output_current_range_t;

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
 * \fn ANALOG_status_t ANALOG_get_bypass_switch_state(uint8_t* bypass_switch_state)
 * \brief Get the bypass switch state.
 * \param[in]   none
 * \param[out]  bypass_switch_state: Pointer to the bypass switch state.
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_get_bypass_switch_state(uint8_t* bypass_switch_state);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_get_output_current_range(ANALOG_output_current_range_t* output_current_range)
 * \brief Get the current measurement range.
 * \param[in]   none
 * \param[out]  output_current_range: Pointer to the active current measurement range.
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_get_output_current_range(ANALOG_output_current_range_t* output_current_range);

/*******************************************************************/
#define ANALOG_exit_error(base) { ERROR_check_exit(analog_status, ANALOG_SUCCESS, base) }

/*******************************************************************/
#define ANALOG_stack_error(base) { ERROR_check_stack(analog_status, ANALOG_SUCCESS, base) }

/*******************************************************************/
#define ANALOG_stack_exit_error(base, code) { ERROR_check_stack_exit(analog_status, ANALOG_SUCCESS, base, code) }

#endif /* __ANALOG_H__ */
