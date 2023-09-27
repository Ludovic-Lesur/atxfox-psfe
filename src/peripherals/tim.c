/*
 * tim.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "tim.h"

#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "tim_reg.h"
#include "types.h"

/*** TIM local macros ***/

#define TIM_TIMEOUT_COUNT	1000000

/*** TIM local global variables ***/

static volatile TIM_completion_irq_cb_t tim2_update_irq_callback = NULL;
static volatile uint8_t tim21_flag = 0;

/*** TIM local functions ***/

/*******************************************************************/
void __attribute__((optimize("-O0"))) TIM2_IRQHandler(void) {
	// Update interrupt.
	if (((TIM2 -> SR) & (0b1 << 0)) != 0) {
		// Call callback.
		if (tim2_update_irq_callback != NULL) {
			tim2_update_irq_callback();
		}
		// Clear flag.
		TIM2 -> SR &= ~(0b1 << 0);
	}
}

/*******************************************************************/
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

/*******************************************************************/
void TIM2_init(uint32_t period_ms, TIM_completion_irq_cb_t irq_callback) {
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 0); // TIM2EN='1'.
	// Ensure timer if off.
	TIM2 -> CR1 &= ~(0b1 << 0); // Disable TIM2 (CEN='0').
	// Configure prescaler and period.
	TIM2 -> PSC = RCC_SYSCLK_FREQUENCY_KHZ; // Timer is clocked by SYSCLK (see RCC_init() function).
	TIM2 -> ARR = period_ms;
	// Enable interrupt.
	TIM2 -> DIER |= (0b1 << 0); // UIE='1'.
	// Generate event to update registers.
	TIM2  -> EGR |= (0b1 << 0); // UG='1'.
	// Register callback.
	tim2_update_irq_callback = irq_callback;
}

/*******************************************************************/
void TIM2_de_init(void) {
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM2);
	// Disable peripheral clock.
	RCC -> APB1ENR &= ~(0b1 << 0); // TIM2EN='0'.
}

/*******************************************************************/
void TIM2_start(void) {
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM2, NVIC_PRIORITY_TIM2);
	// Start timer.
	TIM2 -> CNT = 0; // Reset counter.
	TIM2 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/*******************************************************************/
void TIM2_stop(void) {
	// Stop timer.
	TIM2 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM2);
}

/*******************************************************************/
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

/*******************************************************************/
void TIM21_de_init(void) {
	// Disable timer.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	// Disable peripheral clock.
	RCC -> APB2ENR &= ~(0b1 << 2); // TIM21EN='0'.
}

/*******************************************************************/
TIM_status_t TIM21_measure_lsi_frequency(uint32_t* lsi_frequency_hz) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	uint8_t tim21_interrupt_count = 0;
	uint32_t tim21_ccr1_edge1 = 0;
	uint32_t tim21_ccr1_edge8 = 0;
	uint32_t loop_count = 0;
	// Check parameters.
	if (lsi_frequency_hz == NULL) {
		status = TIM_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset counter.
	TIM21 -> CNT = 0;
	TIM21 -> CCR1 = 0;
	// Enable interrupt.
	TIM21 -> SR &= 0xFFFFF9B8; // Clear all flags.
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM21, NVIC_PRIORITY_TIM21);
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
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM21);
	// Stop counter.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	TIM21 -> CCER &= ~(0b1 << 0); // CC1E='0'.
	return status;
}
