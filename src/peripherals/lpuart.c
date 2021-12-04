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

#define LPUART_BAUD_RATE 		9600 // Baud rate.
#define LPUART_TIMEOUT_COUNT	100000

/* CONVERTS A 4-BIT WORD TO THE ASCII CODE OF THE CORRESPONDING HEXADECIMAL CHARACTER.
 * @param n:	The word to converts.
 * @return:		The results of conversion.
 */
static char LPUART1_HexaToAscii(unsigned char hexa_value) {
	char hexa_ascii = 0;
	if (hexa_value <= 15) {
		hexa_ascii = (hexa_value <= 9 ? (char) (hexa_value + '0') : (char) (hexa_value + ('A' - 10)));
	}
	return hexa_ascii;
}

/* COMPUTE A POWER A 10.
 * @param power:	The desired power.
 * @return result:	Result of computation.
 */
static unsigned int LPUART1_Pow10(unsigned char power) {
	unsigned int result = 0;
	unsigned int pow10_buf[10] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
	if (power <= 9) {
		result = pow10_buf[power];
	}
	return result;
}

/* FILL LPUART1 TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return:			None.
 */
static void LPUART1_FillTxBuffer(unsigned char tx_byte) {
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
void LPUART1_Init(void) {
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 18); // LPUARTEN='1'.
#ifndef DEBUG
	// Configure TX and RX GPIOs (first as high impedance).
	GPIO_Configure(&GPIO_LPUART1_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_Configure(&GPIO_LPUART1_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
#endif
	// Configure peripheral.
	LPUART1 -> CR1 &= 0xEC008000; // Disable peripheral before configuration (UE='0'), 1 stop bit and 8 data bits (M='00').
	LPUART1 -> CR2 &= 0x00F04FEF; // 1 stop bit (STOP='00').
	LPUART1 -> CR3 &= 0xFF0F0836;
	LPUART1 -> CR3 |= (0b1 << 12); // No overrun detection (OVRDIS='0').
	LPUART1 -> BRR &= 0xFFF00000; // Reset all bits.
	LPUART1 -> BRR |= ((RCC_GetSysclkKhz() * 1000) / (LPUART_BAUD_RATE)) * 256; // BRR = (256*fCK)/(baud rate). See p.730 of RM0377 datasheet.
	// Enable transmitter.
	LPUART1 -> CR1 |= (0b1 << 3); // (TE='1').
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
}

/* SEND A BYTE THROUGH LPUART1.
 * @param byte_to_send:	The byte to send.
 * @param format:		Display format (see ByteDisplayFormat enumeration in usart.h).
 * @param print_prexix	Print '0b' or '0x' prefix is non zero.
 * @return: 			None.
 */
void LPUART1_SendValue(unsigned int tx_value, LPUART_Format format, unsigned char print_prefix) {
	// Local variables.
	unsigned char first_non_zero_found = 0;
	unsigned int idx;
	unsigned char current_value = 0;
	unsigned int current_power = 0;
	unsigned int previous_decade = 0;
	// Fill TX buffer according to format.
	switch (format) {
	case LPUART_FORMAT_BINARY:
		if (print_prefix != 0) {
			// Print "0b" prefix.
			LPUART1_FillTxBuffer('0');
			LPUART1_FillTxBuffer('b');
		}
		// Maximum 32 bits.
		for (idx=31 ; idx>=0 ; idx--) {
			if (tx_value & (0b1 << idx)) {
				LPUART1_FillTxBuffer('1'); // = '1'.
				first_non_zero_found = 1;
			}
			else {
				if ((first_non_zero_found != 0) || (idx == 0)) {
					LPUART1_FillTxBuffer('0'); // = '0'.
				}
			}
			if (idx == 0) {
				break;
			}
		}
		break;
	case LPUART_FORMAT_HEXADECIMAL:
		if (print_prefix != 0) {
			// Print "0x" prefix.
			LPUART1_FillTxBuffer('0');
			LPUART1_FillTxBuffer('x');
		}
		// Maximum 4 bytes.
		for (idx=3 ; idx>=0 ; idx--) {
			current_value = (tx_value & (0xFF << (8*idx))) >> (8*idx);
			if (current_value != 0) {
				first_non_zero_found = 1;
			}
			if ((first_non_zero_found != 0) || (idx == 0)) {
				LPUART1_FillTxBuffer(LPUART1_HexaToAscii((current_value & 0xF0) >> 4));
				LPUART1_FillTxBuffer(LPUART1_HexaToAscii(current_value & 0x0F));
			}
			if (idx == 0) {
				break;
			}
		}
		break;
	case LPUART_FORMAT_DECIMAL:
		// Maximum 10 digits.
		for (idx=9 ; idx>=0 ; idx--) {
			current_power = LPUART1_Pow10(idx);
			current_value = (tx_value - previous_decade) / current_power;
			previous_decade += current_value * current_power;
			if (current_value != 0) {
				first_non_zero_found = 1;
			}
			if ((first_non_zero_found != 0) || (idx == 0)) {
				LPUART1_FillTxBuffer(current_value + '0');
			}
			if (idx == 0) {
				break;
			}
		}
		break;
	case LPUART_FORMAT_ASCII:
		// Raw byte.
		if (tx_value <= 0xFF) {
			LPUART1_FillTxBuffer(tx_value);
		}
		break;
	}
}

/* SEND A BYTE ARRAY THROUGH LPUART1.
 * @param tx_string:	Byte array to send.
 * @return:				None.
 */
void LPUART1_SendString(char* tx_string) {
	// Fill TX buffer with new bytes.
	while (*tx_string) {
		LPUART1_FillTxBuffer((unsigned char) *(tx_string++));
	}
}
