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
	TRCS_RANGE_NONE = 0,
	TRCS_RANGE_LOW,
	TRCS_RANGE_MIDDLE,
	TRCS_RANGE_HIGH
} TRCS_Range;

/*** TRCS functions ***/

void TRCS_Init(void);
void TRCS_Task(unsigned int adc_bandgap_result_12bits, unsigned int* trcs_current_ua, unsigned char bypass_status, TRCS_Range* trcs_range);
void TRCS_Off(void);

#endif /* TRCS_H */
