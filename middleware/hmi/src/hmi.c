/*
 * hmi.c
 *
 *  Created on: 12 oct. 2024
 *      Author: Ludo
 */

#include "hmi.h"

#include "analog.h"
#include "error.h"
#include "error_base.h"
#include "math.h"
#include "mode.h"
#include "nvic_priority.h"
#include "st7066u.h"
#include "string.h"
#include "tim.h"
#include "types.h"
#include "version.h"

/*** HMI local macros ***/

#define HMI_TIMER_INSTANCE                  TIM_INSTANCE_TIM2
#define HMI_DISPLAY_PERIOD_MS               300

#define HMI_HW_VERSION_PRINT_DURATION_MS    2000
#define HMI_SW_VERSION_PRINT_DURATION_MS    2000
#define HMI_SIGFOX_EP_ID_PRINT_DURATION_MS  2000

#define HMI_VOUT_ERROR_THRESHOLD_MV         100

/*** HMI local structures ***/

/*******************************************************************/
typedef enum {
    HMI_STATE_OFF = 0,
    HMI_STATE_INIT,
    HMI_STATE_HW_VERSION,
    HMI_STATE_SW_VERSION,
#ifdef PSFE_SIGFOX_MONITORING
    HMI_STATE_SIGFOX_EP_ID,
#endif
    HMI_STATE_ANALOG_DATA,
    HMI_STATE_LAST
} HMI_state_t;

/*******************************************************************/
typedef struct {
    HMI_state_t state;
    volatile uint32_t uptime_ms;
    uint32_t state_switch_time_ms;
} HMI_context_t;

/*** HMI local global variables ***/

static HMI_context_t hmi_ctx;

/*** HMI local functions ***/

