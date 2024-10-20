/*
 * sigfox.c
 *
 *  Created on: 19 oct. 2024
 *      Author: Ludo
 */

#include "sigfox.h"

#include "analog.h"
#include "error.h"
#include "iwdg.h"
#include "math.h"
#include "mode.h"
#include "rcc_reg.h"
#include "rtc.h"
#include "td1208.h"
#include "types.h"
#include "version.h"

#ifdef PSFE_SIGFOX_MONITORING

/*** SIGFOX local macros ***/

#define SIGFOX_PERIOD_SECONDS           300

#define SIGFOX_STARTUP_DATA_LENGTH      8
#define SIGFOX_MONITORING_DATA_LENGTH   9
#define SIGFOX_ERROR_STACK_DATA_LENGTH  12

/*** SIGFOX local structures ***/

/*******************************************************************/
typedef union {
    uint8_t frame[SIGFOX_STARTUP_DATA_LENGTH];
    struct {
        unsigned reset_reason : 8;
        unsigned major_version : 8;
        unsigned minor_version : 8;
        unsigned commit_index : 8;
        unsigned commit_id : 28;
        unsigned dirty_flag : 4;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SIGFOX_startup_data_t;

/*******************************************************************/
typedef struct {
    union {
        uint8_t frame[SIGFOX_MONITORING_DATA_LENGTH];
        struct {
            unsigned vout_mv : 16;
            unsigned iout_range : 8;
            unsigned iout_ua : 24;
            unsigned vmcu_mv : 16;
            unsigned tmcu_degrees : 8;
        } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
    };
} SIGFOX_monitoring_data_t;

/*******************************************************************/
typedef union {
    struct {
        unsigned ep_id_read : 1;
        unsigned startup_frame_sent : 1;
        unsigned enable : 1;
    };
    uint8_t all;
} SIGFOX_flags_t;

/*******************************************************************/
typedef struct {
    SIGFOX_flags_t flags;
    uint32_t next_transmission_time_seconds;
    uint8_t ep_id[TD1208_SIGFOX_EP_ID_SIZE_BYTES];
} SIGFOX_context_t;

/*** SIGFOX local global variables ***/

static SIGFOX_context_t sigfox_ctx = { .flags.all = 0 };

/*** SIGFOX functions ***/

/*******************************************************************/
SIGFOX_status_t SIGFOX_init(void) {
    // Local variables.
    SIGFOX_status_t status = SIGFOX_SUCCESS;
    TD1208_status_t td1208_status = TD1208_SUCCESS;
    // Init context.
    sigfox_ctx.next_transmission_time_seconds = 0;
    // Init TD1208.
    td1208_status = TD1208_init();
    TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
    // Read Sigfox EP ID.
    td1208_status = TD1208_get_sigfox_ep_id(sigfox_ctx.ep_id);
    TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
    // Update flag.
    sigfox_ctx.flags.ep_id_read = 1;
errors:
    // Reload watchdog.
    IWDG_reload();
    return status;
}

/*******************************************************************/
SIGFOX_status_t SIGFOX_de_init(void) {
    // Local variables.
    SIGFOX_status_t status = SIGFOX_SUCCESS;
    TD1208_status_t td1208_status = TD1208_SUCCESS;
    // Init TD1208.
    td1208_status = TD1208_de_init();
    TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
errors:
    return status;
}

/*******************************************************************/
SIGFOX_status_t SIGFOX_start(void) {
    // Local variables.
    SIGFOX_status_t status = SIGFOX_SUCCESS;
    TD1208_status_t td1208_status = TD1208_SUCCESS;
    // Update local flag.
    sigfox_ctx.flags.enable = 1;
    // Send start frame.
    td1208_status = TD1208_send_bit(1);
    TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
errors:
    // Reload watchdog.
    IWDG_reload();
    return status;
}

/*******************************************************************/
SIGFOX_status_t SIGFOX_stop(void) {
    // Local variables.
    SIGFOX_status_t status = SIGFOX_SUCCESS;
    TD1208_status_t td1208_status = TD1208_SUCCESS;
    // Update local flag.
    sigfox_ctx.flags.enable = 0;
    // Send start frame.
    td1208_status = TD1208_send_bit(0);
    TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
errors:
    // Reload watchdog.
    IWDG_reload();
    return status;
}

/*******************************************************************/
SIGFOX_status_t SIGFOX_process(void) {
    // Local variables.
    SIGFOX_status_t status = SIGFOX_SUCCESS;
    TD1208_status_t td1208_status = TD1208_SUCCESS;
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    MATH_status_t math_status = MATH_SUCCESS;
    SIGFOX_startup_data_t startup_frame;
    SIGFOX_monitoring_data_t monitoring_frame;
    uint8_t error_stack_frame[SIGFOX_ERROR_STACK_DATA_LENGTH];
    int32_t vout_mv = 0;
    int32_t iout_ua = 0;
    int32_t vmcu_mv = 0;
    int32_t tmcu_degrees = 0;
    uint32_t tmcu_degrees_signed_magnitude = 0;
    ERROR_code_t error_code;
    uint8_t idx = 0;
    // Check period.
    if ((sigfox_ctx.flags.enable != 0) && (RTC_get_uptime_seconds() >= sigfox_ctx.next_transmission_time_seconds)) {
        // Update next transmission time.
        sigfox_ctx.next_transmission_time_seconds = (RTC_get_uptime_seconds() + SIGFOX_PERIOD_SECONDS);
        // Send startup frame is needed.
        if (sigfox_ctx.flags.startup_frame_sent == 0) {
            // Set flag.
            sigfox_ctx.flags.startup_frame_sent = 1;
            // Build startup frame.
            startup_frame.reset_reason = ((RCC -> CSR) >> 24) & 0xFF;
            startup_frame.major_version = GIT_MAJOR_VERSION;
            startup_frame.minor_version = GIT_MINOR_VERSION;
            startup_frame.commit_index = GIT_COMMIT_INDEX;
            startup_frame.commit_id = GIT_COMMIT_ID;
            startup_frame.dirty_flag = GIT_DIRTY_FLAG;
            // Send startup message through Sigfox.
            td1208_status = TD1208_send_frame(startup_frame.frame, SIGFOX_STARTUP_DATA_LENGTH);
            TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
            // Reload watchdog.
            IWDG_reload();
        }
        // Read analog data.
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_VOUT_MV, &vout_mv);
        ANALOG_exit_error(SIGFOX_ERROR_BASE_ANALOG);
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_IOUT_UA, &iout_ua);
        ANALOG_exit_error(SIGFOX_ERROR_BASE_ANALOG);
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_VMCU_MV, &vmcu_mv);
        ANALOG_exit_error(SIGFOX_ERROR_BASE_ANALOG);
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_TMCU_DEGREES, &tmcu_degrees);
        ANALOG_exit_error(SIGFOX_ERROR_BASE_ANALOG);
        // Convert to signed magnitude
        math_status = MATH_integer_to_signed_magnitude(tmcu_degrees, (MATH_S32_SIZE_BITS - 1), &tmcu_degrees_signed_magnitude);
        MATH_exit_error(SIGFOX_ERROR_BASE_MATH);
        // Build monitoring frame.
        monitoring_frame.vout_mv = vout_mv;
        monitoring_frame.iout_ua = iout_ua;
        monitoring_frame.iout_range = ANALOG_get_iout_range();
        monitoring_frame.vmcu_mv = vmcu_mv;
        monitoring_frame.tmcu_degrees = tmcu_degrees_signed_magnitude;
        // Send monitoring data.
        td1208_status = TD1208_send_frame(monitoring_frame.frame, SIGFOX_MONITORING_DATA_LENGTH);
        TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
        // Reload watchdog.
        IWDG_reload();
        // Check stack.
        if (ERROR_stack_is_empty() == 0) {
            // Read error stack.
            for (idx=0 ; idx<(SIGFOX_ERROR_STACK_DATA_LENGTH >> 1) ; idx++) {
                error_code = ERROR_stack_read();
                error_stack_frame[(idx << 1) + 0] = (uint8_t) ((error_code >> 8) & 0x00FF);
                error_stack_frame[(idx << 1) + 1] = (uint8_t) ((error_code >> 0) & 0x00FF);
            }
            // Send error stack data.
            td1208_status = TD1208_send_frame(error_stack_frame, SIGFOX_ERROR_STACK_DATA_LENGTH);
            TD1208_exit_error(SIGFOX_ERROR_BASE_TD1208);
            // Reload watchdog.
            IWDG_reload();
            // Reset error stack.
            ERROR_stack_init();
        }
    }
errors:
    return status;
}

/*******************************************************************/
SIGFOX_status_t SIGFOX_get_ep_id(uint8_t* sigfox_ep_id) {
    // Local variables.
    SIGFOX_status_t status = SIGFOX_SUCCESS;
    uint8_t idx = 0;
    // Check parameter.
    if (sigfox_ep_id == NULL) {
        status = SIGFOX_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Check read flag.
    if (sigfox_ctx.flags.ep_id_read == 0) {
        status = SIGFOX_ERROR_EP_ID_NOT_READ;
        goto errors;
    }
    for (idx = 0; idx < TD1208_SIGFOX_EP_ID_SIZE_BYTES; idx++) {
        sigfox_ep_id[idx] = sigfox_ctx.ep_id[idx];
    }
errors:
    return status;
}

#endif /* PSFE_SIGFOX_MONITORING */
