/*
 * td1208_hw.c
 *
 *  Created on: 14 oct. 2024
 *      Author: Ludo
 */

#include "td1208_hw.h"

#ifndef TD1208_DRIVER_DISABLE_FLAGS_FILE
#include "td1208_driver_flags.h"
#endif
#include "error.h"
#include "gpio_mapping.h"
#include "lptim.h"
#include "nvic_priority.h"
#include "td1208.h"
#include "types.h"
#include "usart.h"

#ifndef TD1208_DRIVER_DISABLE

/*** TD1208 HW local macros ***/

#define TD1208_HW_USART_INSTANCE    USART_INSTANCE_USART2

/*** TD1208 HW functions ***/

/*******************************************************************/
TD1208_status_t TD1208_HW_init(TD1208_HW_configuration_t* configuration) {
    // Local variables.
    TD1208_status_t status = TD1208_SUCCESS;
    USART_status_t usart_status = USART_SUCCESS;
    USART_configuration_t usart_config;
    // Init USART.
    usart_config.baud_rate = (configuration->uart_baud_rate);
    usart_config.nvic_priority = NVIC_PRIORITY_TD1208_UART;
    usart_config.rxne_callback = (configuration->rx_irq_callback);
    usart_status = USART_init(TD1208_HW_USART_INSTANCE, &GPIO_TD1208_USART, &usart_config);
    USART_exit_error(TD1208_ERROR_BASE_UART);
    usart_status = USART_enable_rx(TD1208_HW_USART_INSTANCE);
    USART_exit_error(TD1208_ERROR_BASE_UART);
errors:
    return status;
}

/*******************************************************************/
TD1208_status_t TD1208_HW_de_init(void) {
    // Local variables.
    TD1208_status_t status = TD1208_SUCCESS;
    USART_status_t usart_status = USART_SUCCESS;
    // Release USART.
    usart_status = USART_de_init(TD1208_HW_USART_INSTANCE, &GPIO_TD1208_USART);
    USART_exit_error(TD1208_ERROR_BASE_UART);
errors:
    return status;
}

/*******************************************************************/
TD1208_status_t TD1208_HW_uart_write(uint8_t* data, uint32_t data_size_bytes) {
    // Local variables.
    TD1208_status_t status = TD1208_SUCCESS;
    USART_status_t usart_status = USART_SUCCESS;
    // Release USART.
    usart_status = USART_write(TD1208_HW_USART_INSTANCE, data, data_size_bytes);
    USART_exit_error(TD1208_ERROR_BASE_UART);
errors:
    return status;
}

/*******************************************************************/
TD1208_status_t TD1208_HW_delay_milliseconds(uint32_t delay_ms) {
    // Local variables.
    TD1208_status_t status = TD1208_SUCCESS;
    LPTIM_status_t lptim_status = LPTIM_SUCCESS;
    // Perform delay.
    lptim_status = LPTIM_delay_milliseconds(delay_ms, LPTIM_DELAY_MODE_ACTIVE);
    LPTIM_exit_error(TD1208_ERROR_BASE_DELAY);
errors:
    return status;
}

#endif /* TD1208_DRIVER_DISABLE */
