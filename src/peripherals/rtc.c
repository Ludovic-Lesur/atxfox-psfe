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
 * @param:						None.
 * @return rtc_initf_success:	1 if RTC entered initialization mode, 0 otherwise.
 */
static unsigned char RTC_EnterInitializationMode(void) {
	// Local variables.
	unsigned char rtc_initf_success = 1;
	// Enter key.
	RTC -> WPR = 0xCA;
	RTC -> WPR = 0x53;
	RTC -> ISR |= (0b1 << 7); // INIT='1'.
	unsigned int loop_count = 0;
	while (((RTC -> ISR) & (0b1 << 6)) == 0) {
		// Wait for INITF='1' or timeout.
		if (loop_count > RTC_INIT_TIMEOUT_COUNT) {
			rtc_initf_success = 0;
			break;
		}
		loop_count++;
	}
	return rtc_initf_success;
}

/* EXIT INITIALIZATION MODE TO PROTECT RTC REGISTERS.
 * @param:	None.
 * @return:	None.
 */
static void RTC_ExitInitializationMode(void) {
	RTC -> ISR &= ~(0b1 << 7); // INIT='0'.
}

/*** RTC functions ***/

/* RESET RTC PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void RTC_Reset(void) {
	// Reset RTC peripheral.
	RCC -> CSR |= (0b1 << 19); // RTCRST='1'.
	unsigned char j = 0;
	for (j=0 ; j<100 ; j++) {
		// Poll a bit always read as '0'.
		// This is required to avoid for loop removal by compiler (size optimization for HW1.0).
		if (((RCC -> CR) & (0b1 << 24)) != 0) {
			break;
		}
	}
	RCC -> CSR &= ~(0b1 << 19); // RTCRST='0'.
}

/* INIT HARDWARE RTC PERIPHERAL.
 * @param lsi_freq_hz:	Effective LSI oscillator frequency used to compute the accurate prescaler value (only if LSI is used as source).
 * @return:				None.
 */
void RTC_Init(unsigned int lsi_freq_hz) {
	// Use LSI clock.
	RCC -> CSR |= (0b10 << 16); // RTCSEL='10'.
	// Enable RTC and register access.
	RCC -> CSR |= (0b1 << 18); // RTCEN='1'.
	// Switch to LSI if RTC failed to enter initialization mode.
	if (RTC_EnterInitializationMode() == 0) {
		RTC_Reset();
		RTC_EnterInitializationMode();
	}
	// Compute prescaler according to measured LSI frequency.
	RTC -> PRER = (127 << 16) | (((lsi_freq_hz / 128) - 1) << 0); // PREDIV_A=127 and PREDIV_S=((lsi_freq_hz/128)-1).
	// Bypass shadow registers.
	RTC -> CR |= (0b1 << 5); // BYPSHAD='1'.
	// Configure wake-up timer.
	RTC -> CR &= ~(0b1 << 10); // Disable wake-up timer.
	RTC -> CR &= ~(0b111 << 0);
	RTC -> CR |= (0b100 << 0); // Wake-up timer clocked by RTC clock (1Hz).
	RTC_ExitInitializationMode();
	// Disable interrupt and clear all flags.
	RTC -> CR &= ~(0b1 << 14);
	RTC -> ISR &= 0xFFFE0000;
}

/* START RTC WAKE-UP TIMER.
 * @param delay_seconds:	Delay in seconds.
 * @return:					None.
 */
void RTC_StartWakeUpTimer(unsigned int delay_seconds) {
	// Clamp parameter.
	unsigned int local_delay_seconds = delay_seconds;
	if (local_delay_seconds > RTC_WAKEUP_TIMER_DELAY_MAX) {
		local_delay_seconds = RTC_WAKEUP_TIMER_DELAY_MAX;
	}
	// Check if timer si not allready running.
	if (((RTC -> CR) & (0b1 << 10)) == 0) {
		// Enable RTC and register access.
		RTC_EnterInitializationMode();
		// Configure wake-up timer.
		RTC -> CR &= ~(0b1 << 10); // Disable wake-up timer.
		RTC -> WUTR = (local_delay_seconds - 1);
		// Clear flags.
		RTC -> ISR &= ~(0b1 << 10); // WUTF='0'.
		// Enable interrupt.
		RTC -> CR |= (0b1 << 14); // WUTE='1'.
		// Start timer.
		RTC -> CR |= (0b1 << 10); // Enable wake-up timer.
		RTC_ExitInitializationMode();
	}
}

/* RETURN THE CURRENT ALARM INTERRUPT STATUS.
 * @param:	None.
 * @return:	1 if the RTC interrupt occured, 0 otherwise.
 */
volatile unsigned char RTC_GetWakeUpTimerFlag(void) {
	return (((RTC -> ISR) >> 10) & 0b1);
}

/* CLEAR ALARM A INTERRUPT FLAG.
 * @param:	None.
 * @return:	None.
 */
void RTC_ClearWakeUpTimerFlag(void) {
	// Clear flag.
	RTC -> ISR &= ~(0b1 << 10); // WUTF='0'.
}
