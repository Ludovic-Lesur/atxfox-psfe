/*
 * trcs_hw.c
 *
 *  Created on: 17 oct. 2024
 *      Author: Ludo
 */

#include "trcs_hw.h"

#ifndef TRCS_DRIVER_DISABLE_FLAGS_FILE
#include "trcs_driver_flags.h"
#endif
#include "analog.h"
#include "error.h"
#include "gpio.h"
#include "gpio_mapping.h"
#include "math.h"
#include "nvic_priority.h"
#include "tim.h"
#include "trcs.h"

#ifndef TRCS_DRIVER_DISABLE

/*** TRCS HW local macros ***/

#define TRCS_HW_TIMER_INSTANCE  TIM_INSTANCE_TIM22

/*** TRCS HW local global variables ***/

static const GPIO_pin_t* TRCS_HW_GPIO_RANGE[TRCS_RANGE_LAST] = { &GPIO_TRCS_RANGE_LOW, &GPIO_TRCS_RANGE_MIDDLE, &GPIO_TRCS_RANGE_HIGH };

/*** TRCS HW functions ***/

/*******************************************************************/
TRCS_status_t TRCS_HW_init(void) {
    // Local variables.
    TRCS_status_t status = TRCS_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    uint8_t idx = 0;
    // Init GPIOs.
    for (idx = 0; idx < TRCS_RANGE_LAST; idx++) {
        GPIO_configure(TRCS_HW_GPIO_RANGE[idx], GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
    }
    // Init sampling timer.
    tim_status = TIM_STD_init(TRCS_HW_TIMER_INSTANCE, NVIC_PRIORITY_TRCS_TIMER);
    TIM_exit_error(TRCS_ERROR_BASE_TIMER);
errors:
    return status;
}

/*******************************************************************/
TRCS_status_t TRCS_HW_de_init(void) {
    // Local variables.
   TRCS_status_t status = TRCS_SUCCESS;
   TIM_status_t tim_status = TIM_SUCCESS;
   // Release sampling timer.
   tim_status = TIM_STD_de_init(TRCS_HW_TIMER_INSTANCE);
   TIM_exit_error(TRCS_ERROR_BASE_TIMER);
errors:
    return status;
}

/*******************************************************************/
TRCS_status_t TRCS_HW_timer_start(uint32_t period_ms, TRCS_HW_timer_irq_cb_t irq_callback) {
    // Local variables.
    TRCS_status_t status = TRCS_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Start sampling timer.
    tim_status = TIM_STD_start(TRCS_HW_TIMER_INSTANCE, (period_ms * MATH_POWER_10[6]), irq_callback);
    TIM_exit_error(TRCS_ERROR_BASE_TIMER);
errors:
    return status;
}

/*******************************************************************/
TRCS_status_t TRCS_HW_timer_stop(void) {
    // Local variables.
    TRCS_status_t status = TRCS_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Stop sampling timer.
    tim_status = TIM_STD_stop(TRCS_HW_TIMER_INSTANCE);
    TIM_exit_error(TRCS_ERROR_BASE_TIMER);
errors:
    return status;
}

/*******************************************************************/
TRCS_status_t TRCS_HW_set_range_state(TRCS_range_t range, uint8_t state) {
    // Local variables.
    TRCS_status_t status = TRCS_SUCCESS;
    // Check parameter.
    if (range >= TRCS_RANGE_LAST) {
        status = TRCS_ERROR_RANGE;
        goto errors;
    }
    // Set GPIO.
    GPIO_write(TRCS_HW_GPIO_RANGE[range], state);
errors:
    return status;
}

/*******************************************************************/
TRCS_status_t TRCS_HW_adc_get_iout(int32_t* iout_mv) {
    // Local variables.
    TRCS_status_t status = TRCS_SUCCESS;
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    // Read data.
    analog_status = ANALOG_read_channel(ANALOG_CHANNEL_IOUT_MV, iout_mv);
    ANALOG_exit_error(TRCS_ERROR_BASE_ADC);
errors:
    return status;
}

#endif /* TRCS_DRIVER_DISABLE */
