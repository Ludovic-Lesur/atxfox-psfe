/*
 * tim.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "tim.h"

#include "rcc.h"
#include "rcc_reg.h"
#include "tim_reg.h"

/*** TIM functions ***/

/* CONFIGURE TIM21 TO OVERFLOW EVERY SECOND.
 * @param:	None.
 * @return:	None.
 */
void TIM21_Init(void) {

	/* Enable peripheral clock */
	RCC -> APB2ENR |= (0b1 << 2); // TIM21EN='1'.

	/* Reset timer before configuration */
	TIM21 -> CR1 &= ~(0b1 << 0); // Disable TIM21 (CEN='0').
	TIM21 -> CNT = 0; // Reset counter.

	/* Configure TIM21 as master to count milliseconds and overflow every seconds */
	TIM21 -> PSC = RCC_SYSCLK_KHZ; // Timer is clocked by SYSCLK (see RCC_Init() function). SYSCLK_KHZ-1 ?
	TIM21 -> ARR = 1000; // 999 ?
	TIM21 -> CR2 &= ~(0b111 << 4); // Reset bits 4-6.
	TIM21 -> CR2 |= (0b010 << 4); // Generate trigger on update event (MMS='010').

	/* Generate event to update registers */
	TIM21 -> EGR |= (0b1 << 0); // UG='1'.

	/* Enable peripheral */
	TIM21 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/* CONFIGURE TIM22 TO COUNT SECONDS SINCE START-UP.
 * @param:	None.
 * @return:	None.
 */
void TIM22_Init(void) {

	/* Enable peripheral clock */
	RCC -> APB2ENR |= (0b1 << 5); // TIM22EN='1'.

	/* Reset timer before configuration */
	TIM22 -> CR1 &= ~(0b1 << 0); // Disable TIM22 (CEN='0').
	TIM22 -> CNT = 0; // Reset counter.

	/* Configure TIM22 as slave to count seconds */
	TIM22 -> SMCR &= ~(0b111 << 4); // TS = '000' to select ITR0 = TIM1 as trigger input.
	TIM22 -> SMCR |= (0b111 << 0); // SMS = '111' to enable slave mode with external clock.

	/* Generate event to update registers */
	TIM22 -> EGR |= (0b1 << 0); // UG='1'.

	/* Enable peripheral */
	TIM22 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/* RETURNS THE NUMBER OF SECONDS ELLAPSED SINCE START-UP.
 * @param:	None.
 * @return:	Number of seconds ellapsed since start-up.
 */
unsigned int TIM22_GetSeconds(void) {
	return (TIM22 -> CNT);
}
