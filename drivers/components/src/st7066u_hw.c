/*
 * st7066u_hw.c
 *
 *  Created on: 12 oct. 2024
 *      Author: Ludo
 */

#include "st7066u_hw.h"

#ifndef ST7066U_DRIVER_DISABLE_FLAGS_FILE
#include "st7066u_driver_flags.h"
#endif
#include "gpio.h"
#include "gpio_mapping.h"
#include "gpio_registers.h"
#include "error.h"
#include "st7066u.h"
#include "types.h"

#ifndef ST7066U_DRIVER_DISABLE

/*** ST7066U HW local global variables ***/

static const GPIO_pin_t* ST7066_HW_GPIO[ST7066U_HW_GPIO_LAST] = { &GPIO_LCD_RS, NULL, &GPIO_LCD_E };

/*** ST7066U HW functions ***/

/*******************************************************************/
ST7066U_status_t ST7066U_HW_init(void) {
    // Local variables.
    ST7066U_status_t status = ST7066U_SUCCESS;
    // Init GPIOs.
    GPIO_configure(&GPIO_LCD_E,   GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_RS,  GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB0, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB1, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB2, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB3, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB4, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB5, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB6, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_configure(&GPIO_LCD_DB7, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    return status;
}

/*******************************************************************/
ST7066U_status_t ST7066U_HW_de_init(void) {
    // Local variables.
    ST7066U_status_t status = ST7066U_SUCCESS;
    // Nothing to do.
    return status;
}

/*******************************************************************/
ST7066U_status_t ST7066U_HW_gpio_write(ST7066_HW_gpio_t gpio, uint8_t state) {
    // Local variables.
    ST7066U_status_t status = ST7066U_SUCCESS;
    // RW not implemented.
    if (gpio == ST7066U_HW_GPIO_RW) goto errors;
    // Write gpio.
    GPIO_write(ST7066_HW_GPIO[gpio], state);
errors:
    return status;
}

/*******************************************************************/
ST7066U_status_t ST7066U_HW_gpio_make_pulse(ST7066_HW_gpio_t gpio, uint32_t pulse_duration_ns) {
    // Local variables.
    ST7066U_status_t status = ST7066U_SUCCESS;
    uint8_t count = 0;
    // Unused parameter.
    UNUSED(pulse_duration_ns);
    // RW not implemented.
    if (gpio == ST7066U_HW_GPIO_RW) goto errors;
    // Make pulse.
    for (count = 0; count < 10; count++) {
        GPIO_write(ST7066_HW_GPIO[gpio], (((count > 1) && (count < 9)) ? 1 : 0));
    }
errors:
    return status;
}

/*******************************************************************/
ST7066U_status_t ST7066U_HW_data_bus_write(uint8_t data_bus_byte) {
    // Local variables.
    ST7066U_status_t status = ST7066U_SUCCESS;
    uint32_t gpioa_odr = 0;
    // Read port.
    gpioa_odr = (GPIOA->ODR);
    // Set bits.
    gpioa_odr &= 0xFFFFFE01;
    gpioa_odr |= (data_bus_byte << 1);
    // Write port.
    GPIOA->ODR = gpioa_odr;
    return status;
}

/*******************************************************************/
ST7066U_status_t ST7066U_HW_delay_milliseconds(uint32_t delay_ms) {
    // Local variables.
    ST7066U_status_t status = ST7066U_SUCCESS;
    LPTIM_status_t lptim_status = LPTIM_SUCCESS;
    // Perform delay.
    lptim_status = LPTIM_delay_milliseconds(delay_ms, LPTIM_DELAY_MODE_ACTIVE);
    LPTIM_exit_error(ST7066U_ERROR_BASE_DELAY);
errors:
    return status;
}

#endif /* ST7066U_DRIVER_DISABLE */
