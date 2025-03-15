/*
 * analog.c
 *
 *  Created on: 09 oct. 2024
 *      Author: Ludo
 */

#include "analog.h"

#include "adc.h"
#include "error.h"
#include "error_base.h"
#include "gpio.h"
#include "mcu_mapping.h"
#include "nvic_priority.h"
#include "psfe_flags.h"
#include "rtc.h"
#include "tim.h"
#include "trcs.h"
#include "types.h"

/*** ANALOG local macros ***/

#define ANALOG_TIMER_PERIOD_MS                  100

#define ANALOG_REF191_VOLTAGE_MV                2048

#if ((PSFE_BOARD_NUMBER == 1) || (PSFE_BOARD_NUMBER == 2) || (PSFE_BOARD_NUMBER == 5) || (PSFE_BOARD_NUMBER == 6) || (PSFE_BOARD_NUMBER == 7) || (PSFE_BOARD_NUMBER == 10))
#define ANALOG_VOUT_DIVIDER_RATIO               2
#define ANALOG_VOUT_VOLTAGE_DIVIDER_RESISTANCE  998000
#else
#define ANALOG_VOUT_DIVIDER_RATIO               6
#define ANALOG_VOUT_VOLTAGE_DIVIDER_RESISTANCE  599000
#endif

#define ANALOG_CALIBRATION_PERIOD_SECONDS       300

#define ANALOG_ERROR_VALUE                      0x7FFFFFFF

/*** ANALOG local structures ***/

/*******************************************************************/
typedef union {
    struct {
        unsigned trcs_started :1;
        unsigned trcs_enable :1;
        unsigned trcs_bypass :1;
    };
    uint8_t all;
} ANALOG_flags_t;

/*******************************************************************/
typedef struct {
    volatile ANALOG_flags_t flags;
    int32_t data[ANALOG_CHANNEL_LAST];
    int32_t ref191_data_12bits;
    uint32_t calibration_next_time_seconds;
} ANALOG_context_t;

/*** ANALOG local global variables ***/

static ANALOG_context_t analog_ctx = {
    .flags.all = 0,
    .data = { [0 ... (ANALOG_CHANNEL_LAST - 1)] = 0 },
    .ref191_data_12bits = ANALOG_ERROR_VALUE,
    .calibration_next_time_seconds = 0
};

/*** ANALOG local functions ***/

