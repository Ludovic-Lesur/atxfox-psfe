/*
 * hmi.c
 *
 *  Created on: 12 oct. 2024
 *      Author: Ludo
 */

#include "hmi.h"

#include "error.h"
#include "st7066u.h"
#include "string.h"
#include "types.h"

/*** HMI functions ***/

/*******************************************************************/
HMI_status_t HMI_init(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    // Init LCD driver.
    st7066u_status = ST7066U_init();
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_de_init(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    // Release LCD driver.
    st7066u_status = ST7066U_de_init();
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_clear(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    // Release LCD driver.
    st7066u_status = ST7066U_clear();
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_print_string(uint8_t row, uint8_t column, char_t* str) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    // Release LCD driver.
    st7066u_status = ST7066U_print_string(row, column, str);
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_print_value(uint8_t row, int32_t value, uint8_t divider_exponent, char_t* unit) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    char_t lcd_string[ST7066U_DRIVER_SCREEN_WIDTH] = {STRING_CHAR_NULL};
    uint8_t number_of_digits = 0;
    uint32_t str_size = 0;
    // Check if unit is provided.
    if (unit != NULL) {
        // Get unit size.
        string_status = STRING_get_size(unit, &str_size);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        // Check size.
        if (str_size > (ST7066U_DRIVER_SCREEN_WIDTH >> 1)) {
            status = HMI_ERROR_UNIT_SIZE_OVERFLOW;
            goto errors;
        }
        // Add space.
        str_size++;
    }
    number_of_digits = (ST7066U_DRIVER_SCREEN_WIDTH - str_size);
    // Convert to floating point representation.
    string_status = STRING_integer_to_floating_decimal_string(value, divider_exponent, number_of_digits, lcd_string);
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    // Append unit if needed.
    if (unit != NULL) {
        // Add space.
        lcd_string[number_of_digits] = STRING_CHAR_SPACE;
        str_size = (number_of_digits + 1);
        // Add unit.
        string_status = STRING_append_string(lcd_string, ST7066U_DRIVER_SCREEN_WIDTH, unit, &str_size);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
    }
    // Print value.
    status = HMI_print_string(row, 0, lcd_string);
    if (status != HMI_SUCCESS) goto errors;
errors:
    return status;
}
