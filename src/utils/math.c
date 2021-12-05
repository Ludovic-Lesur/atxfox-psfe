/*
 * filter.c
 *
 *  Created on: 14 aug. 2020
 *      Author: Ludo
 */

#include "math.h"

/*** MATH local macros ***/

#define MATH_MEDIAN_FILTERLENGTH_MAX	0xFF

/*** MATH functions ***/

/* COMPUTE AVERAGE VALUE.
 * @param data:			Input buffer.
 * @param data_length:	Input buffer length.
 * @return average: 	Average value of the input buffer.
 */
unsigned int MATH_ComputeAverage(unsigned int* data, unsigned char data_length) {
	// Local variables.
	unsigned char idx = 0;
	unsigned int average = 0;
	// Compute
	for (idx=0 ; idx<data_length ; idx++) {
		average = ((average * idx) + data[idx]) / (idx + 1);
	}
	return average;
}

/* COMPUTE AVERAGE MEDIAN VALUE
 * @param data:				Input buffer.
 * @param median_length:	Number of elements taken for median value search.
 * @param average_length:	Number of center elements taken for final average.
 * @return filter_out:		Output value of the median filter.
 */
unsigned int MATH_ComputeMedianFilter(unsigned int* data, unsigned char median_length, unsigned char average_length) {
	// Local variables.
	unsigned int local_buf[MATH_MEDIAN_FILTERLENGTH_MAX];
	unsigned char buffer_sorted = 0;
	unsigned char idx1 = 0;
	unsigned char idx2 = 0;
	unsigned char start_idx = 0;
	unsigned char end_idx = 0;
	unsigned int filter_out = 0;
	unsigned int temp = 0;
	// Copy input buffer into local buffer.
	for (idx1=0 ; idx1<median_length ; idx1++) {
		local_buf[idx1] = data[idx1];
	}
	// Sort buffer in ascending order.
	for (idx1=0; idx1<median_length; ++idx1) {
		buffer_sorted = 1;
		for (idx2=1 ; idx2<(median_length-idx1) ; ++idx2) {
			if (local_buf[idx2 - 1] > local_buf[idx2]) {
				temp = local_buf[idx2 - 1];
				local_buf[idx2 - 1] = local_buf[idx2];
				local_buf[idx2] = temp;
				buffer_sorted = 0;
			}
		}
		if (buffer_sorted != 0) break;
	}
	// Compute average of center values if required.
	if (average_length > 0) {
		// Clamp value.
		if (average_length > median_length) {
			average_length = median_length;
		}
		start_idx = (median_length / 2) - (average_length / 2);
		end_idx = (median_length / 2) + (average_length / 2);
		if (end_idx >= median_length) {
			end_idx = (median_length - 1);
		}
		// Compute average.
		filter_out = MATH_ComputeAverage(&(data[start_idx]), (end_idx - start_idx + 1));
	}
	else {
		// Return median value.
		filter_out = local_buf[(median_length / 2)];
	}
	return filter_out;
}
