/*
 * lcd.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef __LCD_H__
#define __LCD_H__

#include "lptim.h"
#include "string.h"
#include "td1208.h"
#include "types.h"

/*** LCD structures ***/

typedef enum {
	LCD_SUCCESS = 0,
	LCD_ERROR_ROW_OVERFLOW,
	LCD_ERROR_COLUMN_OVERFLOW,
	LCD_ERROR_BASE_LPTIM = 0x0100,
	LCD_ERROR_BASE_STRING = (LCD_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	LCD_ERROR_BASE_LAST = (LCD_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST)
} LCD_status_t;

/*** LCD functions ***/

LCD_status_t LCD_init(void);
LCD_status_t LCD_print(uint8_t row, uint8_t column, char_t* string, uint8_t string_length);
LCD_status_t LCD_clear(void);
LCD_status_t LCD_print_sigfox_id(uint8_t sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]);
LCD_status_t LCD_print_value_5_digits(uint8_t row, uint8_t column, uint32_t value);
LCD_status_t LCD_print_hw_version(void);
LCD_status_t LCD_print_sw_version(uint8_t major_version, uint8_t minor_version, uint8_t commit_index, uint8_t dirty_flag);

#define LCD_status_check(error_base) { if (lcd_status != LCD_SUCCESS) { status = error_base + lcd_status; goto errors; }}
#define LCD_error_check() { ERROR_status_check(lcd_status, LCD_SUCCESS, ERROR_BASE_LCD); }
#define LCD_error_check_print() { ERROR_status_check_print(lcd_status, LCD_SUCCESS, ERROR_BASE_LCD); }

#endif /* __LCD_H__ */
