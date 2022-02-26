/*
 * lpuart.c
 *
 *  Created on: 9 juil. 2019
 *      Author: Ludo
 */

#include "lpuart.h"

#include "gpio.h"
#include "lpuart_reg.h"
#include "mode.h"
#include "mapping.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"

/*** LPUART local macros ***/

#define LPUART_BAUD_RATE 		9600
#define LPUART_TIMEOUT_COUNT	100000

/*** LPUART local functions ***/

/* FILL LPUART1 TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return:			None.
 */
static void LPUART1_fill_tx_buffer(unsigned char tx_byte) {
	// Fill transmit register.
	LPUART1 -> TDR = tx_byte;
	// Wait for transmission to complete.
	unsigned int loop_count = 0;
	while (((LPUART1 -> ISR) & (0b1 << 7)) == 0) {
		// Wait for TXE='1' or timeout.
		loop_count++;
		if (loop_count > LPUART_TIMEOUT_COUNT) break;
	}
}

/*** LPUART functions ***/

/* CONFIGURE LPUART1.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_init(void) {
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 18); // LPUARTEN='1'.
#ifndef DEBUG
	// Configure TX and RX GPIOs (first as high impedance).
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
#endif
	// Configure peripheral.
	LPUART1 -> CR1 &= 0xEC008000; // Disable peripheral before configuration (UE='0'), 1 stop bit and 8 data bits (M='00').
	LPUART1 -> CR2 &= 0x00F04FEF; // 1 stop bit (STOP='00').
	LPUART1 -> CR3 &= 0xFF0F0836;
	LPUART1 -> CR3 |= (0b1 << 12); // No overrun detection (OVRDIS='0').
	LPUART1 -> BRR &= 0xFFF00000; // Reset all bits.
	LPUART1 -> BRR |= ((RCC_get_sysclk_khz() * 1000) / (LPUART_BAUD_RATE)) * 256; // BRR = (256*fCK)/(baud rate). See p.730 of RM0377 datasheet.
	// Enable transmitter.
	LPUART1 -> CR1 |= (0b1 << 3); // (TE='1').
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
}

/* SEND A BYTE ARRAY THROUGH LPUART1.
 * @param tx_string:	Byte array to send.
 * @return:				None.
 */
void LPUART1_send_string(char* tx_string) {
	// Fill TX buffer with new bytes.
	while (*tx_string) {
		LPUART1_fill_tx_buffer((unsigned char) *(tx_string++));
	}
}