/*******************************************************************/
static ANALOG_status_t _ANALOG_convert_channel(ANALOG_channel_t channel) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    int32_t adc_data_12bits = 0;
    int32_t analog_data = 0;
    int32_t vout_voltage_divider_current_ua = 0;
    // Check channel.
    switch (channel) {
    case ANALOG_CHANNEL_VMCU_MV:
        // MCU voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VREFINT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        adc_status = ADC_compute_vmcu(adc_data_12bits, ADC_get_vrefint_voltage_mv(), &analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        break;
    case ANALOG_CHANNEL_TMCU_DEGREES:
        // MCU temperature.
        adc_status = ADC_convert_channel(ADC_CHANNEL_TEMPERATURE_SENSOR, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to degrees.
        adc_status = ADC_compute_tmcu(analog_ctx.data[ANALOG_CHANNEL_VMCU_MV], adc_data_12bits, &analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        break;
    case ANALOG_CHANNEL_VOUT_MV:
        // Check calibration.
        if (analog_ctx.ref191_data_12bits == ANALOG_ERROR_VALUE) {
            status = ANALOG_ERROR_CALIBRATION_MISSING;
            goto errors;
        }
        // Output voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VOUT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        analog_data = (adc_data_12bits * ANALOG_REF191_VOLTAGE_MV * ANALOG_VOUT_DIVIDER_RATIO) / (analog_ctx.ref191_data_12bits);
        break;
    case ANALOG_CHANNEL_IOUT_MV:
        // Check calibration.
        if (analog_ctx.ref191_data_12bits == ANALOG_ERROR_VALUE) {
            status = ANALOG_ERROR_CALIBRATION_MISSING;
            goto errors;
        }
        // Output current.
        adc_status = ADC_convert_channel(ADC_CHANNEL_IOUT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        analog_data = (adc_data_12bits * ANALOG_REF191_VOLTAGE_MV) / (analog_ctx.ref191_data_12bits);
        break;
    case ANALOG_CHANNEL_IOUT_UA:
        // Check bypass switch.
        if (analog_ctx.flags.trcs_bypass == 0) {
            // Read from TRCS board
            analog_data = TRCS_get_iout();
            // Compute output voltage divider current.
            vout_voltage_divider_current_ua = (analog_ctx.data[ANALOG_CHANNEL_VOUT_MV] * 1000) / (ANALOG_VOUT_VOLTAGE_DIVIDER_RESISTANCE);
            // Remove offset current.
            if (analog_data > vout_voltage_divider_current_ua) {
                analog_data -= vout_voltage_divider_current_ua;
            }
            else {
                analog_data = 0;
            }
        }
        else {
            analog_data = ANALOG_ERROR_VALUE;
        }
        break;
    default:
        status = ANALOG_ERROR_CHANNEL;
        goto errors;
    }
    // Update local value.
    analog_ctx.data[channel] = analog_data;
errors:
    return status;
}

/*******************************************************************/
static void _ANALOG_timer_irq_callback(void) {
    // Local variables.
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    uint8_t idx = 0;
    // Convert all channels.
    for (idx = 0; idx < ANALOG_CHANNEL_LAST; idx++) {
        analog_status = _ANALOG_convert_channel(idx);
        ANALOG_stack_error(ERROR_BASE_ANALOG);
    }
}

/*******************************************************************/
static void _ANALOG_trcs_process_callback(void) {
    // Local variables.
    TRCS_status_t trcs_status = TRCS_SUCCESS;
    // Process TRCS board.
    trcs_status = TRCS_process();
    TRCS_stack_error(ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_TRCS);
}

/*******************************************************************/
static ANALOG_status_t _ANALOG_calibrate(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    int32_t adc_data_12bits = 0;
    // Convert external voltage reference.
    adc_status = ADC_convert_channel(ADC_CHANNEL_REF191, &adc_data_12bits);
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
    // Update local calibration value.
    analog_ctx.ref191_data_12bits = adc_data_12bits;
errors:
    return status;
}

/*** ANALOG functions ***/

/*******************************************************************/
ANALOG_status_t ANALOG_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    TRCS_status_t trcs_status = TRCS_SUCCESS;
    uint8_t idx = 0;
    // Init context.
    analog_ctx.flags.all = 0;
    analog_ctx.ref191_data_12bits = ANALOG_ERROR_VALUE;
    analog_ctx.calibration_next_time_seconds = 0;
    // Init data.
    for (idx = 0; idx < ANALOG_CHANNEL_LAST; idx++) {
        analog_ctx.data[idx] = 0;
    }
    // Init bypass detect.
    GPIO_configure(&GPIO_TRCS_BYPASS, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    // Init internal ADC.
    adc_status = ADC_init(&GPIO_ADC_GPIO);
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
    // Init sampling timer.
    tim_status = TIM_STD_init(TIM_INSTANCE_ANALOG, NVIC_PRIORITY_ANALOG_TIMER);
    TIM_exit_error(ANALOG_ERROR_BASE_TIM);
    // Init TRCS board.
    trcs_status = TRCS_init();
    TRCS_exit_error(ANALOG_ERROR_BASE_TRCS);
    // First calibration.
    status = _ANALOG_calibrate();
    if (status != ANALOG_SUCCESS) goto errors;
    // Start sampling timer.
    tim_status = TIM_STD_start(TIM_INSTANCE_ANALOG, ANALOG_TIMER_PERIOD_MS, TIM_UNIT_MS, &_ANALOG_timer_irq_callback);
    TIM_exit_error(TRCS_ERROR_BASE_TIMER);
errors:
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_de_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    TRCS_status_t trcs_status = TRCS_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Erase calibration value.
    analog_ctx.ref191_data_12bits = ANALOG_ERROR_VALUE;
    // Stop sampling timer.
    tim_status = TIM_STD_stop(TIM_INSTANCE_ANALOG);
    TIM_stack_error(ERROR_BASE_ANALOG + TRCS_ERROR_BASE_TIMER);
    // Init TRCS board.
    trcs_status = TRCS_de_init();
    TRCS_stack_error(ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_TRCS);
    // Release internal ADC.
    adc_status = ADC_de_init();
    ADC_stack_error(ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_ADC);
    // Release sampling timer.
    tim_status = TIM_STD_de_init(TIM_INSTANCE_ANALOG);
    TIM_stack_error(ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_TIM);
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_start_trcs(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    // Update local flag.
    analog_ctx.flags.trcs_enable = 1;
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_stop_trcs(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    // Update local flag.
    analog_ctx.flags.trcs_enable = 0;
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_process(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    TRCS_status_t trcs_status = TRCS_SUCCESS;
    // Update bypass switch state.
    analog_ctx.flags.trcs_bypass = GPIO_read(&GPIO_TRCS_BYPASS);
    // Check bypass flag change.
    if (((analog_ctx.flags.trcs_bypass != 0) || (analog_ctx.flags.trcs_enable == 0)) && (analog_ctx.flags.trcs_started != 0)) {
        // Update flag.
        analog_ctx.flags.trcs_started = 0;
        // Stop TRCS board.
        trcs_status = TRCS_stop();
        TRCS_exit_error(ANALOG_ERROR_BASE_TRCS);
    }
    if (((analog_ctx.flags.trcs_bypass == 0) && (analog_ctx.flags.trcs_enable != 0)) && (analog_ctx.flags.trcs_started == 0)) {
        // Update flag.
        analog_ctx.flags.trcs_started = 1;
        // Stop TRCS board.
        trcs_status = TRCS_start(&_ANALOG_trcs_process_callback);
        TRCS_exit_error(ANALOG_ERROR_BASE_TRCS);
    }
    // Check calibration period.
    if (RTC_get_uptime_seconds() >= analog_ctx.calibration_next_time_seconds) {
        // Update next time.
        analog_ctx.calibration_next_time_seconds += ANALOG_CALIBRATION_PERIOD_SECONDS;
        // Convert bandgap.
        status = _ANALOG_calibrate();
        if (status != ANALOG_SUCCESS) goto errors;
    }
    return status;
errors:
    TRCS_stop();
    analog_ctx.flags.trcs_started = 0;
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_read_channel(ANALOG_channel_t channel, int32_t* analog_data) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    // Check parameters.
    if (channel >= ANALOG_CHANNEL_LAST) {
        status = ANALOG_ERROR_CHANNEL;
        goto errors;
    }
    if (analog_data == NULL) {
        status = ANALOG_ERROR_NULL_PARAMETER;
        goto errors;
    }
    (*analog_data) = analog_ctx.data[channel];
errors:
    return status;
}

/*******************************************************************/
uint8_t ANALOG_get_bypass_switch_state(void) {
    return (analog_ctx.flags.trcs_bypass);
}

/*******************************************************************/
ANALOG_iout_range_t ANALOG_get_iout_range(void) {
    // Local variables.
    ANALOG_iout_range_t iout_range = ((analog_ctx.flags.trcs_bypass != 0) ? ANALOG_IOUT_RANGE_BYPASS : ((ANALOG_iout_range_t) TRCS_get_range_state()));
    return iout_range;
}
