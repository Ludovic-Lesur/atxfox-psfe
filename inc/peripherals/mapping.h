/*
 * mapping.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
 */

#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "gpio.h"

/*** MCU mapping ***/

// Analog inputs.
static const GPIO_pin_t GPIO_BANDGAP =				(GPIO_pin_t) {GPIOB, 1, 1, 0};
static const GPIO_pin_t GPIO_VOLTAGE_SENSE =		(GPIO_pin_t) {GPIOB, 1, 0, 0};
static const GPIO_pin_t GPIO_CURRENT_SENSE =		(GPIO_pin_t) {GPIOA, 0, 0, 0};

// LCD.
static const GPIO_pin_t GPIO_LCD_E =				(GPIO_pin_t) {GPIOC, 3, 15, 0};
static const GPIO_pin_t GPIO_LCD_RS =				(GPIO_pin_t) {GPIOC, 3, 14, 0};
static const GPIO_pin_t GPIO_LCD_DB0 =				(GPIO_pin_t) {GPIOA, 0, 1, 0};
static const GPIO_pin_t GPIO_LCD_DB1 =				(GPIO_pin_t) {GPIOA, 0, 2, 0};
static const GPIO_pin_t GPIO_LCD_DB2 =				(GPIO_pin_t) {GPIOA, 0, 3, 0};
static const GPIO_pin_t GPIO_LCD_DB3 =				(GPIO_pin_t) {GPIOA, 0, 4, 0};
static const GPIO_pin_t GPIO_LCD_DB4 =				(GPIO_pin_t) {GPIOA, 0, 5, 0};
static const GPIO_pin_t GPIO_LCD_DB5 =				(GPIO_pin_t) {GPIOA, 0, 6, 0};
static const GPIO_pin_t GPIO_LCD_DB6 =				(GPIO_pin_t) {GPIOA, 0, 7, 0};
static const GPIO_pin_t GPIO_LCD_DB7 =				(GPIO_pin_t) {GPIOA, 0, 8, 0};

// Current range (relays).
static const GPIO_pin_t GPIO_TRCS_BYPASS =			(GPIO_pin_t) {GPIOB, 1, 7, 0};
static const GPIO_pin_t GPIO_TRCS_RANGE_LOW =		(GPIO_pin_t) {GPIOB, 1, 6, 0};
static const GPIO_pin_t GPIO_TRCS_RANGE_MIDDLE =	(GPIO_pin_t) {GPIOB, 1, 3, 0};
static const GPIO_pin_t GPIO_TRCS_RANGE_HIGH =		(GPIO_pin_t) {GPIOA, 0, 15, 0};

// USART.
static const GPIO_pin_t GPIO_USART2_TX =			(GPIO_pin_t) {GPIOA, 0, 9, 4};
static const GPIO_pin_t GPIO_USART2_RX =			(GPIO_pin_t) {GPIOA, 0, 10, 4};

// LPUART.
static const GPIO_pin_t GPIO_LPUART1_TX =			(GPIO_pin_t) {GPIOA, 0, 14, 6};
static const GPIO_pin_t GPIO_LPUART1_RX =			(GPIO_pin_t) {GPIOA, 0, 13, 6};

#endif /* __MAPPING_H__ */
