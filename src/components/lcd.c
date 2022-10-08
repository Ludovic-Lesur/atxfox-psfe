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

/*** LCD local macros ***/

#define LCD_ROW_IDX_MAX		1
#define LCD_COLUMN_IDX_MAX	7

/*** LCD local functions ***/

/* CREATE A PULSE ON ENABLE SIGNAL TO WRITE LCD REGISTERS.
 * @param:	None.
 * @return:	None.
 */
static void LCD_enable_pulse(void) {
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
static void LCD_command(uint8_t lcd_command) {
	// Put command on output port.
	GPIOA -> ODR &= 0xFFFFFE01;
	GPIOA -> ODR |= (lcd_command << 1);
	// Send command.
	GPIO_write(&GPIO_LCD_RS, 0);
	LCD_enable_pulse();
}

/* SEND DATA TO LCD SCREEN.
 * @param lcd_data:	Data to send.
 * @return:			None.
 */
static void LCD_data(uint8_t lcd_data) {
	// Put data on output port.
	GPIOA -> ODR &= 0xFFFFFE01;
	GPIOA -> ODR |= (lcd_data << 1);
	// Send command.
	GPIO_write(&GPIO_LCD_RS, 1);
	LCD_enable_pulse();
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
	lptim1_status = LPTIM1_delay_milliseconds(100, 0);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(30, 0);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(10, 0);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	LCD_command(0x30);
	lptim1_status = LPTIM1_delay_milliseconds(10, 0);
	LPTIM1_status_check(LCD_ERROR_BASE_LPTIM);
	LCD_command(0x38); // 8-bits / 2 lines mode.
	LCD_command(0x08); // Display off.
	LCD_command(0x0C); // Display on.
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
LCD_status_t LCD_print(uint8_t row, uint8_t column, char* string, uint8_t string_length) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	// Check parameters.
	if (row > LCD_ROW_IDX_MAX) {
		status = LCD_ERROR_ROW_OVERFLOW;
		goto errors;
	}
	if (column > LCD_COLUMN_IDX_MAX) {
		status = LCD_ERROR_COLUMN_OVERFLOW;
		goto errors;
	}
	// Set position.
	LCD_command(((row * 0x40) + column) + 0x80);
	// Print string.
	uint8_t column_idx = column;
	uint8_t string_idx = 0;
	// Loop until string is printed or screen edge is reached.
	while ((column_idx <= LCD_COLUMN_IDX_MAX) && (column_idx < (column + string_length))) {
		LCD_data(string[string_idx]);
		string_idx++;
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
	LCD_command(0x01);
	lptim1_status = LPTIM1_delay_milliseconds(2, 0);
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
	char sigfox_id_string[2 * SIGFOX_DEVICE_ID_LENGTH_BYTES];
	uint8_t byte_idx = 0;
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
	uint8_t u1, u2, u3, u4, u5;
	uint8_t d1, d2, d3;
	char value_string[5] = {0};
	// Convert value to message.
	if (value < 10000) {
		// Format = u.ddd
		u1 = (value) / (1000);
		d1 = (value - (u1 * 1000)) / (100);
		d2 = (value - (u1 * 1000) - (d1 * 100)) / (10);
		d3 = value - (u1 * 1000) - (d1 * 100) - (d2 * 10);
		string_status = STRING_value_to_string(u1, STRING_FORMAT_DECIMAL, 0, &(value_string[0]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		value_string[1] = '.';
		string_status = STRING_value_to_string(d1, STRING_FORMAT_DECIMAL, 0, &(value_string[2]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(d2, STRING_FORMAT_DECIMAL, 0, &(value_string[3]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(d3, STRING_FORMAT_DECIMAL, 0, &(value_string[4]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
	}
	else if (value < 100000) {
		// Format = uu.dd
		u1 = (value) / (10000);
		u2 = (value - (u1 * 10000)) / (1000);
		d1 = (value - (u1 * 10000) - (u2 * 1000)) / (100);
		d2 = (value - (u1 * 10000) - (u2 * 1000) - (d1 * 100)) / (10);
		string_status = STRING_value_to_string(u1, STRING_FORMAT_DECIMAL, 0, &(value_string[0]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u2, STRING_FORMAT_DECIMAL, 0, &(value_string[1]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		value_string[2] = '.';
		string_status = STRING_value_to_string(d1, STRING_FORMAT_DECIMAL, 0, &(value_string[3]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(d2, STRING_FORMAT_DECIMAL, 0, &(value_string[4]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
	}
	else if (value < 1000000) {
		// Format = uuu.d
		u1 = (value) / (100000);
		u2 = (value - (u1 * 100000)) / (10000);
		u3 = (value - (u1 * 100000) - (u2 * 10000)) / (1000);
		d1 = (value - (u1 * 100000) - (u2 * 10000) - (u3 * 1000)) / (100);
		string_status = STRING_value_to_string(u1, STRING_FORMAT_DECIMAL, 0, &(value_string[0]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u2, STRING_FORMAT_DECIMAL, 0, &(value_string[1]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u3, STRING_FORMAT_DECIMAL, 0, &(value_string[2]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		value_string[3] = '.';
		string_status = STRING_value_to_string(d1, STRING_FORMAT_DECIMAL, 0, &(value_string[4]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
	}
	else {
		// Format = uuuuu
		u1 = (value) / (10000000);
		u2 = (value - (u1 * 10000000)) / (1000000);
		u3 = (value - (u1 * 10000000) - (u2 * 1000000)) / (100000);
		u4 = (value - (u1 * 10000000) - (u2 * 1000000) - (u3 * 100000)) / (10000);
		u5 = (value - (u1 * 10000000) - (u2 * 1000000) - (u3 * 100000) - (u4 * 10000)) / (1000);
		string_status = STRING_value_to_string(u1, STRING_FORMAT_DECIMAL, 0, &(value_string[0]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u2, STRING_FORMAT_DECIMAL, 0, &(value_string[1]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u3, STRING_FORMAT_DECIMAL, 0, &(value_string[2]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u4, STRING_FORMAT_DECIMAL, 0, &(value_string[3]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
		string_status = STRING_value_to_string(u5, STRING_FORMAT_DECIMAL, 0, &(value_string[4]));
		STRING_status_check(LCD_ERROR_BASE_STRING);
	}
	// Print value.
	status = LCD_print(row, column, value_string, 5);
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
 * @param major_version:	Software major version.
 * @param minor_version:	Software minor version.
 * @param commit_index:		Software git commit index.
 * @param dirty_flag:		Software git dirty flag.
 * @return status:			Function executions status.
 */
LCD_status_t LCD_print_sw_version(uint8_t major_version, uint8_t minor_version, uint8_t commit_index, uint8_t dirty_flag) {
	// Local variables.
	LCD_status_t status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	char sw_version_string[LCD_COLUMN_IDX_MAX + 1];
	// Build string.
	string_status = STRING_value_to_string((major_version / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[0]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	string_status = STRING_value_to_string((major_version - 10 * (major_version / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[1]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	sw_version_string[2] = STRING_CHAR_DOT;
	string_status = STRING_value_to_string((minor_version / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[3]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	string_status = STRING_value_to_string((minor_version - 10 * (minor_version / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[4]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	sw_version_string[5] = STRING_CHAR_DOT;
	string_status = STRING_value_to_string((commit_index / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[6]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	string_status = STRING_value_to_string((commit_index - 10 * (commit_index / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[7]));
	STRING_status_check(LCD_ERROR_BASE_STRING);
	// Print version.
	if (dirty_flag == 0) {
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
