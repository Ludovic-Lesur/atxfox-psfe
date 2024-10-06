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

/*** Errors management ***/

#if (PSFE_BOARD_NUMBER == 0) || (PSFE_BOARD_NUMBER > PSFE_NUMBER_OF_BOARDS)
#error "PSFE board number error."
#endif

#endif /* __PSFE_H__ */
