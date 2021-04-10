/*
 * lptim.c
 *
 *  Created on: 9 juil. 2019
 *      Author: Ludo
 */

#include "lptim.h"

#include "lptim_reg.h"
#include "rcc.h"
#include "rcc_reg.h"

/*** LPTIM functions ***/

/* INIT LPTIM FOR DELAY OPERATION.
 * @param lptim1_use_lsi:	Use APB as clock source if 0, LSI otherwise.
 * @return:					None.
 */
void LPTIM1_Init(void) {
	// Enable clock.
	RCC -> APB1ENR |= (0b1 << 31); // LPTIM1EN='1'.
	// Configure peripheral.
	LPTIM1 -> CR &= ~(0b1 << 0); // Disable LPTIM1 (ENABLE='0'), needed to write CFGR.
	LPTIM1 -> CFGR |= (0b1 << 19); // Enable timeout.
	LPTIM1 -> CNT &= 0xFFFF0000; // Reset counter.
	LPTIM1 -> CR |= (0b1 << 0); // Enable LPTIM1 (ENABLE='1'), needed to write ARR.
	LPTIM1 -> ARR = RCC_GetSysclkKhz(); // Overflow period = 1ms.
	while (((LPTIM1 -> ISR) & (0b1 << 4)) == 0);
	// Clear all flags.
	LPTIM1 -> ICR |= (0b1111111 << 0);
	// Disable peripheral by default.
	LPTIM1 -> CR &= ~(0b1 << 0); // Disable LPTIM1 (ENABLE='0').
}

/* DELAY FUNCTION.
 * @param delay_ms:	Number of milliseconds to wait.
 * @return:			None.
 */
void LPTIM1_DelayMilliseconds(unsigned int delay_ms) {
	// Enable timer.
	LPTIM1 -> CR |= (0b1 << 0); // Enable LPTIM1 (ENABLE='1').
	// Make as many overflows as required.
	unsigned int ms_count = 0;
	unsigned int loop_start_time = 0;
	unsigned char lptim_default = 0;
	for (ms_count=0 ; ms_count<delay_ms ; ms_count++) {
		// Start counter.
		LPTIM1 -> CNT &= 0xFFFF0000;
		LPTIM1 -> CR |= (0b1 << 1); // SNGSTRT='1'.
		while (((LPTIM1 -> ISR) & (0b1 << 1)) == 0);
		// Clear flag.
		LPTIM1 -> ICR |= (0b1 << 1);
		// Exit in case of timeout.
		if (lptim_default != 0) {
			break;
		}
	}
	// Disable timer.
	LPTIM1 -> CR &= ~(0b1 << 0); // Disable LPTIM1 (ENABLE='0').
}
