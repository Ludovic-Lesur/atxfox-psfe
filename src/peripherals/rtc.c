/*
 * rtc.c
 *
 *  Created on: 16 aug. 2020
 *      Author: Ludo
 */

#include "rtc.h"

#include "error.h"
#include "exti.h"
#include "nvic.h"
#include "rcc_reg.h"
#include "rtc_reg.h"
#include "types.h"

/*** RTC local macros ***/

#define RTC_INIT_TIMEOUT_COUNT	1000000

/*** RTC local global variables ***/

static volatile uint8_t rtc_wakeup_timer_flag = 0;
static volatile uint32_t rtc_time_seconds = 0;

/*** RTC local functions ***/

/*******************************************************************/
void __attribute__((optimize("-O0"))) RTC_IRQHandler(void) {
	// Wake-up timer interrupt.
	if (((RTC -> ISR) & (0b1 << 10)) != 0) {
		// Set local flag.
		if (((RTC -> CR) & (0b1 << 14)) != 0) {
			rtc_wakeup_timer_flag = 1;
			rtc_time_seconds += RTC_WAKEUP_PERIOD_SECONDS;
		}
		// Clear RTC and EXTI flags.
		RTC -> ISR &= ~(0b1 << 10); // WUTF='0'.
		EXTI_clear_flag(EXTI_LINE_RTC_WAKEUP_TIMER);
	}
}

/*******************************************************************/
static RTC_status_t __attribute__((optimize("-O0"))) _RTC_enter_initialization_mode(void) {
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

/*******************************************************************/
static void __attribute__((optimize("-O0"))) _RTC_exit_initialization_mode(void) {
	RTC -> ISR &= ~(0b1 << 7); // INIT='0'.
}

/*** RTC functions ***/

/*******************************************************************/
RTC_status_t RTC_init(void) {
	// Local variables.
	RTC_status_t status = RTC_SUCCESS;
	RCC_status_t rcc_status = RCC_SUCCESS;
	uint32_t lsi_frequency_hz = 0;
	uint32_t loop_count = 0;
	// Use LSI.
	RCC -> CSR &= ~(0b11 << 16); // Reset bits.
	RCC -> CSR |= (0b10 << 16); // RTCSEL='10'.
	// Enable RTC and register access.
	RCC -> CSR |= (0b1 << 18); // RTCEN='1'.
	// Enter initialization mode.
	status = _RTC_enter_initialization_mode();
	if (status != RTC_SUCCESS) goto errors;
	// Poll WUTWF flag before accessing reload register.
	while (((RTC -> ISR) & (0b1 << 2)) == 0) {
		// Wait for WUTWF='1' or timeout.
		loop_count++;
		if (loop_count > RTC_INIT_TIMEOUT_COUNT) {
			status = RTC_ERROR_WAKEUP_TIMER_REGISTER_ACCESS;
			goto errors;
		}
	}
	// Get effective LSI frequency.
	rcc_status = RCC_measure_lsi_frequency(&lsi_frequency_hz);
	RCC_stack_error();
	// Compute prescaler according to measured LSI frequency.
	RTC -> PRER = (127 << 16) | (((lsi_frequency_hz >> 7) - 1) << 0);
	// Configure wake-up timer.
	RTC -> WUTR = (RTC_WAKEUP_PERIOD_SECONDS - 1);
	// Clear flags.
	RTC -> ISR &= 0xFFFF005F;
	// Configure interrupt.
	EXTI_configure_line(EXTI_LINE_RTC_WAKEUP_TIMER, EXTI_TRIGGER_RISING_EDGE);
	NVIC_enable_interrupt(NVIC_INTERRUPT_RTC, NVIC_PRIORITY_RTC);
	// Enable wake-up timer clocked by RTC clock (1Hz).
	RTC -> CR = 0x00004424;
errors:
	_RTC_exit_initialization_mode();
	return status;
}

/*******************************************************************/
volatile uint8_t RTC_get_wakeup_timer_flag(void) {
	return rtc_wakeup_timer_flag;
}

/*******************************************************************/
void RTC_clear_wakeup_timer_flag(void) {
	// Clear flag.
	rtc_wakeup_timer_flag = 0;
}

/*******************************************************************/
uint32_t RTC_get_time_seconds(void) {
	return rtc_time_seconds;
}
