/*
 * lpuart.c
 *
 *  Created on: 9 juil. 2019
 *      Author: Ludo
 */

#include "lpuart.h"

#include "atxfox.h"
#include "gpio.h"
#include "lpuart_reg.h"
#include "mapping.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"

/*** LPUART local macros ***/

#define LPUART_BAUD_RATE 		9600 // Baud rate.
#define LPUART_TX_BUFFER_SIZE	128 // TX buffer size.

/*** LPUART local structures ***/

typedef struct {
	unsigned char tx_buf[LPUART_TX_BUFFER_SIZE]; 	// Transmit buffer.
	unsigned int tx_buf_read_idx; 					// Reading index in TX buffer.
	unsigned int tx_buf_write_idx; 					// Writing index in TX buffer.
} LPUART_Context;

/*** LPUART local global variables ***/

static volatile LPUART_Context lpuart_ctx;

/*** LPUART local functions ***/

/* LPUART1 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_IRQHandler(void) {
	// TXE interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 7)) != 0) {
		if ((lpuart_ctx.tx_buf_read_idx) != (lpuart_ctx.tx_buf_write_idx)) {
			LPUART1 -> TDR = lpuart_ctx.tx_buf[lpuart_ctx.tx_buf_read_idx]; // Fill transmit data register with new byte.
			lpuart_ctx.tx_buf_read_idx++; // Increment TX read index.
			if (lpuart_ctx.tx_buf_read_idx == LPUART_TX_BUFFER_SIZE) {
				lpuart_ctx.tx_buf_read_idx = 0; // Manage roll-over.
			}
		}
		else {
			// No more bytes, disable TXE interrupt.
			LPUART1 -> CR1 &= ~(0b1 << 7); // TXEIE='0'.
		}
	}
	// Overrun error interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 3)) != 0) {
		// Clear ORE flag.
		LPUART1 -> ICR |= (0b1 << 3);
	}
}

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
	// Fill buffer.
	lpuart_ctx.tx_buf[lpuart_ctx.tx_buf_write_idx] = tx_byte;
	// Manage index roll-over.
	lpuart_ctx.tx_buf_write_idx++;
	if (lpuart_ctx.tx_buf_write_idx == LPUART_TX_BUFFER_SIZE) {
		lpuart_ctx.tx_buf_write_idx = 0;
	}
}

/*** LPUART functions ***/

/* CONFIGURE LPUART1.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_Init(void) {
	// Init context.
	unsigned int idx = 0;
	for (idx=0 ; idx<LPUART_TX_BUFFER_SIZE ; idx++) lpuart_ctx.tx_buf[idx] = 0;
	lpuart_ctx.tx_buf_write_idx = 0;
	lpuart_ctx.tx_buf_read_idx = 0;
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
	// Set interrupt priority.
	NVIC_SetPriority(NVIC_IT_LPUART1, 3);
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
	// Disable interrupt.
	NVIC_DisableInterrupt(NVIC_IT_LPUART1);
	/* Local variables */
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
	// Enable interrupt.
	LPUART1 -> CR1 |= (0b1 << 7); // (TXEIE = '1').
	NVIC_EnableInterrupt(NVIC_IT_LPUART1);
}

/* SEND A BYTE ARRAY THROUGH LPUART1.
 * @param tx_string:	Byte array to send.
 * @return:				None.
 */
void LPUART1_SendString(char* tx_string) {
	// Disable interrupt.
	NVIC_DisableInterrupt(NVIC_IT_LPUART1);
	// Fill TX buffer with new bytes.
	while (*tx_string) {
		LPUART1_FillTxBuffer((unsigned char) *(tx_string++));
	}
	// Enable interrupt.
	LPUART1 -> CR1 |= (0b1 << 7); // (TXEIE = '1').
	NVIC_EnableInterrupt(NVIC_IT_LPUART1);
}
