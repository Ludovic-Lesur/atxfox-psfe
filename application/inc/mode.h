/*
 * mode.h
 *
 *  Created on: 04 dec. 2021
 *      Author: Ludo
 */

#ifndef __MODE_H__
#define __MODE_H__

/*** Board modes ***/

//#define DEBUG
/*** Board options ***/

#define PSFE_BOARD_NUMBER       1 // 1.0.x

//#define PSFE_SERIAL_MONITORING
//#define PSFE_SIGFOX_MONITORING

/*** Board flags definition ***/

#define PSFE_NUMBER_OF_BOARDS   10
#define PSFE_BOARD_INDEX        (PSFE_BOARD_NUMBER - 1)

/*** Errors management ***/

#if (PSFE_BOARD_NUMBER == 0) || (PSFE_BOARD_NUMBER > PSFE_NUMBER_OF_BOARDS)
#error "PSFE board number error."
#endif

#endif /* __MODE_H__ */
