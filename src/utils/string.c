/*
 * string.c
 *
 *  Created on: Dec 5, 2021
 *      Author: Ludo
 */

#include "string.h"

#include "math.h"

/*** STRING local macros ***/



/*** STRING functions ***/

/* CONVERTS THE ASCII CODE OF AN HEXADECIMAL CHARACTER TO THE CORRESPONDING A 4-BIT WORD.
 * @param ascii_code:	Hexadecimal ASCII code to convert.
 * @return value:		Corresponding value.
 */
unsigned char STRING_ascii_to_hexa(char ascii_code) {
	unsigned char value = 0;
	if ((ascii_code >= 'A') && (ascii_code <= 'F')) {
		value = ascii_code - 'A' + 10;
	}
	if ((ascii_code >= '0') && (ascii_code <= '9')) {
		value = ascii_code - '0';
	}
	return value;
}

/* RETURN CORRESPONDING ASCII CHARACTER OF A GIVEN DECIMAL VALUE.
 * @param value:		Decimal digit.
 * @return ascii_code:	Corresponding ASCII code.
 */
char STRING_decimal_to_ascii(unsigned char decimal_digit) {
	char ascii_code = 0;
	if (decimal_digit < 10) {
		ascii_code = decimal_digit + '0';
	}
	return ascii_code;
}

/* CONVERTS A 4-BITS VARIABLE TO THE ASCII CODE OF THE CORRESPONDING HEXADECIMAL CHARACTER IN ASCII.
 * @param n:			Hexadecimal digit.
 * @return ascii_code:	Corresponding ASCII code.
 */
char STRING_hexa_to_ascii(unsigned char hexa_digit) {
	char ascii_code = 0;
	if (hexa_digit <= 15) {
		ascii_code = (hexa_digit <= 9 ? (char) (hexa_digit + '0') : (char) (hexa_digit + ('A' - 10)));
	}
	return ascii_code;
}

/* CHECK IF A GIVEN ASCII CODE CORRESPONDS TO AN HEXADECIMAL CHARACTER.
 * @param ascii_code:	The byte to analyse.
 * @return:				1 if the byte is the ASCII code of an hexadecimal character, 0 otherwise.
 */
unsigned char STRING_is_hexa_char(char ascii_code) {
	return (((ascii_code >= '0') && (ascii_code <= '9')) || ((ascii_code >= 'A') && (ascii_code <= 'F')));
}

/* CHECK IF A GIVEN ASCII CODE CORRESPONDS TO A DECIMAL CHARACTER.
 * @param ascii_code:	The byte to analyse.
 * @return:				1 if the byte is the ASCII code of a decimal character, 0 otherwise.
 */
unsigned char STRING_is_decimal_char(char ascii_code) {
	return ((ascii_code >= '0') && (ascii_code <= '9'));
}

/* CONVERT A VALUE TO A STRING ACCORDING TO INUT FORMAT.
 * @param value:        Value to print.
 * @param format:       Printing format.
 * @param print_prefix: Print base prefix is non zero.
 * @param string:       Output string.
 */
void STRING_convert_value(unsigned int value, STRING_format_t format, unsigned char print_prefix, char* string) {
    // Local variables.
	unsigned char first_non_zero_found = 0;
	unsigned int idx;
    unsigned int string_idx = 0;
	unsigned char current_value = 0;
	unsigned int current_power = 0;
	unsigned int previous_decade = 0;
	// Fill TX buffer according to format.
	switch (format) {
	case STRING_FORMAT_BINARY:
		if (print_prefix != 0) {
			// Print "0b" prefix.
            string[string_idx++] = '0';
            string[string_idx++] = 'b';
		}
		// Maximum 32 bits.
		for (idx=31 ; idx>=0 ; idx--) {
			if (value & (0b1 << idx)) {
				string[string_idx++] = '1';
				first_non_zero_found = 1;
			}
			else {
				if ((first_non_zero_found != 0) || (idx == 0)) {
					string[string_idx++] = '0';
				}
			}
			if (idx == 0) {
				break;
			}
		}
		break;
	case STRING_FORMAT_HEXADECIMAL:
		if (print_prefix != 0) {
			// Print "0x" prefix.
			string[string_idx++] = '0';
			string[string_idx++] = 'x';
		}
		// Maximum 4 bytes.
		for (idx=3 ; idx>=0 ; idx--) {
			current_value = (value & (0xFF << (8*idx))) >> (8*idx);
			if (current_value != 0) {
				first_non_zero_found = 1;
			}
			if ((first_non_zero_found != 0) || (idx == 0)) {
				string[string_idx++] = STRING_hexa_to_ascii((current_value & 0xF0) >> 4);
				string[string_idx++] = STRING_hexa_to_ascii(current_value & 0x0F);
			}
			if (idx == 0) {
				break;
			}
		}
		break;
	case STRING_FORMAT_DECIMAL:
		// Maximum 10 digits.
		for (idx=9 ; idx>=0 ; idx--) {
			current_power = MATH_pow_10(idx);
			current_value = (value - previous_decade) / current_power;
			previous_decade += current_value * current_power;
			if (current_value != 0) {
				first_non_zero_found = 1;
			}
			if ((first_non_zero_found != 0) || (idx == 0)) {
				string[string_idx++] = current_value + '0';
			}
			if (idx == 0) {
				break;
			}
		}
		break;
	case STRING_FORMAT_ASCII:
		// Raw byte.
		if (value <= 0xFF) {
			string[string_idx++] = value;
		}
		break;
	}
    // End string.
    string[string_idx++] = STRING_CHAR_NULL;
}
