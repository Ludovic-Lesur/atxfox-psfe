/*
 * usart.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "usart.h"

#include "gpio.h"
#include "mapping.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "td1208.h"
#include "usart_reg.h"

/*** USART local macros ***/

// Baud rate.
#define USART_BAUD_RATE 		9600
#define USART2_TIMEOUT_COUNT	100000

/*** USART local functions ***/

/* USART2 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) USART2_IRQHandler(void) {
	// RXNE interrupt.
	if (((USART2 -> ISR) & (0b1 << 5)) != 0) {
		// Store incoming byte in command buffer.
		TD1208_fill_rx_buffer(USART2 -> RDR);
		// Clear RXNE flag.
		USART2 -> RQR |= (0b1 << 3);
	}
	// Overrun error interrupt.
	if (((USART2 -> ISR) & (0b1 << 3)) != 0) {
		// Clear ORE flag.
		USART2 -> ICR |= (0b1 << 3);
	}
}

/* FILL USART TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return:			None.
 */
static void USART2_fill_tx_buffer(unsigned char tx_byte) {
#ifdef USE_TXE_INTERRUPT
	// Fill buffer.
	usart_ctx.tx_buf[usart_ctx.tx_buf_write_idx] = tx_byte;
	// Manage index roll-over.
	usart_ctx.tx_buf_write_idx++;
	if (usart_ctx.tx_buf_write_idx == USART_TX_BUFFER_SIZE) {
		usart_ctx.tx_buf_write_idx = 0;
	}
#else
	// Fill transmit register.
	USART2 -> TDR = tx_byte;
	// Wait for transmission to complete.
	unsigned int loop_count = 0;
	while (((USART2 -> ISR) & (0b1 << 7)) == 0) {
		// Wait for TXE='1' or timeout.
		loop_count++;
		if (loop_count > USART2_TIMEOUT_COUNT) break;
	}
#endif
}

/*** USART functions ***/

/* CONFIGURE USART2 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void USART2_init(void) {
	// Init context.
#ifdef USE_TXE_INTERRUPT
	unsigned int idx = 0;
	for (idx=0 ; idx<USART_TX_BUFFER_SIZE ; idx++) usart_ctx.tx_buf[idx] = 0;
	usart_ctx.tx_buf_write_idx = 0;
	usart_ctx.tx_buf_read_idx = 0;
#endif
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 17); // USART2EN='1'.
	// Configure TX and RX GPIOs.
	GPIO_configure(&GPIO_USART2_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_USART2_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_HIGH, GPIO_PULL_NONE);
	// Configure peripheral.
	USART2 -> CR3 |= (0b1 << 12); // No overrun detection (OVRDIS='1').
	USART2 -> BRR = ((RCC_get_sysclk_khz() * 1000) / (USART_BAUD_RATE)); // BRR = (fCK)/(baud rate). See p.730 of RM0377 datasheet.
	// Enable transmitter and receiver.
	USART2 -> CR1 |= (0b1 << 5) | (0b11 << 2); // TE='1', RE='1' and RXNEIE='1'.
	NVIC_set_priority(NVIC_IT_USART2, 0);
	// Enable peripheral.
	USART2 -> CR1 |= (0b1 << 0);
}

/* SEND A STRING THROUGH USART2.
 * @param tx_string:	String to send.
 * @return:				None.
 */
void USART2_send_string(char* tx_string) {
#ifdef USE_TXE_INTERRUPT
	// Disable interrupt.
	NVIC_disable_interrupt(IT_USART2);
#endif
	// Fill TX buffer with new bytes.
	while (*tx_string) {
		USART2_fill_tx_buffer((unsigned char) *(tx_string++));
	}
	// Enable interrupt.
#ifdef USE_TXE_INTERRUPT
	USART2 -> CR1 |= (0b1 << 7); // TXEIE='1'.
	NVIC_enable_interrupt(IT_USART2);
#endif
}
