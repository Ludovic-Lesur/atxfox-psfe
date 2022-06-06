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

#define LPUART_BAUD_RATE 			9600
#define LPUART_TIMEOUT_COUNT		100000
#define LPUART_STRING_LENGTH_MAX	1000

/*** LPUART local functions ***/

/* FILL LPUART1 TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return status:	Function execution status.
 */
static LPUART_status_t LPUART1_fill_tx_buffer(unsigned char tx_byte) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	unsigned int loop_count = 0;
	// Fill transmit register.
	LPUART1 -> TDR = tx_byte;
	// Wait for transmission to complete.
	while (((LPUART1 -> ISR) & (0b1 << 7)) == 0) {
		// Wait for TXE='1' or timeout.
		loop_count++;
		if (loop_count > LPUART_TIMEOUT_COUNT) {
			status = LPUART_ERROR_TX_TIMEOUT;
			goto errors;
		}
	}
errors:
	return status;
}

/*** LPUART functions ***/

/* CONFIGURE LPUART1.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_init(void) {
	// Local variables.
	unsigned int brr = 0;
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 18); // LPUARTEN='1'.
#ifndef DEBUG
	// Configure TX and RX GPIOs (first as high impedance).
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
#endif
	// Configure peripheral.
	LPUART1 -> CR3 |= (0b1 << 12); // No overrun detection (OVRDIS='0').
	LPUART1 -> BRR &= 0xFFF00000; // Reset all bits.
	brr = (RCC_get_sysclk_khz() * 1000 * 256);
	brr /= LPUART_BAUD_RATE;
	LPUART1 -> BRR = (brr & 0x000FFFFF); // BRR = (256*fCK)/(baud rate). See p.730 of RM0377 datasheet.
	// Enable transmitter.
	LPUART1 -> CR1 |= (0b1 << 3); // (TE='1').
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
}

/* SEND A BYTE ARRAY THROUGH LPUART1.
 * @param tx_string:	Byte array to send.
 * @return status:		Function execution status.
 */
LPUART_status_t LPUART1_send_string(char* tx_string) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	unsigned int char_count = 0;
	// Loop on all characters.
	while (*tx_string) {
		// Fill transmit register.
		status = LPUART1_fill_tx_buffer(*(tx_string++));
		if (status != LPUART_SUCCESS) goto errors;
		// Check char count.
		char_count++;
		if (char_count > LPUART_STRING_LENGTH_MAX) {
			status = LPUART_ERROR_STRING_LENGTH;
			goto errors;
		}
	}
errors:
	return status;
}
