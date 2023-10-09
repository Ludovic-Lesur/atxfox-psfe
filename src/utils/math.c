/*
 * math.c
 *
 *  Created on: 28 aug. 2020
 *      Author: Ludo
 */

#include "math.h"

#include "types.h"

/*** MATH local macros ***/

#define MATH_MEDIAN_FILTER_LENGTH_MAX	0xFF

static const uint32_t MATH_POW10[MATH_DECIMAL_DIGIT_MAX_NUMBER] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

/*** MATH local functions ***/

/*******************************************************************/
#define _MATH_min(data, data_size) { \
	uint8_t idx = 0; \
	for (idx=0 ; idx<data_size ; idx++) { \
		if (data[idx] < min) { \
			min = data[idx]; \
		} \
	} \
}

/*******************************************************************/
#define _MATH_max(data, data_size) { \
	uint8_t idx = 0; \
	for (idx=0 ; idx<data_size ; idx++) { \
		if (data[idx] > max) { \
			max = data[idx]; \
		} \
	} \
}

/*******************************************************************/
#define _MATH_average(data, data_size) { \
	uint8_t idx = 0; \
	for (idx=0 ; idx<data_size ; idx++) { \
		average = ((average * idx) + data[idx]) / (idx + 1); \
	} \
}

/*******************************************************************/
#define _MATH_median_filter(data, median_size, average_size) { \
	uint8_t buffer_sorted = 0; \
	uint8_t idx1 = 0; \
	uint8_t idx2 = 0; \
	/* Copy input buffer into local buffer */ \
	for (idx1=0 ; idx1<median_size ; idx1++) { \
		local_buf[idx1] = data[idx1]; \
	} \
	/* Sort buffer in ascending order. */ \
	for (idx1=0; idx1<median_size; ++idx1) { \
		buffer_sorted = 1; \
		for (idx2=1 ; idx2<(median_size-idx1) ; ++idx2) { \
			if (local_buf[idx2 - 1] > local_buf[idx2]) { \
				temp = local_buf[idx2 - 1]; \
				local_buf[idx2 - 1] = local_buf[idx2]; \
				local_buf[idx2] = temp; \
				buffer_sorted = 0; \
			} \
		} \
		if (buffer_sorted != 0) break; \
	} \
	/* Compute start and end indexes for final averaging */ \
	if (average_size > 0) { \
		/* Clamp value */ \
		if (average_size > median_size) { \
			average_size = median_size; \
		} \
		start_idx = (median_size / 2) - (average_size / 2); \
		end_idx = (median_size / 2) + (average_size / 2); \
		if (end_idx >= median_size) { \
			end_idx = (median_size - 1); \
		} \
	} \
}

/*******************************************************************/
#define _MATH_check_pointer(ptr) { \
	if (ptr == NULL) { \
		status = MATH_ERROR_NULL_PARAMETER; \
		goto errors; \
	} \
}

/*** MATH functions ***/

