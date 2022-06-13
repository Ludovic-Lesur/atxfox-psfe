/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "iwdg.h"
#include "psfe.h"

/*** PSFE main function ***/

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {
	// Init boiard.
	PSFE_init_hw();
	PSFE_init_context();
	PSFE_start();
	// Main loop.
	while (1) {
		PSFE_task();
		IWDG_reload();
	}
	return 0;
}
