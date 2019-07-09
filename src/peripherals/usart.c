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
#include "tim.h"
#include "tst868u.h"
#include "usart_reg.h"

/*** USART local macros ***/

// If defined, use TXE interrupt for sending data.
#define USE_TXE_INTERRUPT
// Baud rate.
#define USART_BAUD_RATE 		9600
// TX buffer size.
#define USART_TX_BUFFER_SIZE	32

/*** USART local structures ***/

typedef struct {
#ifdef USE_TXE_INTERRUPT
	unsigned char tx_buf[USART_TX_BUFFER_SIZE]; 	// Transmit buffer.
	unsigned int tx_buf_read_idx; 					// Reading index in TX buffer.
	unsigned int tx_buf_write_idx; 					// Writing index in TX buffer.
#endif
} USART_Context;


/*** USART local global variables ***/

static volatile USART_Context usart_ctx;

/*** USART local functions ***/

/* USART2 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void USART2_IRQHandler(void) {

#ifdef USE_TXE_INTERRUPT
	/* TXE interrupt */
	if (((USART2 -> ISR) & (0b1 << 7)) != 0) {
		if ((usart_ctx.tx_buf_read_idx) != (usart_ctx.tx_buf_write_idx)) {
			// Fill transmit data register with new byte.
			USART2 -> TDR = usart_ctx.tx_buf[usart_ctx.tx_buf_read_idx];
			// Increment TX read index.
			usart_ctx.tx_buf_read_idx++;
			// Manage roll-over.
			if (usart_ctx.tx_buf_read_idx == USART_TX_BUFFER_SIZE) {
				usart_ctx.tx_buf_read_idx = 0;
			}
		}
		else {
			// No more bytes, disable TXE interrupt.
			USART2 -> CR1 &= ~(0b1 << 7); // TXEIE = '0'.
		}
	}
#endif

	/* RXNE interrupt */
	if (((USART2 -> ISR) & (0b1 << 5)) != 0) {
		// Store incoming byte in command buffer.
		unsigned char rx_byte = USART2 -> RDR;
#ifdef PSFE_SIGFOX_TST868U
		TST868U_FillRxBuffer(rx_byte);
#endif
#ifdef PSFE_SIGFOX_TD1208
		TD1208_FillRxBuffer(rx_byte);
#endif
	}

	/* Overrun error interrupt */
	if (((USART2 -> ISR) & (0b1 << 3)) != 0) {
		// Clear ORE flag.
		USART2 -> ICR |= (0b1 << 3);
	}
}

/* FILL USART2 TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return:			None.
 */
void USART2_FillTxBuffer(unsigned char tx_byte) {
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
	while (((USART2 -> ISR) & (0b1 << 7)) == 0); // Wait for TXE='1'.
#endif
}

/*** USART functions ***/

/* CONFIGURE USART2 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void USART2_Init(void) {

	/* Init context */
	unsigned int idx = 0;
#ifdef USE_TXE_INTERRUPT
	for (idx=0 ; idx<USART_TX_BUFFER_SIZE ; idx++) usart_ctx.tx_buf[idx] = 0;
	usart_ctx.tx_buf_write_idx = 0;
	usart_ctx.tx_buf_read_idx = 0;
#endif

	/* Enable peripheral clock */
	RCC -> APB1ENR |= (0b1 << 17); // USART2EN='1'.

	/* Configure TX and RX GPIOs */
	GPIO_Configure(&GPIO_USART2_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_UP);
	GPIO_Configure(&GPIO_USART2_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_UP);

	/* Configure peripheral */
	USART2 -> CR1 = 0; // Disable peripheral before configuration (UE='0'), 1 stop bit and 8 data bits (M='00').
	USART2 -> CR2 = 0; // 1 stop bit (STOP='00').
	USART2 -> CR3 = 0;
	USART2 -> CR3 |= (0b1 << 12); // No overrun detection (OVRDIS='1').
	USART2 -> BRR = ((RCC_SYSCLK_KHZ * 1000) / (USART_BAUD_RATE)); // BRR = (fCK)/(baud rate). See p.730 of RM0377 datasheet.

	/* Enable transmitter and receiver */
	USART2 -> CR1 |= (0b11 << 2); // TE='1' and RE='1'.
	USART2 -> CR1 |= (0b1 << 5); // RXNEIE='1'.

	/* Enable peripheral */
	USART2 -> CR1 |= (0b1 << 0);
	NVIC_EnableInterrupt(IT_USART2);
}

/* SEND A BYTE ARRAY THROUGH USART2.
 * @param tx_data:			Byte array to send.
 * @param tx_data_length:	Number of bytes to send.
 * @return:					None.
 */
void USART2_Send(unsigned char* tx_data, unsigned char tx_data_length) {
	// Disable interrupt.
	NVIC_DisableInterrupt(IT_USART2);
	// Fill TX buffer with new bytes.
	unsigned char byte_idx = 0;
	for (byte_idx=0 ; byte_idx<tx_data_length ; byte_idx++) {
		USART2_FillTxBuffer(tx_data[byte_idx]);
	}
	// Enable interrupt.
#ifdef USE_TXE_INTERRUPT
	USART2 -> CR1 |= (0b1 << 7); // TXEIE='1'.
#endif
	NVIC_EnableInterrupt(IT_USART2);
}
