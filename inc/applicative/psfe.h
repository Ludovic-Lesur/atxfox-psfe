/*
 * atxfox.h
 *
 *  Created on: 25 jul. 2020
 *      Author: Ludo
 */

#ifndef __PSFE_H__
#define __PSFE_H__

#include "mode.h"
#include "types.h"

/*** PSFE macros ***/

#define PSFE_NUMBER_OF_BOARDS		10
#define PSFE_BOARD_INDEX			(PSFE_BOARD_NUMBER - 1)

/*** PSFE global variables ***/

static const uint8_t PSFE_TRCS_NUMBER[PSFE_NUMBER_OF_BOARDS] = {5, 6, 1, 2, 7, 8, 9, 3, 4, 10};
static const uint8_t PSFE_VOUT_VOLTAGE_DIVIDER_RATIO[PSFE_NUMBER_OF_BOARDS] = {2, 2, 6, 6, 2, 2, 2, 6, 6, 2};
static const uint32_t PSFE_VOUT_VOLTAGE_DIVIDER_RESISTANCE[PSFE_NUMBER_OF_BOARDS] = {998000, 998000, 599000, 599000, 998000, 998000, 998000, 599000, 599000, 998000};

/*** Errors management ***/

#if (PSFE_BOARD_NUMBER == 0) || (PSFE_BOARD_NUMBER > PSFE_NUMBER_OF_BOARDS)
#error "PSFE board number error."
#endif

#endif /* __PSFE_H__ */
