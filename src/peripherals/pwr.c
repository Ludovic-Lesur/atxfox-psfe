/*
 * pwr.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "pwr.h"

#include "pwr_reg.h"
#include "rcc_reg.h"

/*** PWR functions ***/

/* INIT PWR INTERFACE.
 * @param:	None.
 * @return:	None.
 */
void PWR_Init(void) {
	// Enable power interface clock.
	RCC -> APB1ENR |= (0b1 << 28); // PWREN='1'.
	// Unlock back-up registers (DBP bit).
	PWR -> CR |= (0b1 << 8);
}
