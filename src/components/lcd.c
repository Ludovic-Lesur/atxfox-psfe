/*
 * lcd.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "lcd.h"

#include "gpio.h"
#include "gpio_reg.h"
#include "lptim.h"
#include "mapping.h"
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
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
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
	lptim1_status = LPTIM1_delay_milliseconds(100, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(30, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(10, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_exit_error(LCD_ERROR_BASE_LPTIM1);
	_LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(10, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_exit_error(LCD_ERROR_BASE_LPTIM1);
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
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	// Clear command.
	_LCD_command(0x01);
	lptim1_status = LPTIM1_delay_milliseconds(2, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_exit_error(LCD_ERROR_BASE_LPTIM1);
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
LCD_status_t LCD_print_value_5_digits(uint8_t row, uint8_t column, uint32_t value) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	char_t value_string[STRING_DIGIT_FUNCTION_SIZE] = {STRING_CHAR_NULL};
	// Convert to 5 digits.
	string_status = STRING_value_to_5_digits_string(value, value_string);
	STRING_exit_error(LCD_ERROR_BASE_STRING);
	// Print value.
	status = LCD_print_string(row, column, value_string);
errors:
	return status;
}
