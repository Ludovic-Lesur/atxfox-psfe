/*
 * tim.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "tim.h"

#include "nvic.h"
#include "psfe.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "tim_reg.h"

/*** TIM local macros ***/

#define TIM_TIMEOUT_COUNT	1000000

/*** TIM local global variables ***/

static volatile unsigned char tim21_flag = 0;
static volatile unsigned char tim2_irq_count = 0;

/*** TIM local functions ***/

/* TIM2 INTERRUPT HANDLER.
 * @param:		None.
 * @return:		None.
 * @duration: 	~ 5ms.
 */
void __attribute__((optimize("-O0"))) TIM2_IRQHandler(void) {
	// Update interrupt.
	if (((TIM2 -> SR) & (0b1 << 0)) != 0) {
		// Perform ADC conversion and TRCS control.
		PSFE_adc_callback();
		// Check display period.
		tim2_irq_count++;
		if ((tim2_irq_count * PSFE_ADC_CONVERSION_PERIOD_MS) >= PSFE_LCD_UART_PRINT_PERIOD_MS) {
			// Update display and log.
			PSFE_lcd_uart_callback();
			tim2_irq_count = 0;
		}
		// Clear flag.
		TIM2 -> SR &= ~(0b1 << 0);
	}
}

/* TIM21 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) TIM21_IRQHandler(void) {
	// TI1 interrupt.
	if (((TIM21 -> SR) & (0b1 << 1)) != 0) {
		// Update flags.
		if (((TIM21 -> DIER) & (0b1 << 1)) != 0) {
			tim21_flag = 1;
		}
		TIM21 -> SR &= ~(0b1 << 1);
	}
}

/*** TIM functions ***/

/* CONFIGURE TIM21 FOR LSI FREQUENCY MEASUREMENT.
 * @param:	None.
 * @return:	None.
 */
void TIM21_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 2); // TIM21EN='1'.
	// Configure timer.
	// Channel input on TI1.
	// Capture done every 8 edges.
	// CH1 mapped on LSI.
	TIM21 -> CCMR1 |= (0b01 << 0) | (0b11 << 2);
	TIM21 -> OR |= (0b101 << 2);
	// Enable interrupt.
	TIM21 -> DIER |= (0b1 << 1); // CC1IE='1'.
	// Generate event to update registers.
	TIM21 -> EGR |= (0b1 << 0); // UG='1'.
}

/* MEASURE LSI CLOCK FREQUENCY WITH TIM21 CH1.
 * @param lsi_frequency_hz:		Pointer that will contain measured LSI frequency in Hz.
 * @return status:				Function execution status.
 */
TIM_status_t TIM21_get_lsi_frequency(unsigned int* lsi_frequency_hz) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	unsigned char tim21_interrupt_count = 0;
	unsigned int tim21_ccr1_edge1 = 0;
	unsigned int tim21_ccr1_edge8 = 0;
	unsigned int loop_count = 0;
	// Reset counter.
	TIM21 -> CNT = 0;
	TIM21 -> CCR1 = 0;
	// Enable interrupt.
	TIM21 -> SR &= 0xFFFFF9B8; // Clear all flags.
	NVIC_enable_interrupt(NVIC_IT_TIM21);
	// Enable TIM21 peripheral.
	TIM21 -> CR1 |= (0b1 << 0); // CEN='1'.
	TIM21 -> CCER |= (0b1 << 0); // CC1E='1'.
	// Wait for 2 captures.
	while (tim21_interrupt_count < 2) {
		// Wait for interrupt.
		tim21_flag = 0;
		loop_count = 0;
		while (tim21_flag == 0) {
			loop_count++;
			if (loop_count > TIM_TIMEOUT_COUNT) {
				status = TIM_ERROR_INTERRUPT_TIMEOUT;
				goto errors;
			}
		}
		tim21_interrupt_count++;
		if (tim21_interrupt_count == 1) {
			tim21_ccr1_edge1 = (TIM21 -> CCR1);
		}
		else {
			tim21_ccr1_edge8 = (TIM21 -> CCR1);
		}
	}
	// Compute LSI frequency.
	(*lsi_frequency_hz) = (8 * RCC_HSI_FREQUENCY_KHZ * 1000) / (tim21_ccr1_edge8 - tim21_ccr1_edge1);
errors:
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_IT_TIM21);
	// Stop counter.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	TIM21 -> CCER &= ~(0b1 << 0); // CC1E='0'.
	return status;
}

/* DISABLE TIM21 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void TIM21_disable(void) {
	// Disable TIM21 peripheral.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	RCC -> APB2ENR &= ~(0b1 << 2); // TIM21EN='0'.
}

/* CONFIGURE TIM2 FOR ADC CONVERSION PERIOD.
 * @param period_ms:	Timer period in ms.
 * @return:				None.
 */
void TIM2_init(unsigned int period_ms) {
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 0); // TIM2EN='1'.
	// Reset timer before configuration.
	TIM2 -> CR1 &= ~(0b1 << 0); // Disable TIM2 (CEN='0').
	TIM2 -> CNT = 0; // Reset counter.
	// Configure prescaler and period.
	TIM2 -> PSC = RCC_get_sysclk_khz(); // Timer is clocked by SYSCLK (see RCC_init() function).
	TIM2 -> ARR = period_ms;
	// Enable interrupt.
	TIM2 -> DIER |= (0b1 << 0); // UIE='1'.
	NVIC_set_priority(NVIC_IT_TIM2, 1);
	// Generate event to update registers.
	TIM2  -> EGR |= (0b1 << 0); // UG='1'.
}

/* START TIM2.
 * @param:	None.
 * @return:	None.
 */
void TIM2_start(void) {
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_IT_TIM2);
	// Start timer.
	TIM2 -> CR1 |= (0b1 << 0); // CEN='1'.
}
