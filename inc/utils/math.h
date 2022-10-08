/*
 * math.h
 *
 *  Created on: 28 aug. 2020
 *      Author: Ludo
 */

#ifndef __MATH_H__
#define __MATH_H__

#include "mode.h"
#include "types.h"

/*** MATH macros ***/

#define MATH_BINARY_MAX_LENGTH			32
#define MATH_DECIMAL_MAX_LENGTH			10
#define MATH_HEXADECIMAL_MAX_LENGTH		4
#define MATH_BYTE_MAX					0xFF

/*** MATH structures ***/

typedef enum {
	MATH_SUCCESS = 0,
	MATH_ERROR_NULL_PARAMETER,
	MATH_ERROR_OVERFLOW,
	MATH_ERROR_UNDEFINED,
	MATH_ERROR_SIGN_BIT,
	MATH_ERROR_BASE_LAST = 0x0100
} MATH_status_t;

/*** MATH functions ***/

MATH_status_t MATH_min_u8(uint8_t* data, uint8_t data_length, uint8_t* result);
MATH_status_t MATH_min_u16(uint16_t* data, uint8_t data_length, uint16_t* result);
MATH_status_t MATH_min_u32(uint32_t* data, uint8_t data_length, uint32_t* result);

MATH_status_t MATH_max_u8(uint8_t* data, uint8_t data_length, uint8_t* result);
MATH_status_t MATH_max_u16(uint16_t* data, uint8_t data_length, uint16_t* result);
MATH_status_t MATH_max_u32(uint32_t* data, uint8_t data_length, uint32_t* result);

MATH_status_t MATH_average_u8(uint8_t* data, uint8_t data_length, uint8_t* result);
MATH_status_t MATH_average_u16(uint16_t* data, uint8_t data_length, uint16_t* result);
MATH_status_t MATH_average_u32(uint32_t* data, uint8_t data_length, uint32_t* result);

MATH_status_t MATH_median_filter_u8(uint8_t* data, uint8_t median_length, uint8_t average_length, uint8_t* result);
MATH_status_t MATH_median_filter_u16(uint16_t* data, uint8_t median_length, uint8_t average_length, uint16_t* result);
MATH_status_t MATH_median_filter_u32(uint32_t* data, uint8_t median_length, uint8_t average_length, uint32_t* result);

MATH_status_t MATH_pow_10(uint8_t power, uint32_t* result);
MATH_status_t MATH_abs(int32_t x, uint32_t* result);
MATH_status_t MATH_atan2(int32_t x, int32_t y, uint32_t* alpha);

MATH_status_t MATH_two_complement(uint32_t value, uint8_t sign_bit_position, int32_t* result);
MATH_status_t MATH_one_complement(int32_t value, uint8_t sign_bit_position, uint32_t* result);

#define MATH_status_check(error_base) { if (math_status != MATH_SUCCESS) { status = error_base + math_status; goto errors; }}
#define MATH_error_check() { ERROR_status_check(math_status, MATH_SUCCESS, ERROR_BASE_MATH); }
#define MATH_error_check_print() { ERROR_status_check_print(math_status, MATH_SUCCESS, ERROR_BASE_MATH); }

#endif /* __MATH_H__ */
