/*
 * range.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludovic
 */

#ifndef RANGE_H
#define RANGE_H

/*** RANGE structures ***/

typedef enum {
	RANGE_NONE,
	RANGE_LOW,
	RANGE_MIDDLE,
	RANGE_HIGH
} RANGE_Level;

/*** RANGE functions ***/

void RANGE_Init(void);
void RANGE_Set(RANGE_Level range_level);

#endif /* RANGE_H */
