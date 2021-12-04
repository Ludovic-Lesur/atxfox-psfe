/*
 * trcs.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludovic
 */

#ifndef TRCS_H
#define TRCS_H

/*** TRCS structures ***/

typedef enum {
	TRCS_RANGE_LOW = 0,
	TRCS_RANGE_MIDDLE,
	TRCS_RANGE_HIGH,
	TRCS_RANGE_NONE
} TRCS_Range;

/*** TRCS functions ***/

void TRCS_Init(void);
void TRCS_Task(void);
void TRCS_SetBypassFlag(unsigned char bypass_flag);
void TRCS_GetRange(volatile TRCS_Range* range);
void TRCS_GetIout(volatile unsigned int* iout_ua);
void TRCS_Off(void);

#endif /* TRCS_H */
