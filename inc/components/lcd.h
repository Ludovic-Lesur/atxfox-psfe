/*
 * lcd.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef __LCD_H__
#define __LCD_H__

#include "td1208.h"

/*** LCD functions ***/

void LCD_init(void);
void LCD_print(unsigned char row, unsigned char column, char* string, unsigned char string_length);
void LCD_clear(void);
void LCD_print_sigfox_id(unsigned char row, unsigned char sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]);
void LCD_print_value_5_digits(unsigned char row, unsigned char column, unsigned int value);

#endif /* __LCD_H__ */
