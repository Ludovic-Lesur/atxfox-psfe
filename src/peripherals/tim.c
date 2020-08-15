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

/*** TIM local global variables ***/

static volatile unsigned char tim21_flag = 0;
static volatile unsigned char tim22_flag = 0;

/*** TIM local functions ***/

/* TIM21 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void TIM21_IRQHandler(void) {
	// Set flag.
	tim21_flag = 1;
	// Clear hardware flag.
	TIM21 -> SR &= ~(0b1 << 0);
}

/* TIM22 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void TIM22_IRQHandler(void) {
	// Set flag.
	tim22_flag = 1;
	// Clear hardware flag.
	TIM22 -> SR &= ~(0b1 << 0);
}

/*** TIM functions ***/

/* CONFIGURE TIM21 TO MANAGE LCD DISPLAY PERIOD.
 * @param:	None.
 * @return:	None.
 */
void TIM21_Init(unsigned int period_ms) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 2); // TIM21EN='1'.
	// Reset timer before configuration.
	TIM21 -> CR1 &= ~(0b1 << 0); // Disable TIM21 (CEN='0').
	TIM21 -> CNT = 0; // Reset counter.
	// Configure prescaler and period.
	TIM21 -> PSC = RCC_SYSCLK_KHZ; // Timer is clocked by SYSCLK (see RCC_Init() function).
	TIM21 -> ARR = period_ms;
	// Enable interrupt.
	TIM21 -> DIER |= (0b1 << 0); // UIE='1'.
	tim21_flag = 0;
	NVIC_SetPriority(IT_TIM21, 0);
	NVIC_EnableInterrupt(IT_TIM21);
	// Generate event to update registers.
	TIM21  -> EGR |= (0b1 << 0); // UG='1'.
	// Enable peripheral.
	TIM21 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/* READ TIM22 FLAG.
 * @param:	None.
 * @return:	Timer period overflow flag.
 */
unsigned char TIM21_GetFlag(void) {
	return tim21_flag;
}

/* CLEAR TIM22 FLAG.
 * @param:	None.
 * @return:	None.
 */
void TIM21_ClearFlag(void) {
	tim21_flag = 0;
}

/* CONFIGURE TIM22 TO OVERFLOW EVERY SECONDS.
 * @param:	None.
 * @return:	None.
 */
void TIM22_Init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 5); // TIM22EN='1'.
	// Reset timer before configuration.
	TIM22 -> CR1 &= ~(0b1 << 0); // Disable TIM22 (CEN='0').
	TIM22 -> CNT = 0; // Reset counter.
	// Configure prescaler and period.
	TIM22 -> PSC = RCC_SYSCLK_KHZ; // Timer is clocked by SYSCLK (see RCC_Init() function).
	TIM22 -> ARR = 1000;
	// Enable interrupt.
	TIM22 -> DIER |= (0b1 << 0); // UIE='1'.
	tim22_flag = 0;
	NVIC_SetPriority(IT_TIM22, 1);
	NVIC_EnableInterrupt(IT_TIM22);
	// Generate event to update registers.
	TIM22  -> EGR |= (0b1 << 0); // UG='1'.
	// Enable peripheral.
	TIM22 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/* READ TIM22 FLAG.
 * @param:	None.
 * @return:	Timer period overflow flag.
 */
unsigned char TIM22_GetFlag(void) {
	return tim22_flag;
}

/* CLEAR TIM22 FLAG.
 * @param:	None.
 * @return:	None.
 */
void TIM22_ClearFlag(void) {
	tim22_flag = 0;
}
