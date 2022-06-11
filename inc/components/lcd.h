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
LCD_status_t LCD_print(unsigned char row, unsigned char column, char* string, unsigned char string_length);
LCD_status_t LCD_clear(void);
LCD_status_t LCD_print_sigfox_id(unsigned char row, unsigned char sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]);
LCD_status_t LCD_print_value_5_digits(unsigned char row, unsigned char column, unsigned int value);

#endif /* __LCD_H__ */
