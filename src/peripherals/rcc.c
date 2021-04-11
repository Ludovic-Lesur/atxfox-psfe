/*
 * rcc.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "rcc.h"

#include "flash.h"
#include "rcc_reg.h"

/*** RCC local macros ***/

#define RCC_MSI_RESET_FREQUENCY_KHZ		2100
#define RCC_TIMEOUT_COUNT				10

/*** RCC local global variables ***/

static unsigned int rcc_sysclk_khz;

/*** RCC local functions ***/

/* PERFORM A MANUAL DELAY.
 * @param:	None.
 * @return:	None.
 */
static void RCC_Delay(void) {
	unsigned int j = 0;
	unsigned int loop_count = (19 * rcc_sysclk_khz) / 3; // Value for 100ms.
	for (j=0 ; j<loop_count ; j++) {
		// Poll a bit always read as '0'.
		// This is required to avoid for loop removal by compiler (size optimization for HW1.0).
		if (((RCC -> CR) & (0b1 << 24)) != 0) {
			break;
		}
	}
}

/*** RCC functions ***/

/* CONFIGURE PERIPHERALs CLOCK PRESCALER AND SOURCES.
 * @param:	None.
 * @return:	None.
 */
void RCC_Init(void) {
	// Prescalers (HCLK, PCLK1 and PCLK2 must not exceed 32MHz).
	RCC -> CFGR &= ~(0b1111 << 4); // HCLK = SYSCLK = 16MHz (HPRE='0000').
	RCC -> CFGR &= ~(0b111 << 8); // PCLK1 = HCLK = 16MHz (PPRE1='000').
	RCC -> CFGR &= ~(0b111 << 11); // PCLK2 = HCLK = 16MHz (PPRE2='000').
	// Peripherals clock source.
	RCC -> CCIPR &= 0xFFF0C3F0; // All peripherals clocked via the corresponding APBx line.
	// Reset clock is MSI 2.1MHz.
	rcc_sysclk_khz = RCC_MSI_RESET_FREQUENCY_KHZ;
}

/* RETURN THE CURRENT SYSTEM CLOCK FREQUENCY.
 * @param:					None.
 * @return rcc_sysclk_khz:	Current system clock frequency in kHz.
 */
unsigned int RCC_GetSysclkKhz(void) {
	return rcc_sysclk_khz;
}

/* CONFIGURE AND USE HSI AS SYSTEM CLOCK (16MHz INTERNAL RC).
 * @param:					None.
 * @return sysclk_on_hsi:	'1' if SYSCLK source was successfully switched to HSI, 0 otherwise.
 */
unsigned char RCC_SwitchToHsi(void) {
	// Set flash latency.
	FLASH_SetLatency(1);
	// Init HSI.
	RCC -> CR |= (0b1 << 0); // Enable HSI (HSI16ON='1').
	// Wait for HSI to be stable.
	unsigned char sysclk_on_hsi = 0;
	unsigned int count = 0;
	while ((((RCC -> CR) & (0b1 << 2)) == 0) && (count < RCC_TIMEOUT_COUNT)) {
		RCC_Delay();
		count++; // Wait for HSIRDYF='1' or timeout.
	}
	// Check timeout.
	if (count < RCC_TIMEOUT_COUNT) {
		// Switch SYSCLK.
		RCC -> CFGR &= ~(0b11 << 0); // Reset bits 0-1.
		RCC -> CFGR |= (0b01 << 0); // Use HSI as system clock (SW='01').
		// Wait for clock switch.
		count = 0;
		while ((((RCC -> CFGR) & (0b11 << 2)) != (0b01 << 2)) && (count < RCC_TIMEOUT_COUNT)) {
			RCC_Delay();
			count++; // Wait for SWS='01' or timeout.
		}
		// Check timeout.
		if (count < RCC_TIMEOUT_COUNT) {
			// Disable MSI and HSE.
			RCC -> CR &= ~(0b1 << 8); // Disable MSI (MSION='0').
			RCC -> CR &= ~(0b1 << 16); // Disable HSE (HSEON='0').
			// Update flag and frequency.
			sysclk_on_hsi = 1;
			rcc_sysclk_khz = RCC_HSI_FREQUENCY_KHZ;
		}
	}
	return sysclk_on_hsi;
}

/* CONFIGURE AND USE LSI AS LOW SPEED OSCILLATOR (32kHz INTERNAL RC).
 * @param:					None.
 * @return lsi_available:	'1' if LSI was successfully started, 0 otherwise.
 */
unsigned char RCC_EnableLsi(void) {
	// Enable LSI.
	RCC -> CSR |= (0b1 << 0); // LSION='1'.
	// Wait for LSI to be stable.
	unsigned char lsi_available = 0;
	unsigned int count = 0;
	while ((((RCC -> CSR) & (0b1 << 1)) == 0) && (count < RCC_TIMEOUT_COUNT)) {
		RCC_Delay();
		count++; // Wait for LSIRDY='1'.
	}
	// Check timeout.
	if (count < RCC_TIMEOUT_COUNT) {
		// Update flag.
		lsi_available = 1;
	}
	return lsi_available;
}
