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
void __attribute__((optimize("-O0"))) NMI_Handler(void) {
	// Trigger software reset.
	SCB -> AIRCR = 0x05FA0000 | ((SCB -> AIRCR) & 0x0000FFFF) | (0b1 << 2);
}

/* HARD FAULT INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) HardFault_Handler(void) {
	// Trigger software reset.
	SCB -> AIRCR = 0x05FA0000 | ((SCB -> AIRCR) & 0x0000FFFF) | (0b1 << 2);
}

/* SVC INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) SVC_Handler(void) {
	// TBD.
}

/* PENDING SV INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) PendSV_Handler(void) {
	// TBD.
}

/* SYSTEM TICK INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) SysTick_Handler(void) {
	// TBD.
}
