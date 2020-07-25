/*
 * lcd.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef LCD_H
#define LCD_H

#include "td1208.h"

/*** LCD functions ***/

void LCD_Init(void);
void LCD_Print(unsigned char row, unsigned char column, char* string, unsigned char string_length);
void LCD_Clear(void);
void LCD_PrintSigfoxId(unsigned char row, unsigned char sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]);
void LCD_PrintValue5Digits(unsigned char row, unsigned char column, unsigned int value);

#endif /* LCD_H */
