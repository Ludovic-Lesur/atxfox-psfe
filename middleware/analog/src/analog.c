/*
 * analog.c
 *
 *  Created on: 09 oct. 2024
 *      Author: Ludo
 */

#include "analog.h"

#include "adc.h"
#include "error.h"
#include "gpio_mapping.h"
#include "mode.h"
#include "types.h"

/*** ANALOG local macros ***/

#define ANALOG_VMCU_MV_DEFAULT                  3300
#define ANALOG_TMCU_DEGREES_DEFAULT             25

#define ANALOG_REF191_VOLTAGE_MV                2048

#if ((PSFE_BOARD_NUMBER == 1) || (PSFE_BOARD_NUMBER == 2) || (PSFE_BOARD_NUMBER == 5) || (PSFE_BOARD_NUMBER == 6) || (PSFE_BOARD_NUMBER == 7) || (PSFE_BOARD_NUMBER == 10))
#define ANALOG_VOUT_DIVIDER_RATIO_NUM           2
#define ANALOG_VOUT_DIVIDER_RATIO_DEN           1
#else
#define ANALOG_VOUT_DIVIDER_RATIO_NUM           599
#define ANALOG_VOUT_DIVIDER_RATIO_DEN           100
#endif

#define ANALOG_ADC_CHANNEL_REF191               ADC_CHANNEL_IN9
#define ANALOG_ADC_CHANNEL_VOUT                 ADC_CHANNEL_IN8
#define ANALOG_ADC_CHANNEL_IOUT                 ADC_CHANNEL_IN0

#define ANALOG_ERROR_VALUE                      0xFFFF

/*** ANALOG local structures ***/

/*******************************************************************/
typedef struct {
    int32_t vmcu_mv;
    int32_t ref191_data_12bits;
} ANALOG_context_t;

/*** ANALOG local global variables ***/

static ANALOG_context_t analog_ctx = { .ref191_data_12bits = ANALOG_ERROR_VALUE };

/*** ANALOG functions ***/

/*******************************************************************/
ANALOG_status_t ANALOG_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    // Init context.
    analog_ctx.vmcu_mv = ANALOG_VMCU_MV_DEFAULT;
    analog_ctx.ref191_data_12bits = ANALOG_ERROR_VALUE;
    // Init internal ADC.
    adc_status = ADC_init(&GPIO_ADC_GPIO);
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
errors:
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_de_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    // Erase calibration value.
    analog_ctx.ref191_data_12bits = ANALOG_ERROR_VALUE;
    // Release internal ADC.
    adc_status = ADC_de_init();
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
errors:
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_calibrate(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    int32_t adc_data_12bits = 0;
    // Convert external voltage reference.
    adc_status = ADC_convert_channel(ANALOG_ADC_CHANNEL_REF191, &adc_data_12bits);
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
    // Update local calibration value.
    analog_ctx.ref191_data_12bits = adc_data_12bits;
errors:
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_convert_channel(ANALOG_channel_t channel, int32_t* analog_data) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    int32_t adc_data_12bits = 0;
    // Check parameter.
    if (analog_data == NULL) {
        status = ANALOG_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Check channel.
    switch (channel) {
    case ANALOG_CHANNEL_VMCU_MV:
        // MCU voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VREFINT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        adc_status = ADC_compute_vmcu(adc_data_12bits, ADC_get_vrefint_voltage_mv(), analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Update local value for temperature computation.
        analog_ctx.vmcu_mv = (*analog_data);
        break;
    case ANALOG_CHANNEL_TMCU_DEGREES:
        // MCU temperature.
        adc_status = ADC_convert_channel(ADC_CHANNEL_TEMPERATURE_SENSOR, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to degrees.
        adc_status = ADC_compute_tmcu(analog_ctx.vmcu_mv, adc_data_12bits, analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        break;
    case ANALOG_CHANNEL_VOUT_MV:
        // Check calibration.
        if (analog_ctx.ref191_data_12bits == ANALOG_ERROR_VALUE) {
            status = ANALOG_ERROR_CALIBRATION_MISSING;
            goto errors;
        }
        // Output voltage.
        adc_status = ADC_convert_channel(ANALOG_ADC_CHANNEL_VOUT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = ((int32_t) adc_data_12bits * ANALOG_REF191_VOLTAGE_MV * ANALOG_VOUT_DIVIDER_RATIO_NUM) / ((int32_t) analog_ctx.ref191_data_12bits * ANALOG_VOUT_DIVIDER_RATIO_DEN);
        break;
    case ANALOG_CHANNEL_IOUT_MV:
        // Check calibration.
        if (analog_ctx.ref191_data_12bits == ANALOG_ERROR_VALUE) {
            status = ANALOG_ERROR_CALIBRATION_MISSING;
            goto errors;
        }
        // Output current.
        adc_status = ADC_convert_channel(ANALOG_ADC_CHANNEL_IOUT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = ((int32_t) adc_data_12bits * ANALOG_REF191_VOLTAGE_MV) / ((int32_t) analog_ctx.ref191_data_12bits);
        break;
    default:
        status = ANALOG_ERROR_CHANNEL;
        goto errors;
    }
errors:
    return status;
}
