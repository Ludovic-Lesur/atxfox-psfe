/*
 * trcs.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludovic
 */

#ifndef TRCS_H
#define TRCS_H

/*** TRCS macros ***/

#define TRCS_IOUT_ERROR_VALUE	0xFFFFFFFF

/*** TRCS structures ***/

typedef enum {
	TRCS_RANGE_LOW = 0,
	TRCS_RANGE_MIDDLE,
	TRCS_RANGE_HIGH,
	TRCS_RANGE_NONE,
	TRCS_RANGE_LAST
} TRCS_range_t;

/*** TRCS functions ***/

void TRCS_init(void);
void TRCS_task(void);
void TRCS_set_bypass_flag(unsigned char bypass_state);
void TRCS_get_range(volatile TRCS_range_t* range);
void TRCS_get_iout(volatile unsigned int* iout_ua);
void TRCS_off(void);

#endif /* TRCS_H */
