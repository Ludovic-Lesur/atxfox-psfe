/*
 * hmi.h
 *
 *  Created on: 12 oct. 2024
 *      Author: Ludo
 */

#ifndef __HMI_H__
#define __HMI_H__

#include "st7066u.h"
#include "string.h"
#include "types.h"

/*** HMI structures ***/

/*!******************************************************************
 * \enum HMI_status_t
 * \brief HMI driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    HMI_SUCCESS = 0,
    HMI_ERROR_UNIT_SIZE_OVERFLOW,
    // Low level drivers errors.
    HMI_ERROR_BASE_STRING = 0x0100,
    HMI_ERROR_BASE_ST7066U = (HMI_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
    // Last base value.
    HMI_ERROR_BASE_LAST = (HMI_ERROR_BASE_ST7066U + ST7066U_ERROR_BASE_LAST)
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
 * \fn HMI_status_t HMI_clear(void)
 * \brief Clear LCD screen.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_clear(void);

/*!******************************************************************
 * \fn HMI_status_t HMI_print_string(uint8_t row, uint8_t column, char_t* str)
 * \brief Print a string on LCD screen.
 * \param[in]   row: Row where to print.
 * \param[in]   column: Column where to print.
 * \param[in]   str: String to print.
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_print_string(uint8_t row, uint8_t column, char_t* str);

/*!******************************************************************
 * \fn HMI_status_t HMI_print_value(uint8_t row, int32_t value, uint8_t divider_exponent, char_t* unit)
 * \brief Print a value with unit on LCD screen.
 * \param[in]   row: Row where to print.
 * \param[in]   value: Value to print.
 * \param[in]   divider_exponent: Input value will be divider by 10^(divider_exponent) before being represented.
 * \param[in]   unit: Unit to display after value (can be NULL).
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_print_value(uint8_t row, int32_t value, uint8_t divider_exponent, char_t* unit);

/*******************************************************************/
#define HMI_exit_error(base) { ERROR_check_exit(hmi_status, HMI_SUCCESS, base) }

/*******************************************************************/
#define HMI_stack_error(base) { ERROR_check_stack(hmi_status, HMI_SUCCESS, base) }

/*******************************************************************/
#define HMI_stack_exit_error(base, code) { ERROR_check_stack_exit(hmi_status, HMI_SUCCESS, base, code) }

#endif /* __HMI_H__ */
