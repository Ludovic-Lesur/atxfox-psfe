/*
 * mapping.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
 */

#ifndef MAPPING_H
#define MAPPING_H

#include "gpio.h"

/*** MCU mapping ***/

// Analog inputs.
static const GPIO GPIO_BANDGAP =			(GPIO) {GPIOB, 1, 1, 0};
static const GPIO GPIO_VOLTAGE_SENSE =		(GPIO) {GPIOB, 1, 0, 0};
static const GPIO GPIO_CURRENT_SENSE =		(GPIO) {GPIOA, 0, 0, 0};

// LCD.
static const GPIO GPIO_LCD_E =				(GPIO) {GPIOC, 3, 15, 0};
static const GPIO GPIO_LCD_RS =				(GPIO) {GPIOC, 3, 14, 0};
static const GPIO GPIO_LCD_DB0 =			(GPIO) {GPIOA, 0, 1, 0};
static const GPIO GPIO_LCD_DB1 =			(GPIO) {GPIOA, 0, 2, 0};
static const GPIO GPIO_LCD_DB2 =			(GPIO) {GPIOA, 0, 3, 0};
static const GPIO GPIO_LCD_DB3 =			(GPIO) {GPIOA, 0, 4, 0};
static const GPIO GPIO_LCD_DB4 =			(GPIO) {GPIOA, 0, 5, 0};
static const GPIO GPIO_LCD_DB5 =			(GPIO) {GPIOA, 0, 6, 0};
static const GPIO GPIO_LCD_DB6 =			(GPIO) {GPIOA, 0, 7, 0};
static const GPIO GPIO_LCD_DB7 =			(GPIO) {GPIOA, 0, 8, 0};

// Current range (relays).
static const GPIO GPIO_BYPASS =				(GPIO) {GPIOB, 1, 7, 0};
static const GPIO GPIO_TRCS_RANGE_LOW =		(GPIO) {GPIOB, 1, 6, 0};
static const GPIO GPIO_TRCS_RANGE_MIDDLE =	(GPIO) {GPIOB, 1, 3, 0};
static const GPIO GPIO_TRCS_RANGE_HIGH =	(GPIO) {GPIOA, 0, 15, 0};

// USART.
static const GPIO GPIO_USART2_TX =			(GPIO) {GPIOA, 0, 9, 4};
static const GPIO GPIO_USART2_RX =			(GPIO) {GPIOA, 0, 10, 4};

// LPUART.
static const GPIO GPIO_LPUART1_TX =			(GPIO) {GPIOA, 0, 14, 6};
static const GPIO GPIO_LPUART1_RX =			(GPIO) {GPIOA, 0, 13, 6};

#endif /* MAPPING_H */
