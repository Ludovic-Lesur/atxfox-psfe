/*
 * fault.c
 *
 *  Created on: 10 apr. 2021
 *      Author: Ludo
 */

#include "scb_reg.h"

/* NON MASKABLE INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void NMI_Handler(void) {
	// Trigger software reset.
	SCB -> AIRCR = 0x05FA0000 | ((SCB -> AIRCR) & 0x0000FFFF) | (0b1 << 2);
}

/* HARD FAULT INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void HardFault_Handler(void) {
	// Trigger software reset.
	SCB -> AIRCR = 0x05FA0000 | ((SCB -> AIRCR) & 0x0000FFFF) | (0b1 << 2);
}

/* SVC INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void SVC_Handler(void) {
	// TBD.
}

/* PENDING SV INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void PendSV_Handler(void) {
	// TBD.
}

/* SYSTEM TICK INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void SysTick_Handler(void) {
	// TBD.
}
