/*
 * lcd.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "lcd.h"

#include "error.h"
#include "gpio.h"
#include "gpio_reg.h"
#include "lptim.h"
#include "gpio_mapping.h"
#include "string.h"
#include "types.h"

/*** LCD local functions ***/

/*******************************************************************/
static void _LCD_enable_pulse(void) {
	// Local variables.
	uint32_t loop_count = 0;
	// Pulse width > 460ns.
	for (loop_count=0 ; loop_count<5 ; loop_count++) {
		GPIO_write(&GPIO_LCD_E, ((loop_count > 0x01) ? 1 : 0));
	}
	GPIO_write(&GPIO_LCD_E, 0);
}

/*******************************************************************/
static void _LCD_command(uint8_t lcd_command) {
	// Put command on output port.
	GPIOA -> ODR &= 0xFFFFFE01;
	GPIOA -> ODR |= (lcd_command << 1);
	// Send command.
	GPIO_write(&GPIO_LCD_RS, 0);
	_LCD_enable_pulse();
}

/*******************************************************************/
static void _LCD_data(uint8_t lcd_data) {
	// Put data on output port.
	GPIOA -> ODR &= 0xFFFFFE01;
	GPIOA -> ODR |= (lcd_data << 1);
	// Send command.
	GPIO_write(&GPIO_LCD_RS, 1);
	_LCD_enable_pulse();
}

/*** LCD functions ***/

/*******************************************************************/
LCD_status_t LCD_init(void) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	// Init GPIOs.
	GPIO_configure(&GPIO_LCD_E, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_RS, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB0, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB1, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB2, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB3, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB4, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB5, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB6, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LCD_DB7, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Initialization sequence.
	GPIO_write(&GPIO_LCD_E, 0);
	lptim_status = LPTIM_delay_milliseconds(100, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x30);
	lptim_status = LPTIM_delay_milliseconds(30, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x30);
	lptim_status = LPTIM_delay_milliseconds(10, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x30);
	lptim_status = LPTIM_delay_milliseconds(10, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x38); // 8-bits / 2 lines mode.
	_LCD_command(0x08); // Display off.
	_LCD_command(0x0C); // Display on.
errors:
	LCD_clear();
	return status;
}

/*******************************************************************/
LCD_status_t LCD_clear(void) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	// Clear command.
	_LCD_command(0x01);
	lptim_status = LPTIM_delay_milliseconds(2, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_exit_error(LCD_ERROR_BASE_LPTIM1);
errors:
	return status;
}

/*******************************************************************/
LCD_status_t LCD_print_string(uint8_t row, uint8_t column, char_t* str) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	uint8_t column_idx = column;
	uint8_t char_idx = 0;
	// Check parameters.
	if (str == NULL) {
		status = LCD_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (row >= LCD_HEIGHT_CHAR) {
		status = LCD_ERROR_ROW_OVERFLOW;
		goto errors;
	}
	if (column >= LCD_WIDTH_CHAR) {
		status = LCD_ERROR_COLUMN_OVERFLOW;
		goto errors;
	}
	// Set position.
	_LCD_command(((row * 0x40) + column) + 0x80);
	// Loop until string is printed or screen edge is reached.
	column_idx = column;
	while ((column_idx <= LCD_WIDTH_CHAR) && (str[char_idx] != STRING_CHAR_NULL)) {
		_LCD_data(str[char_idx]);
		char_idx++;
		column_idx++;
	}
errors:
	return status;
}

/*******************************************************************/
LCD_status_t LCD_print_value(uint8_t row, int32_t value, uint8_t divider_exponent, char_t* unit) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	char_t lcd_string[LCD_WIDTH_CHAR] = {STRING_CHAR_NULL};
	uint8_t number_of_digits = 0;
	uint32_t str_size = 0;
	// Get unit size.
	string_status = STRING_get_size(unit, &str_size);
	STRING_exit_error(LCD_ERROR_BASE_STRING);
	// Check size.
	if (str_size > (LCD_WIDTH_CHAR >> 1)) {
	    status = LCD_ERROR_UNIT_SIZE_OVERFLOW;
	    goto errors;
	}
	number_of_digits = (LCD_WIDTH_CHAR - str_size - 1);
	// Convert to floating point representation.
	string_status = STRING_integer_to_floating_decimal_string(value, divider_exponent, number_of_digits, lcd_string);
	STRING_exit_error(LCD_ERROR_BASE_STRING);
	// Add space.
	lcd_string[number_of_digits] = STRING_CHAR_SPACE;
	str_size = (number_of_digits + 1);
	// Append unit.
	string_status = STRING_append_string(lcd_string, LCD_WIDTH_CHAR, unit, &str_size);
	STRING_exit_error(LCD_ERROR_BASE_STRING);
	// Print value.
	status = LCD_print_string(row, 0, lcd_string);
errors:
	return status;
}
