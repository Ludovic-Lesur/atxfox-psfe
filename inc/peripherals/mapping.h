/*
 * mapping.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
 */

#ifndef MAPPING_H
#define MAPPING_H

/*** MCU mapping ***/

// Analog inputs.
#define GPIO_BANDGAP		(GPIO) {GPIOB, 1, 0}
#define GPIO_VOLTAGE_SENSE	(GPIO) {GPIOB, 0, 0}
#define GPIO_CURRENT_SENSE	(GPIO) {GPIOA, 0, 0}

// LCD.
//#define GPIO_LCD_E		(GPIO) {GPIOC, 14, 0}
#define GPIO_LCD_E			(GPIO) {GPIOB, 3, 0}
//#define GPIO_LCD_RS		(GPIO) {GPIOC, 15, 0}
#define GPIO_LCD_RS			(GPIO) {GPIOB, 4, 0}
#define GPIO_LCD_DB0		(GPIO) {GPIOA, 1, 0}
#define GPIO_LCD_DB1		(GPIO) {GPIOA, 2, 0}
#define GPIO_LCD_DB2		(GPIO) {GPIOA, 3, 0}
#define GPIO_LCD_DB3		(GPIO) {GPIOA, 4, 0}
#define GPIO_LCD_DB4		(GPIO) {GPIOA, 5, 0}
#define GPIO_LCD_DB5		(GPIO) {GPIOA, 6, 0}
#define GPIO_LCD_DB6		(GPIO) {GPIOA, 7, 0}
#define GPIO_LCD_DB7		(GPIO) {GPIOA, 8, 0}

// Current range (relays).
#define GPIO_BYPASS			(GPIO) {GPIOB, 6, 0}
#define GPIO_RANGE_LOW		(GPIO) {GPIOA, 15, 0}
#define GPIO_RANGE_MIDDLE	(GPIO) {GPIOB, 3, 0}
#define GPIO_RANGE_HIGH		(GPIO) {GPIOB, 7, 0}

// USART.
#define GPIO_USART2_TX		(GPIO) {GPIOA, 9, 4}
#define GPIO_USART2_RX		(GPIO) {GPIOA, 10, 4}

#endif /* MAPPING_H */
