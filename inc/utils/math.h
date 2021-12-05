/*
 * math.h
 *
 *  Created on: 14 aug. 2020
 *      Author: Ludo
 */

#ifndef MATH_H
#define MATH_H

/*** MATH functions ***/

unsigned int MATH_ComputeAverage(unsigned int* data, unsigned char data_length);
unsigned int MATH_ComputeMedianFilter(unsigned int* buf, unsigned char median_length, unsigned char average_length);

#endif /* MATH_H */
