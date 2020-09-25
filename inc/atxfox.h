/*
 * atxfox.h
 *
 *  Created on: 25 july 2020
 *      Author: Ludo
 */

#ifndef ATXFOX_H
#define ATXFOX_H

/*** ATXFOX macros ***/

#define ATXFOX_NUMBER_OF_BOARDS		10
#define PSFE_BOARD_NUMBER			1	// 1.0.x
#define PSFE_BOARD_INDEX			(PSFE_BOARD_NUMBER - 1)
//#define DEBUG						// Do not use LPUART1 when debugging (same pins).

/*** ATXFOX global variables ***/

// PSFE-TRCS boards pairs.
static const unsigned char psfe_trcs_number[ATXFOX_NUMBER_OF_BOARDS] = {5, 6, 1, 2, 7, 8, 9, 3, 4, 10};
// Output voltage divider ratio.
static const unsigned char psfe_vout_voltage_divider_ratio[ATXFOX_NUMBER_OF_BOARDS] = {2, 2, 6, 6, 2, 2, 2, 6, 6, 2};
// Output voltage divider total resistance (used to remove the offset current caused by the divider).
static const unsigned int psfe_vout_voltage_divider_resistance[ATXFOX_NUMBER_OF_BOARDS] = {998000, 998000, 599000, 599000, 998000, 998000, 998000, 599000, 599000, 998000};

#if (PSFE_BOARD_NUMBER == 0) || (PSFE_BOARD_NUMBER > ATXFOX_NUMBER_OF_BOARDS)
#error "PSFE board number error."
#endif

#endif /* ATXFOX_H */
