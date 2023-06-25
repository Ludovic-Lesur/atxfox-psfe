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
#include "tim.h"
#include "types.h"
#include "version.h"

/*** LCD local macros ***/

#define LCD_ROW_IDX_MAX		1
#define LCD_COLUMN_IDX_MAX	7

/*** LCD local functions ***/

/* CREATE A PULSE ON ENABLE SIGNAL TO WRITE LCD REGISTERS.
 * @param:	None.
 * @return:	None.
 */
static void _LCD_enable_pulse(void) {
	// Local variables.
	uint32_t loop_count = 0;
	// Pulse width > 460ns.
	for (loop_count=0 ; loop_count<5 ; loop_count++) {
		GPIO_write(&GPIO_LCD_E, ((loop_count > 0x01) ? 1 : 0));
	}
	GPIO_write(&GPIO_LCD_E, 0);
}

/* SEND A COMMAND TO LCD SCREEN.
 * @param lcd_command:	Command to send.
 * @return:				None.
 */
static void _LCD_command(uint8_t lcd_command) {
	// Put command on output port.
	GPIOA -> ODR &= 0xFFFFFE01;
	GPIOA -> ODR |= (lcd_command << 1);
	// Send command.
	GPIO_write(&GPIO_LCD_RS, 0);
	_LCD_enable_pulse();
}

/* SEND DATA TO LCD SCREEN.
 * @param lcd_data:	Data to send.
 * @return:			None.
 */
static void _LCD_data(uint8_t lcd_data) {
	// Put data on output port.
	GPIOA -> ODR &= 0xFFFFFE01;
	GPIOA -> ODR |= (lcd_data << 1);
	// Send command.
	GPIO_write(&GPIO_LCD_RS, 1);
	_LCD_enable_pulse();
}

/*** LCD functions ***/

/* INIT LCD INTERFACE.
 * @param:			None.
 * @return status:	Function executions status.
 */
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
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	_LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(30, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	_LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(10, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	_LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(10, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	_LCD_command(0x38); // 8-bits / 2 lines mode.
	_LCD_command(0x08); // Display off.
	_LCD_command(0x0C); // Display on.
errors:
	LCD_clear();
	return status;
}

/* PRINT A STRING ON LCD SCREEN.
 * @param row :     		Current row (0 or 1).
 * @param position  		Cursor position (0 to 7).
 * @param string:			Char array to print.
 * @param string_length:	Number of characters to print.
 * @return status:			Function executions status.
 */
LCD_status_t LCD_print(uint8_t row, uint8_t column, char_t* string, uint8_t string_length) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	uint8_t column_idx = column;
	uint8_t char_idx = 0;
	// Check parameters.
	if (string == NULL) {
		status = LCD_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (row > LCD_ROW_IDX_MAX) {
		status = LCD_ERROR_ROW_OVERFLOW;
		goto errors;
	}
	if (column > LCD_COLUMN_IDX_MAX) {
		status = LCD_ERROR_COLUMN_OVERFLOW;
		goto errors;
	}
	// Set position.
	_LCD_command(((row * 0x40) + column) + 0x80);
	// Loop until string is printed or screen edge is reached.
	while ((column_idx <= LCD_COLUMN_IDX_MAX) && (column_idx < (column + string_length))) {
		_LCD_data(string[char_idx]);
		char_idx++;
		column_idx++;
	}
errors:
	return status;
}

/* CLEAR LCD SCREEN.
 * @param:			None.
 * @return status:	Function executions status.
 */
LCD_status_t LCD_clear(void) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	// Clear command.
	_LCD_command(0x01);
	lptim1_status = LPTIM1_delay_milliseconds(2, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
errors:
	return status;
}

/* PRINT A SIGFOX DEVICE ID.
 * @param sigfox_id:	ID to print.
 * @return status:		Function executions status.
 */
LCD_status_t LCD_print_sigfox_id(uint8_t sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	char_t sigfox_id_string[2 * SIGFOX_DEVICE_ID_LENGTH_BYTES];
	uint8_t byte_idx = 0;
	// Check parameter.
	if (sigfox_id == NULL) {
		status = LCD_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build corresponding string.
	for (byte_idx=0 ; byte_idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; byte_idx++) {
		string_status = STRING_value_to_string(sigfox_id[byte_idx], STRING_FORMAT_HEXADECIMAL, 0, &(sigfox_id_string[2 * byte_idx]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
	}
	status = LCD_print(0, 0, " SFX ID ", 8);
	if (status != LCD_SUCCESS) goto errors;
	// Print ID.
	status = LCD_print(1, 0, sigfox_id_string, (2 * SIGFOX_DEVICE_ID_LENGTH_BYTES));
	if (status != LCD_SUCCESS) goto errors;
errors:
	return status;
}

/* PRINT A VALUE ON 5 DIGITS.
 * @param value:	(value * 1000) to print.
 * @return status:	Function executions status.
 */
LCD_status_t LCD_print_value_5_digits(uint8_t row, uint8_t column, uint32_t value) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	char_t value_string[STRING_DIGIT_FUNCTION_SIZE] = {STRING_CHAR_NULL};
	// Convert to 5 digits.
	string_status = STRING_value_to_5_digits_string(value, value_string);
	STRING_status_check(LCD_ERROR_BASE_STRING);
	// Print value.
	status = LCD_print(row, column, value_string, STRING_DIGIT_FUNCTION_SIZE);
errors:
	return status;
}

/* PRINT HARDWARE VERSION.
 * @param:	None.
 * @return:	None.
 */
LCD_status_t LCD_print_hw_version(void) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	// Print version.
	status = LCD_print(0, 0, " ATXFox ", 8);
	if (status != LCD_SUCCESS) goto errors;
	status = LCD_print(1, 0, " HW 1.0 ", 8);
errors:
	return status;
}

/* PRINT SOFTWARE VERSION.
 * @param:			None.
 * @return status:	Function executions status.
 */
LCD_status_t LCD_print_sw_version(void) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	char_t sw_version_string[LCD_COLUMN_IDX_MAX + 1];
	// Build string.
	string_status = STRING_value_to_string((GIT_MAJOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[0]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	string_status = STRING_value_to_string((GIT_MAJOR_VERSION - 10 * (GIT_MAJOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[1]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	sw_version_string[2] = STRING_CHAR_DOT;
	string_status = STRING_value_to_string((GIT_MINOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[3]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	string_status = STRING_value_to_string((GIT_MINOR_VERSION - 10 * (GIT_MINOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[4]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	sw_version_string[5] = STRING_CHAR_DOT;
	string_status = STRING_value_to_string((GIT_COMMIT_INDEX / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[6]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	string_status = STRING_value_to_string((GIT_COMMIT_INDEX - 10 * (GIT_COMMIT_INDEX / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[7]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	// Print version.
	if (GIT_DIRTY_FLAG == 0) {
		status = LCD_print(0, 0, "   SW   ", 8);
	}
	else {
		status = LCD_print(0, 0, "  SW !  ", 8);
	}
	if (status != LCD_SUCCESS) goto errors;
	status = LCD_print(1, 0, sw_version_string, 8);
errors:
	return status;
}
