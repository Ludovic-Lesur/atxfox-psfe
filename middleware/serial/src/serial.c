/*
 * serial.c
 *
 *  Created on: 19 oct. 2024
 *      Author: Ludo
 */

#include "serial.h"

#include "analog.h"
#include "error.h"
#include "psfe_flags.h"
#include "rtc.h"
#include "terminal.h"
#include "terminal_instance.h"
#include "types.h"

#ifdef PSFE_SERIAL_MONITORING

/*** SERIAL local macros ***/

#define SERIAL_PERIOD_SECONDS   1

/*** SERIAL local structures ***/

/*******************************************************************/
typedef struct {
    uint8_t enable;
    uint32_t next_transmission_time_seconds;
} SERIAL_context_t;

/*** SERIAL local global variables ***/

static SERIAL_context_t serial_ctx;

/*** SERIAL functions ***/

/*******************************************************************/
SERIAL_status_t SERIAL_init(void) {
    // Local variables.
    SERIAL_status_t status = SERIAL_SUCCESS;
    TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
    // Init context.
    serial_ctx.enable = 0;
    serial_ctx.next_transmission_time_seconds = 0;
    // Open terminal.
    terminal_status = TERMINAL_open(TERMINAL_INSTANCE_SERIAL, NULL);
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
errors:
    return status;
}

/*******************************************************************/
SERIAL_status_t SERIAL_de_init(void) {
    // Local variables.
    SERIAL_status_t status = SERIAL_SUCCESS;
    TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
    // Close terminal.
    terminal_status = TERMINAL_close(TERMINAL_INSTANCE_SERIAL);
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
errors:
    return status;
}

/*******************************************************************/
SERIAL_status_t SERIAL_start(void) {
    // Local variables.
    SERIAL_status_t status = SERIAL_SUCCESS;
    TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
    // Update local flag.
    serial_ctx.enable = 1;
    // Print start message.
    terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "SERIAL monitoring start\r\n");
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
    terminal_status = TERMINAL_print_buffer(TERMINAL_INSTANCE_SERIAL);
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
    terminal_status = TERMINAL_flush_buffer(TERMINAL_INSTANCE_SERIAL);
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
errors:
    return status;
}

/*******************************************************************/
SERIAL_status_t SERIAL_stop(void) {
    // Local variables.
    SERIAL_status_t status = SERIAL_SUCCESS;
    TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
    // Update local flag.
    serial_ctx.enable = 0;
    // Print stop message.
    terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "SERIAL monitoring stop\r\n");
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
    terminal_status = TERMINAL_print_buffer(TERMINAL_INSTANCE_SERIAL);
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
    terminal_status = TERMINAL_flush_buffer(TERMINAL_INSTANCE_SERIAL);
    TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
errors:
    return status;
}

/*******************************************************************/
SERIAL_status_t SERIAL_process(void) {
    // Local variables.
    SERIAL_status_t status = SERIAL_SUCCESS;
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
    int32_t vout_mv = 0;
    int32_t iout_ua = 0;
    // Check period.
    if ((serial_ctx.enable != 0) && (RTC_get_uptime_seconds() >= serial_ctx.next_transmission_time_seconds)) {
        // Update next transmission time.
        serial_ctx.next_transmission_time_seconds = (RTC_get_uptime_seconds() + SERIAL_PERIOD_SECONDS);
        // Read analog data.
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_VOUT_MV, &vout_mv);
        ANALOG_exit_error(SERIAL_ERROR_BASE_ANALOG);
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_IOUT_UA, &iout_ua);
        ANALOG_exit_error(SERIAL_ERROR_BASE_ANALOG);
        // Print output voltage.
        terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "Vout=");
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        terminal_status = TERMINAL_buffer_add_integer(TERMINAL_INSTANCE_SERIAL, vout_mv, STRING_FORMAT_DECIMAL, 0);
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "mV Iout=");
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        // Print output current.
        if (ANALOG_get_bypass_switch_state() == 0) {
            terminal_status = TERMINAL_buffer_add_integer(TERMINAL_INSTANCE_SERIAL, iout_ua, STRING_FORMAT_DECIMAL, 0);
            TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
            terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "uA ");
            TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        }
        else {
            terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "N/A ");
            TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        }
        // Print range.
        terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "Range=");
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        terminal_status = TERMINAL_buffer_add_integer(TERMINAL_INSTANCE_SERIAL, (int32_t) ANALOG_get_iout_range(), STRING_FORMAT_DECIMAL, 0);
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        terminal_status = TERMINAL_buffer_add_string(TERMINAL_INSTANCE_SERIAL, "\r\n");
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        // Send serial message.
        terminal_status = TERMINAL_print_buffer(TERMINAL_INSTANCE_SERIAL);
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
        terminal_status = TERMINAL_flush_buffer(TERMINAL_INSTANCE_SERIAL);
        TERMINAL_exit_error(SERIAL_ERROR_BASE_TERMINAL);
    }
errors:
    return status;
}

#endif /*** PSFE_SERIAL_MONITORING ***/
