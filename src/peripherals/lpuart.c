/*
 * lpuart.c
 *
 *  Created on: 09 jul. 2019
 *      Author: Ludo
 */

#include "lpuart.h"

#include "exti.h"
#include "gpio.h"
#include "lpuart_reg.h"
#include "mode.h"
#include "mapping.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "string.h"
#include "types.h"

/*** LPUART local macros ***/

#define LPUART_BAUD_RATE 		9600
#define LPUART_TIMEOUT_COUNT	100000

/*** LPUART local global variables ***/

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
static LPUART_rx_irq_cb_t lpuart1_rx_irq_callback = NULL;
#endif

/*** LPUART local functions ***/

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
void LPUART1_IRQHandler(void) {
	// Local variables.
	uint8_t rx_byte = 0;
	// RXNE interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 5)) != 0) {
		// Read incoming byte.
		rx_byte = (LPUART1 -> RDR);
		// Transmit byte to upper layer.
		if (lpuart1_rx_irq_callback != NULL) {
			lpuart1_rx_irq_callback(rx_byte);
		}
		// Clear RXNE flag.
		LPUART1 -> RQR |= (0b1 << 3);
	}
	// Overrun error interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 3)) != 0) {
		// Clear ORE flag.
		LPUART1 -> ICR |= (0b1 << 3);
	}
	EXTI_clear_flag(EXTI_LINE_LPUART1);
}
#endif

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
static LPUART_status_t _LPUART1_fill_tx_buffer(uint8_t tx_byte) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	uint32_t loop_count = 0;
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
#endif

/*** LPUART functions ***/

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
void LPUART1_init(LPUART_rx_irq_cb_t irq_callback) {
	// Local variables.
	uint64_t brr = 0;
	// Select HSI as clock source.
	RCC -> CCIPR |= (0b10 << 10); // LPUART1SEL='10'.
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 18); // LPUARTEN='1'.
	// Configure peripheral.
	LPUART1 -> CR3 |= (0b1 << 12); // No overrun detection (OVRDIS='0').
	// Baud rate.
	brr = ((uint64_t) RCC_HSI_FREQUENCY_KHZ * (uint64_t) 1000 * (uint64_t) 256);
	brr /= (uint64_t) LPUART_BAUD_RATE;
	LPUART1 -> BRR = (((uint32_t) brr) & 0x000FFFFF); // BRR = (256*fCK)/(baud rate). See p.730 of RM0377 datasheet.
	// Configure interrupt.
	EXTI_configure_line(EXTI_LINE_LPUART1, EXTI_TRIGGER_RISING_EDGE);
	// Enable transmitter.
	LPUART1 -> CR1 |= (0b1 << 3); // TE='1'.
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
	// Configure GPIOs.
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Register callback.
	lpuart1_rx_irq_callback = irq_callback;
}
#endif

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
void LPUART1_enable_rx(void) {
	// Mute mode request.
	LPUART1 -> RQR |= (0b1 << 2); // MMRQ='1'.
	// Clear flag and enable interrupt.
	LPUART1 -> RQR |= (0b1 << 3);
	NVIC_enable_interrupt(NVIC_INTERRUPT_LPUART1, NVIC_PRIORITY_LPUART1);
	// Enable receiver.
	LPUART1 -> CR1 |= (0b1 << 2); // RE='1'.
#ifdef LPUART_USE_NRE
	GPIO_write(&GPIO_LPUART1_NRE, 0);
#endif
}
#endif

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
void LPUART1_disable_rx(void) {
	// Disable receiver.
#ifdef LPUART_USE_NRE
	GPIO_write(&GPIO_LPUART1_NRE, 1);
#endif
	LPUART1 -> CR1 &= ~(0b1 << 2); // RE='0'.
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_LPUART1);
}
#endif

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
LPUART_status_t LPUART1_write(uint8_t* data, uint32_t data_size_bytes) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	uint32_t idx = 0;
	uint32_t loop_count = 0;
	// Check parameter.
	if (data == NULL) {
		status = LPUART_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Fill TX buffer with new bytes.
	for (idx=0 ; idx<data_size_bytes ; idx++) {
		status = _LPUART1_fill_tx_buffer(data[idx]);
		if (status != LPUART_SUCCESS) goto errors;
	}
	// Wait for TC flag.
	while (((LPUART1 -> ISR) & (0b1 << 6)) == 0) {
		// Exit if timeout.
		loop_count++;
		if (loop_count > LPUART_TIMEOUT_COUNT) {
			status = LPUART_ERROR_TC_TIMEOUT;
			goto errors;
		}
	}
errors:
	return status;
}
#endif

#if (defined USE_SERIAL_MONITORING) && !(defined DEBUG)
/*******************************************************************/
LPUART_status_t LPUART1_write_string(char_t* str) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	uint8_t str_size = 0;
	// Get string size.
	string_status = STRING_get_size(str, &str_size);
	STRING_exit_error(LPUART_ERROR_BASE_STRING);
	// Send string.
	status = LPUART1_write((uint8_t*) str, (uint32_t) str_size);
errors:
	return status;
}
#endif
