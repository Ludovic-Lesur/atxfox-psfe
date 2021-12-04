/*
 * lptim.c
 *
 *  Created on: 9 jul. 2019
 *      Author: Ludo
 */

#include "lptim.h"

#include "lptim_reg.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"

/*** LPTIM local macros ***/

#define LPTIM_TIMEOUT_COUNT		1000000
#define LPTIM_DELAY_MS_MIN		1
#define LPTIM_DELAY_MS_MAX		55000

/*** LPTIM local global variables ***/

static unsigned int lptim_clock_frequency_hz = 0;

/* WRITE ARR REGISTER.
 * @param arr_value:	ARR register value to write.
 * @return:				None.
 */
static void LPTIM1_WriteArr(unsigned int arr_value) {
	unsigned int loop_count = 0;
	// Reset bits.
	LPTIM1 -> ICR |= (0b1 << 4);
	LPTIM1 -> ARR &= 0xFFFF0000;
	while (((LPTIM1 -> ISR) & (0b1 << 4)) == 0) {
		// Wait for ARROK='1' or timeout.
		loop_count++;
		if (loop_count > LPTIM_TIMEOUT_COUNT) break;
	}
	// Write new value.
	LPTIM1 -> ICR |= (0b1 << 4);
	LPTIM1 -> ARR |= arr_value;
	loop_count = 0;
	while (((LPTIM1 -> ISR) & (0b1 << 4)) == 0) {
		// Wait for ARROK='1' or timeout.
		loop_count++;
		if (loop_count > LPTIM_TIMEOUT_COUNT) break;
	}
}

/*** LPTIM functions ***/

/* INIT LPTIM FOR DELAY OPERATION.
 * @param lsi_freq_hz:	Effective LSI oscillator frequency.
 * @return:				None.
 */
void LPTIM1_Init(unsigned int lsi_freq_hz) {
	// Disable peripheral.
	RCC -> APB1ENR &= ~(0b1 << 31); // LPTIM1EN='0'.
	// Enable peripheral clock.
	RCC -> CCIPR &= ~(0b11 << 18); // Reset bits 18-19.
	RCC -> CCIPR |= (0b01 << 18); // LPTIMSEL='01' (LSI clock selected).
	lptim_clock_frequency_hz = (lsi_freq_hz >> 5);
	RCC -> APB1ENR |= (0b1 << 31); // LPTIM1EN='1'.
	// Configure peripheral.
	LPTIM1 -> CR &= ~(0b1 << 0); // Disable LPTIM1 (ENABLE='0'), needed to write CFGR.
	LPTIM1 -> CFGR &= ~(0b1 << 0);
	LPTIM1 -> CFGR |= (0b101 << 9); // Prescaler = 32.
	LPTIM1 -> CNT &= 0xFFFF0000; // Reset counter.
	// Clear all flags.
	LPTIM1 -> ICR |= (0b1111111 << 0);
}

/* DELAY FUNCTION.
 * @param delay_ms:	Number of milliseconds to wait.
 * @return:			None.
 */
void LPTIM1_DelayMilliseconds(unsigned int delay_ms) {
	// Clamp value if required.
	unsigned int local_delay_ms = delay_ms;
	if (local_delay_ms > LPTIM_DELAY_MS_MAX) {
		local_delay_ms = LPTIM_DELAY_MS_MAX;
	}
	if (local_delay_ms < LPTIM_DELAY_MS_MIN) {
		local_delay_ms = LPTIM_DELAY_MS_MIN;
	}
	// Enable timer.
	LPTIM1 -> CR |= (0b1 << 0); // Enable LPTIM1 (ENABLE='1').
	// Reset counter.
	LPTIM1 -> CNT &= 0xFFFF0000;
	// Compute ARR value.
	unsigned int arr = ((local_delay_ms * lptim_clock_frequency_hz) / (1000)) & 0x0000FFFF;
	LPTIM1_WriteArr(arr);
	// Clear flag.
	LPTIM1 -> ICR |= (0b1 << 1);
	LPTIM1 -> CR |= (0b1 << 1); // SNGSTRT='1'.
	// Wait for interrupt.
	while (((LPTIM1 -> ISR) & (0b1 << 1)) == 0);
	// Disable timer.
	LPTIM1 -> CR &= ~(0b1 << 0); // Disable LPTIM1 (ENABLE='0').
}