/*******************************************************************/
static HMI_status_t _HMI_print_value(uint8_t row, int32_t value, uint8_t divider_exponent, char_t* unit) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    char_t lcd_string[ST7066U_DRIVER_SCREEN_WIDTH] = { STRING_CHAR_NULL };
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
    st7066u_status = ST7066U_print_string(row, 0, lcd_string);
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_print_hw_version(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    // Print version.
    st7066u_status = ST7066U_print_string(0, 0, " ATXFox ");
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
    st7066u_status = ST7066U_print_string(1, 0, " HW 1.0 ");
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_print_sw_version(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    char_t sw_version_string[ST7066U_DRIVER_SCREEN_WIDTH];
    // Build string.
    string_status = STRING_integer_to_string((GIT_MAJOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[0]));
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    string_status = STRING_integer_to_string((GIT_MAJOR_VERSION - 10 * (GIT_MAJOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[1]));
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    sw_version_string[2] = STRING_CHAR_DOT;
    string_status = STRING_integer_to_string((GIT_MINOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[3]));
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    string_status = STRING_integer_to_string((GIT_MINOR_VERSION - 10 * (GIT_MINOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[4]));
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    sw_version_string[5] = STRING_CHAR_DOT;
    string_status = STRING_integer_to_string((GIT_COMMIT_INDEX / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[6]));
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    string_status = STRING_integer_to_string((GIT_COMMIT_INDEX - 10 * (GIT_COMMIT_INDEX / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[7]));
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    // Print version.
    if (GIT_DIRTY_FLAG == 0) {
        st7066u_status = ST7066U_print_string(0, 0, "   sw   ");
    }
    else {
        st7066u_status = ST7066U_print_string(0, 0, "sw dirty");
    }
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
    st7066u_status = ST7066U_print_string(1, 0, sw_version_string);
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
static HMI_status_t _HMI_print_sigfox_ep_id(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    SIGFOX_status_t sigfox_status = SIGFOX_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    uint8_t sigfox_ep_id[TD1208_SIGFOX_EP_ID_SIZE_BYTES];
    char_t sigfox_ep_id_str[TD1208_SIGFOX_EP_ID_SIZE_BYTES * MATH_U8_SIZE_HEXADECIMAL_DIGITS];
    uint8_t idx = 0;
    // Read EP-ID.
    sigfox_status = SIGFOX_get_ep_id((uint8_t*) sigfox_ep_id);
    SIGFOX_stack_error(ERROR_BASE_SIGFOX);
    // Build corresponding string.
    for (idx = 0; idx < TD1208_SIGFOX_EP_ID_SIZE_BYTES; idx++) {
        string_status = STRING_integer_to_string(sigfox_ep_id[idx], STRING_FORMAT_HEXADECIMAL, 0, &(sigfox_ep_id_str[2 * idx]));
        STRING_exit_error(HMI_ERROR_BASE_STRING);
    }
    st7066u_status = ST7066U_print_string(0, 0, "SFX EPID");
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
    // Print ID.
    if (sigfox_status == SIGFOX_SUCCESS) {
        st7066u_status = ST7066U_print_string(1, 0, sigfox_ep_id_str);
    }
    else {
        st7066u_status = ST7066U_print_string(1, 0, "TD ERROR");
    }
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}
#endif

/*******************************************************************/
static HMI_status_t _HMI_process(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    int32_t analog_data = 0;
    // State machine.
    switch (hmi_ctx.state) {
    case HMI_STATE_OFF:
        // Nothing to do.
        break;
    case HMI_STATE_INIT:
        // Print HW version.
        _HMI_print_hw_version();
        // Update state.
        hmi_ctx.state_switch_time_ms = hmi_ctx.uptime_ms;
        hmi_ctx.state = HMI_STATE_HW_VERSION;
        break;
    case HMI_STATE_HW_VERSION:
        // Check delay.
        if (hmi_ctx.uptime_ms >= (hmi_ctx.state_switch_time_ms + HMI_HW_VERSION_PRINT_DURATION_MS)) {
            // Print SW version.
            _HMI_print_sw_version();
            // Update state.
            hmi_ctx.state_switch_time_ms = hmi_ctx.uptime_ms;
            hmi_ctx.state = HMI_STATE_SW_VERSION;
        }
        break;
    case HMI_STATE_SW_VERSION:
        // Check delay.
        if (hmi_ctx.uptime_ms >= (hmi_ctx.state_switch_time_ms + HMI_SW_VERSION_PRINT_DURATION_MS)) {
#ifdef PSFE_SIGFOX_MONITORING
            // Print SW version.
            _HMI_print_sigfox_ep_id();
            // Update state.
            hmi_ctx.state_switch_time_ms = hmi_ctx.uptime_ms;
            hmi_ctx.state = HMI_STATE_SIGFOX_EP_ID;
#else
            hmi_ctx.state = HMI_STATE_ANALOG_DATA;
#endif
        }
        break;
#ifdef PSFE_SIGFOX_MONITORING
    case HMI_STATE_SIGFOX_EP_ID:
        // Check delay.
        if (hmi_ctx.uptime_ms >= (hmi_ctx.state_switch_time_ms + HMI_SIGFOX_EP_ID_PRINT_DURATION_MS)) {
            // Update state.
            hmi_ctx.state = HMI_STATE_ANALOG_DATA;
        }
        break;
#endif
    case HMI_STATE_ANALOG_DATA:
        // Read output voltage.
        analog_status = ANALOG_read_channel(ANALOG_CHANNEL_VOUT_MV, &analog_data);
        ANALOG_exit_error(HMI_ERROR_BASE_ANALOG);
        // Print output voltage.
        if (analog_data > HMI_VOUT_ERROR_THRESHOLD_MV) {
            status = _HMI_print_value(0, analog_data, 3, "V");
            if (status != HMI_SUCCESS) goto errors;
        }
        else {
            st7066u_status = ST7066U_print_string(0, 0, "NO INPUT");
            ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
        }
        // Check bypass switch state.
        if (ANALOG_get_bypass_switch_state() == 0) {
            // Read output current.
            analog_status = ANALOG_read_channel(ANALOG_CHANNEL_IOUT_UA, &analog_data);
            ANALOG_exit_error(HMI_ERROR_BASE_ANALOG);
            // Print output current.
            if (analog_data < 1000) {
                status = _HMI_print_value(1, analog_data, 0, "ua");
                if (status != HMI_SUCCESS) goto errors;
            }
            else {
                status = _HMI_print_value(1, analog_data, 3, "ma");
                if (status != HMI_SUCCESS) goto errors;
            }
        }
        else {
            st7066u_status = ST7066U_print_string(1, 0, " BYPASS ");
            ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
        }
        break;
    default:
        status = HMI_ERROR_STATE;
        goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
static void _HMI_timer_irq_callback(void) {
    // Local variables.
    HMI_status_t hmi_status = HMI_SUCCESS;
    // Set local flag.
    hmi_ctx.uptime_ms += HMI_DISPLAY_PERIOD_MS;
    // Process HMI.
    hmi_status = _HMI_process();
    HMI_stack_error(ERROR_BASE_HMI);
}

/*** HMI functions ***/

/*******************************************************************/
HMI_status_t HMI_init(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Init context.
    hmi_ctx.state = HMI_STATE_OFF;
    hmi_ctx.uptime_ms = 0;
    hmi_ctx.state_switch_time_ms = 0;
    // Init LCD driver.
    st7066u_status = ST7066U_init();
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
    // Init display timer.
    tim_status = TIM_STD_init(HMI_TIMER_INSTANCE, NVIC_PRIORITY_HMI_TIMER);
    TIM_exit_error(HMI_ERROR_BASE_TIM);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_de_init(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Release display timer.
    tim_status = TIM_STD_de_init(HMI_TIMER_INSTANCE);
    TIM_exit_error(HMI_ERROR_BASE_TIM);
    // Release LCD driver.
    st7066u_status = ST7066U_de_init();
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_start(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Update state.
    hmi_ctx.state = HMI_STATE_INIT;
    // Start timer.
    tim_status = TIM_STD_start(HMI_TIMER_INSTANCE, (HMI_DISPLAY_PERIOD_MS * MATH_POWER_10[6]), &_HMI_timer_irq_callback);
    TIM_exit_error(HMI_ERROR_BASE_TIM);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_stop(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    ST7066U_status_t st7066u_status = ST7066U_SUCCESS;
    // Update state.
    hmi_ctx.state = HMI_STATE_OFF;
    // Stop timer.
    tim_status = TIM_STD_stop(HMI_TIMER_INSTANCE);
    TIM_exit_error(HMI_ERROR_BASE_TIM);
    // Clear screen.
    st7066u_status = ST7066U_clear();
    ST7066U_exit_error(HMI_ERROR_BASE_ST7066U);
errors:
    return status;
}
