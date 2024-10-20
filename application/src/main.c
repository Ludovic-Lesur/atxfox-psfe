/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

// Peripherals.
#include "exti.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "mode.h"
#include "nvic_priority.h"
#include "pwr.h"
#include "rcc.h"
#include "rtc.h"
// Utils.
#include "error.h"
// Middleware
#include "analog.h"
#include "hmi.h"
#include "serial.h"
#include "sigfox.h"
// Applicative.
#include "error_base.h"
#include "mode.h"

/*** MAIN local macros ***/

#define PSFE_VMCU_THRESHOLD_ON_MV       3300
#define PSFE_VMCU_THRESHOLD_OFF_MV      3100

/*** MAIN local functions ***/

/*******************************************************************/
static void _PSFE_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	RTC_status_t rtc_status = RTC_SUCCESS;
	ANALOG_status_t analog_status = ANALOG_SUCCESS;
	HMI_status_t hmi_status = HMI_SUCCESS;
#ifndef DEBUG
    IWDG_status_t iwdg_status = IWDG_SUCCESS;
#endif
#ifdef PSFE_SERIAL_MONITORING
	SERIAL_status_t serial_status = SERIAL_SUCCESS;
#endif
#ifdef PSFE_SIGFOX_MONITORING
	SIGFOX_status_t sigfox_status = SIGFOX_SUCCESS;
#endif
	// Init memory.
	NVIC_init();
	// Init power module and clock tree.
	PWR_init();
	rcc_status = RCC_init(NVIC_PRIORITY_CLOCK);
	RCC_stack_error(ERROR_BASE_RCC);
	// Init GPIOs.
    GPIO_init();
    EXTI_init();
#ifndef DEBUG
    // Start independent watchdog.
    iwdg_status = IWDG_init();
    IWDG_stack_error(ERROR_BASE_IWDG);
#endif
    // High speed oscillator.
	rcc_status = RCC_switch_to_hsi();
	RCC_stack_error(ERROR_BASE_RCC);
	// Calibrate clocks.
	rcc_status = RCC_calibrate_internal_clocks(NVIC_PRIORITY_CLOCK_CALIBRATION);
	RCC_stack_error(ERROR_BASE_RCC);
	// Init RTC.
	rtc_status = RTC_init(NULL, NVIC_PRIORITY_RTC);
	RTC_stack_error(ERROR_BASE_RTC);
	// Init delay timer.
	LPTIM_init(NVIC_PRIORITY_DELAY);
	// Init middleware modules.
	analog_status = ANALOG_init();
	ANALOG_stack_error(ERROR_BASE_ANALOG);
	hmi_status = HMI_init();
	HMI_stack_error(ERROR_BASE_HMI);
#ifdef PSFE_SERIAL_MONITORING
	serial_status = SERIAL_init();
	SERIAL_stack_error(ERROR_BASE_SERIAL);
#endif
#ifdef PSFE_SIGFOX_MONITORING
	sigfox_status = SIGFOX_init();
	SIGFOX_stack_error(ERROR_BASE_SIGFOX);
#endif
}

/*** MAIN function ***/

/*******************************************************************/
int main(void) {
    // Local variables.
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    HMI_status_t hmi_status = HMI_SUCCESS;
#ifdef PSFE_SERIAL_MONITORING
    SERIAL_status_t serial_status = SERIAL_SUCCESS;
#endif
#ifdef PSFE_SIGFOX_MONITORING
    SIGFOX_status_t sigfox_status = SIGFOX_SUCCESS;
#endif
    uint8_t board_state = 0;
    int32_t vmcu_mv = 0;
	// Init board.
	_PSFE_init_hw();
	// Main loop.
	while (1) {
	    // Reload watchdog.
	    IWDG_reload();
	    // Process modules.
        analog_status = ANALOG_process();
        ANALOG_stack_error(ERROR_BASE_ANALOG);
#ifdef PSFE_SERIAL_MONITORING
        serial_status = SERIAL_process();
        SERIAL_stack_error(ERROR_BASE_SERIAL);
#endif
#ifdef PSFE_SIGFOX_MONITORING
        sigfox_status = SIGFOX_process();
        SIGFOX_stack_error(ERROR_BASE_SIGFOX);
#endif
	    // Check board power supply.
	    analog_status = ANALOG_read_channel(ANALOG_CHANNEL_VMCU_MV, &vmcu_mv);
	    ANALOG_stack_error(ERROR_BASE_ANALOG);
	    // Manage modules state.
	    if ((vmcu_mv < PSFE_VMCU_THRESHOLD_OFF_MV) && (board_state != 0)) {
	        // Stop TRCS board.
	        analog_status = ANALOG_stop_trcs();
	        ANALOG_stack_error(ERROR_BASE_ANALOG);
	        // Stop front-end.
	        hmi_status = HMI_stop();
	        HMI_stack_error(ERROR_BASE_HMI);
#ifdef PSFE_SERIAL_MONITORING
	        serial_status = SERIAL_stop();
	        SERIAL_stack_error(ERROR_BASE_SERIAL);
#endif
#ifdef PSFE_SIGFOX_MONITORING
            sigfox_status = SIGFOX_stop();
            SIGFOX_stack_error(ERROR_BASE_SIGFOX);
#endif
            // Update state.
            board_state = 0;
	    }
	    if ((vmcu_mv > PSFE_VMCU_THRESHOLD_ON_MV) && (board_state == 0)) {
	        // Stop TRCS board.
            analog_status = ANALOG_start_trcs();
            ANALOG_stack_error(ERROR_BASE_ANALOG);
	        // Start front-end.
	        hmi_status = HMI_start();
            HMI_stack_error(ERROR_BASE_HMI);
#ifdef PSFE_SERIAL_MONITORING
            serial_status = SERIAL_start();
            SERIAL_stack_error(ERROR_BASE_SERIAL);
#endif
#ifdef PSFE_SIGFOX_MONITORING
            sigfox_status = SIGFOX_start();
            SIGFOX_stack_error(ERROR_BASE_SIGFOX);
#endif
            // Update state.
           board_state = 1;
        }
	}
	return 0;
}
