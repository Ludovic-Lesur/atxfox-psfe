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

/*** PSFE board number (to be defined) ***/

#define PSFE_BOARD_NUMBER			6	// 1.0.x
#define DEBUG							// Do not use LPUART1 when debugging (same pins).

/*** ATXFOX global variables ***/

// PSFE-TRCS boards pairs.
static const unsigned char psfe_trcs_number[ATXFOX_NUMBER_OF_BOARDS] = {5, 6, 1, 2, 7, 8, 9, 3, 4, 10};
// Output voltage divider ratio.
static const unsigned char psfe_vout_voltage_divider[ATXFOX_NUMBER_OF_BOARDS] = {2, 2, 6, 6, 2, 2, 2, 6, 6, 2};

#endif /* ATXFOX_H_ */
