/*
 * rtc.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "rtc.h"

#include "exti.h"
#include "exti_reg.h"
#include "mode.h"
#include "nvic.h"
#include "rcc_reg.h"
#include "rtc_reg.h"

/*** RTC local macros ***/

#define RTC_INIT_TIMEOUT_COUNT		1000
#define RTC_WAKEUP_TIMER_DELAY_MAX	65536

/*** RTC local functions ***/

/* ENTER INITIALIZATION MODE TO ENABLE RTC REGISTERS UPDATE.
 * @param:			None.
 * @return status:	Function execution status.
 */
static RTC_status_t __attribute__((optimize("-O0"))) RTC_enter_initialization_mode(void) {
	// Local variables.
	RTC_status_t status = RTC_SUCCESS;
	uint32_t loop_count = 0;
	// Enter key.
	RTC -> WPR = 0xCA;
	RTC -> WPR = 0x53;
	RTC -> ISR |= (0b1 << 7); // INIT='1'.
	// Wait for initialization mode.
	while (((RTC -> ISR) & (0b1 << 6)) == 0) {
		// Wait for INITF='1' or timeout.
		loop_count++;
		if (loop_count > RTC_INIT_TIMEOUT_COUNT) {
			status = RTC_ERROR_INITIALIZATION_MODE;
			break;
		}
	}
	return status;
}

/* EXIT INITIALIZATION MODE TO PROTECT RTC REGISTERS.
 * @param:	None.
 * @return:	None.
 */
static void __attribute__((optimize("-O0"))) RTC_exit_initialization_mode(void) {
	RTC -> ISR &= ~(0b1 << 7); // INIT='0'.
}

/*** RTC functions ***/

/* RESET RTC PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) RTC_reset(void) {
	// Local variables.
	uint8_t j = 0;
	// Reset RTC peripheral.
	RCC -> CSR |= (0b1 << 19); // RTCRST='1'.
	for (j=0 ; j<100 ; j++);
	RCC -> CSR &= ~(0b1 << 19); // RTCRST='0'.
}

/* INIT HARDWARE RTC PERIPHERAL.
 * @param lsi_freq_hz:	Effective LSI oscillator frequency used to compute the accurate prescaler value (only if LSI is used as source).
 * @return status:		Function execution status.
 */
RTC_status_t __attribute__((optimize("-O0"))) RTC_init(uint32_t lsi_freq_hz) {
	// Local variables.
	RTC_status_t status = RTC_SUCCESS;
	// Use LSI clock.
	RCC -> CSR |= (0b10 << 16); // RTCSEL='10'.
	// Enable RTC and register access.
	RCC -> CSR |= (0b1 << 18); // RTCEN='1'.
	// Switch to LSI if RTC failed to enter initialization mode.
	status = RTC_enter_initialization_mode();
	if (status != RTC_SUCCESS) {
		RTC_reset();
		status = RTC_enter_initialization_mode();
		if (status != RTC_SUCCESS) goto errors;
	}
	// Compute prescaler according to measured LSI frequency.
	RTC -> PRER = (127 << 16) | (((lsi_freq_hz / 128) - 1) << 0); // PREDIV_A=127 and PREDIV_S=((lsi_freq_hz/128)-1).
	// Bypass shadow registers.
	RTC -> CR |= (0b1 << 5); // BYPSHAD='1'.
	// Configure wake-up timer.
	RTC -> CR |= (0b100 << 0); // Wake-up timer clocked by RTC clock (1Hz).
	RTC_exit_initialization_mode();
	// Disable interrupts and clear all flags.
	RTC -> ISR &= 0xFFFE0000;
errors:
	return status;
}

/* START RTC WAKE-UP TIMER.
 * @param delay_seconds:	Delay in seconds.
 * @return status:			Function execution status.
 */
RTC_status_t RTC_start_wakeup_timer(uint32_t delay_seconds) {
	// Local variables.
	RTC_status_t status = RTC_SUCCESS;
	// Check parameter.
	if (delay_seconds > RTC_WAKEUP_TIMER_DELAY_MAX) {
		status = RTC_ERROR_WAKEUP_TIMER_DELAY;
		goto errors;
	}
	// Check if timer si not allready running.
	if (((RTC -> CR) & (0b1 << 10)) != 0) {
		status = RTC_ERROR_WAKEUP_TIMER_RUNNING;
		goto errors;
	}
	// Enable RTC and register access.
	status = RTC_enter_initialization_mode();
	if (status != RTC_SUCCESS) goto errors;
	// Configure wake-up timer.
	RTC -> WUTR = (delay_seconds - 1);
	// Clear flags.
	RTC_clear_wakeup_timer_flag();
	// Enable interrupt.
	RTC -> CR |= (0b1 << 14); // WUTE='1'.
	// Start timer.
	RTC -> CR |= (0b1 << 10); // Enable wake-up timer.
	RTC_exit_initialization_mode();
errors:
	return status;
}

/* STOP RTC WAKE-UP TIMER.
 * @param:			None.
 * @return status:	Function execution status.
 */
RTC_status_t RTC_stop_wakeup_timer(void) {
	// Local variables.
	RTC_status_t status = RTC_SUCCESS;
	// Enable RTC and register access.
	status = RTC_enter_initialization_mode();
	if (status != RTC_SUCCESS) goto errors;
	// Disable wake-up timer.
	RTC -> CR &= ~(0b1 << 10);
	RTC_exit_initialization_mode();
	// Disable interrupt.
	RTC -> CR &= ~(0b1 << 14); // WUTE='0'.
errors:
	return status;
}

/* RETURN THE CURRENT ALARM INTERRUPT STATUS.
 * @param:	None.
 * @return:	1 if the RTC interrupt occured, 0 otherwise.
 */
volatile uint8_t RTC_get_wakeup_timer_flag(void) {
	return (((RTC -> ISR) >> 10) & 0b1);
}

/* CLEAR ALARM A INTERRUPT FLAG.
 * @param:	None.
 * @return:	None.
 */
void RTC_clear_wakeup_timer_flag(void) {
	// Clear flag.
	RTC -> ISR &= ~(0b1 << 10); // WUTF='0'.
}
