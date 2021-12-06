/*
 * atxfox.h
 *
 *  Created on: 25 july 2020
 *      Author: Ludo
 */

#ifndef ATXFOX_H
#define ATXFOX_H

#include "mode.h"

/*** PSFE macros ***/

#define PSFE_NUMBER_OF_BOARDS			10
#define PSFE_BOARD_INDEX				(PSFE_BOARD_NUMBER - 1)

#define PSFE_ADC_CONVERSION_PERIOD_MS	100
#define PSFE_LCD_UART_PRINT_PERIOD_MS	300

/*** PSFE global variables ***/

static const unsigned char psfe_trcs_number[PSFE_NUMBER_OF_BOARDS] = {5, 6, 1, 2, 7, 8, 9, 3, 4, 10};
static const unsigned char psfe_vout_voltage_divider_ratio[PSFE_NUMBER_OF_BOARDS] = {2, 2, 6, 6, 2, 2, 2, 6, 6, 2};

/*** PSFE functions ***/

void PSFE_Init(void);
void PSFE_Task(void);
void PSFE_SetBypassFlag(unsigned char bypass_state);
void PSFE_AdcCallback(void);
void PSFE_LcdUartCallback(void);

#endif /* ATXFOX_H */
