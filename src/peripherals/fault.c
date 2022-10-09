/*
 * fault.c
 *
 *  Created on: 10 apr. 2021
 *      Author: Ludo
 */

#include "pwr.h"

/* NON MASKABLE INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) NMI_Handler(void) {
	// Trigger software reset.
	PWR_software_reset();
}

/* HARD FAULT INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) HardFault_Handler(void) {
	// Trigger software reset.
	PWR_software_reset();
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