/*******************************************************************/
MATH_status_t MATH_min_u8(uint8_t* data, uint8_t data_size, uint8_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint8_t min = 0xFF;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute minimum value.
	_MATH_min(data, data_size);
	(*result) = min;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_min_u16(uint16_t* data, uint8_t data_size, uint16_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint16_t min = 0xFFFF;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Local variables.
	// Compute minimum value.
	_MATH_min(data, data_size);
	(*result) = min;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_min_u32(uint32_t* data, uint8_t data_size, uint32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint32_t min = 0xFFFFFFFF;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute minimum value.
	_MATH_min(data, data_size);
	(*result) = min;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_max_u8(uint8_t* data, uint8_t data_size, uint8_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint8_t max = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute minimum value.
	_MATH_max(data, data_size);
	(*result) = max;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_max_u16(uint16_t* data, uint8_t data_size, uint16_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint16_t max = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute minimum value.
	_MATH_max(data, data_size);
	(*result) = max;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_max_u32(uint32_t* data, uint8_t data_size, uint32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint32_t max = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute minimum value.
	_MATH_max(data, data_size);
	(*result) = max;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_average_u8(uint8_t* data, uint8_t data_size, uint8_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint8_t average = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute average.
	_MATH_average(data, data_size);
	(*result) = average;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_average_u16(uint16_t* data, uint8_t data_size, uint16_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint16_t average = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute average.
	_MATH_average(data, data_size);
	(*result) = average;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_average_u32(uint32_t* data, uint8_t data_size, uint32_t* result) {
// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint32_t average = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute average.
	_MATH_average(data, data_size);
	(*result) = average;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_median_filter_u8(uint8_t* data, uint8_t median_size, uint8_t average_size, uint8_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint8_t local_buf[MATH_MEDIAN_FILTER_LENGTH_MAX];
	uint8_t temp = 0;
	uint8_t start_idx = 0;
	uint8_t end_idx = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute median filter.
	_MATH_median_filter(data, median_size, average_size);
	// Compute average or median value.
	if (average_size > 0) {
		status = MATH_average_u8(&(data[start_idx]), (end_idx - start_idx + 1), result);
	}
	else {
		(*result) = local_buf[(median_size / 2)];
	}
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_median_filter_u16(uint16_t* data, uint8_t median_size, uint8_t average_size, uint16_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint16_t local_buf[MATH_MEDIAN_FILTER_LENGTH_MAX];
	uint16_t temp = 0;
	uint8_t start_idx = 0;
	uint8_t end_idx = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute median filter.
	_MATH_median_filter(data, median_size, average_size);
	// Compute average or median value.
	if (average_size > 0) {
		status = MATH_average_u16(&(data[start_idx]), (end_idx - start_idx + 1), result);
	}
	else {
		(*result) = local_buf[(median_size / 2)];
	}
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_median_filter_u32(uint32_t* data, uint8_t median_size, uint8_t average_size, uint32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint32_t local_buf[MATH_MEDIAN_FILTER_LENGTH_MAX];
	uint32_t temp = 0;
	uint8_t start_idx = 0;
	uint8_t end_idx = 0;
	// Check parameters.
	_MATH_check_pointer(data);
	_MATH_check_pointer(result);
	// Compute median filter.
	_MATH_median_filter(data, median_size, average_size);
	// Compute average or median value.
	if (average_size > 0) {
		status = MATH_average_u32(&(data[start_idx]), (end_idx - start_idx + 1), result);
	}
	else {
		(*result) = local_buf[(median_size / 2)];
	}
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_pow_10(uint8_t power, uint32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	// Check parameters.
	_MATH_check_pointer(result);
	if (power >= MATH_DECIMAL_DIGIT_MAX_NUMBER) {
		status = MATH_ERROR_OVERFLOW;
		goto errors;
	}
	(*result) = MATH_POW10[power];
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_abs(int32_t x, uint32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	// Check parameters.
	_MATH_check_pointer(result);
	// Check sign.
	if (x >= 0) (*result) = x;
	if (x < 0) (*result) = (-1) * x;
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_atan2(int32_t x, int32_t y, uint32_t* alpha) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	int32_t local_x = x;
	int32_t local_y = y;
	uint32_t abs_x = 0;
	uint32_t abs_y = 0;
	// Check parameters.
	if ((x == 0) && (y == 0)) {
		status = MATH_ERROR_UNDEFINED;
		goto errors;
	}
	_MATH_check_pointer(alpha);
	// Scale x and y to avoid overflow.
	do {
		status = MATH_abs(local_x, &abs_x);
		if (status != MATH_SUCCESS) goto errors;
		status = MATH_abs(local_y, &abs_y);
		if (status != MATH_SUCCESS) goto errors;
	}
	while ((abs_x > 10000) || (abs_y > 10000));
	// Use the quotient within [-1,1]
	if (abs_x >= abs_y) {
		// Use arctan approximation: arctan(z)=(pi/4)*z.
		(*alpha) = (((45 * abs_y) << 10) / (abs_x)) >> 10; // Quadrant 1.
		// Add offset depending on quadrant.
		if ((x > 0) && (y < 0)) {
			(*alpha) = (360 - (*alpha)); // Quadrant 8.
		}
		if (x < 0) {
			if (y > 0) {
				(*alpha) = (180 - (*alpha)); // Quadrant 4.
			}
			else {
				(*alpha) = (180 + (*alpha)); // Quadrant 5.
			}
		}
	}
	else {
		// Use arctan approximation: arctan(z)=(pi/4)*z.
		(*alpha) = (((45 * abs_x) << 10) / (abs_y)) >> 10;
		// Add offset depending on quadrant and arctan(1/z)=+/-90-arctan(z).
		if (x > 0) {
			if (y > 0) {
				(*alpha) = (90 - (*alpha)); // Quadrant 2.
			}
			else {
				(*alpha) = (270 + (*alpha)); // Quadrant 7.
			}
		}
		else {
			if (y > 0) {
				(*alpha) = (90 + (*alpha)); // Quadrant 3.
			}
			else {
				(*alpha) = (270 - (*alpha)); // Quadrant 6.
			}
		}
	}
	// Ensure angle is in [0,359] range.
	(*alpha) = ((*alpha) % 360);
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_two_complement_to_int32(uint32_t value, uint8_t sign_bit_position, int32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint8_t bit_idx = 0;
	uint32_t not_value = 0;
	uint32_t absolute_value = 0;
	// Check parameters.
	if (sign_bit_position > (MATH_BINARY_DIGIT_MAX_NUMBER - 1)) {
		status = MATH_ERROR_SIGN_BIT;
		goto errors;
	}
	_MATH_check_pointer(result);
	// Check sign bit.
	if ((value & (0b1 << sign_bit_position)) == 0) {
		// Value is positive: nothing to do.
		(*result) = (int32_t) value;
	}
	else {
		// Value is negative.
		for (bit_idx=0 ; bit_idx<=sign_bit_position ; bit_idx++) {
			if ((value & (0b1 << bit_idx)) == 0) {
				not_value |= (0b1 << bit_idx);
			}
		}
		absolute_value = not_value + 1;
		(*result) = (-1) * absolute_value;
	}
errors:
	return status;
}

/*******************************************************************/
MATH_status_t MATH_int32_to_signed_magnitude(int32_t value, uint8_t sign_bit_position, uint32_t* result) {
	// Local variables.
	MATH_status_t status = MATH_SUCCESS;
	uint32_t absolute_value = 0;
	uint32_t absolute_mask = ((0b1 << sign_bit_position) - 1);
	// Check parameters.
	if (sign_bit_position > (MATH_BINARY_DIGIT_MAX_NUMBER - 1)) {
		status = MATH_ERROR_SIGN_BIT;
		goto errors;
	}
	_MATH_check_pointer(result);
	// Compute absolute value.
	status = MATH_abs(value, &absolute_value);
	if (status != MATH_SUCCESS) goto errors;
	// Check size.
	if (absolute_value > absolute_mask) {
		status = MATH_ERROR_MAGNITUDE_OVERFLOW;
		goto errors;
	}
	(*result) = (absolute_value & absolute_mask);
	if (value < 0) {
		(*result) |= (0b1 << sign_bit_position);
	}
errors:
	return status;
}
