/*
 * mapping.h
 *
 *  Created on: 30 dec. 2018
 *      Author: Ludo
 */

#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "gpio.h"

/*** MAPPING global variables ***/

// Analog inputs.
extern const GPIO_pin_t GPIO_ADC_IN9;
extern const GPIO_pin_t GPIO_ADC_IN8;
extern const GPIO_pin_t GPIO_ADC_IN0;
// LCD.
extern const GPIO_pin_t GPIO_LCD_E;
extern const GPIO_pin_t GPIO_LCD_RS;
extern const GPIO_pin_t GPIO_LCD_DB0;
extern const GPIO_pin_t GPIO_LCD_DB1;
extern const GPIO_pin_t GPIO_LCD_DB2;
extern const GPIO_pin_t GPIO_LCD_DB3;
extern const GPIO_pin_t GPIO_LCD_DB4;
extern const GPIO_pin_t GPIO_LCD_DB5;
extern const GPIO_pin_t GPIO_LCD_DB6;
extern const GPIO_pin_t GPIO_LCD_DB7;
// Current range (TRCS board control).
extern const GPIO_pin_t GPIO_TRCS_BYPASS;
extern const GPIO_pin_t GPIO_TRCS_RANGE_LOW;
extern const GPIO_pin_t GPIO_TRCS_RANGE_MIDDLE;
extern const GPIO_pin_t GPIO_TRCS_RANGE_HIGH;
// USART.
extern const GPIO_pin_t GPIO_USART2_TX;
extern const GPIO_pin_t GPIO_USART2_RX;
// LPUART.
extern const GPIO_pin_t GPIO_LPUART1_TX;
extern const GPIO_pin_t GPIO_LPUART1_RX;
// Programming.
extern const GPIO_pin_t GPIO_SWDIO;
extern const GPIO_pin_t GPIO_SWCLK;

#endif /* __MAPPING_H__ */
