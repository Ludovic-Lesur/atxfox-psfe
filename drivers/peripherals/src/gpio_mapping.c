/*
 * gpio_mapping.c
 *
 *  Created on: 06 oct. 2024
 *      Author: Ludo
 */

#include "gpio_mapping.h"

#include "adc.h"
#include "gpio.h"
#include "gpio_reg.h"
#include "lpuart.h"
#include "mode.h"
#include "usart.h"

/*** GPIO MAPPING local global variables ***/

// Analog inputs.
static const GPIO_pin_t GPIO_ADC_BANDGAP_MEASURE = { GPIOB, 1, 1, 0 };
static const GPIO_pin_t GPIO_ADC_VOUT_MEASURE = { GPIOB, 1, 0, 0 };
static const GPIO_pin_t GPIO_ADC_IOUT_MEASURE = { GPIOA, 0, 0, 0 };
// Analog inputs list.
static const GPIO_pin_t* GPIO_ADC_PINS_LIST[GPIO_ADC_CHANNEL_LAST] = { &GPIO_ADC_BANDGAP_MEASURE, &GPIO_ADC_VOUT_MEASURE, &GPIO_ADC_IOUT_MEASURE };
#ifdef PSFE_SERIAL_MONITORING
// LPUART.
static const GPIO_pin_t GPIO_LPUART1_TX = { GPIOA, 0, 14, 6 };
static const GPIO_pin_t GPIO_LPUART1_RX = { GPIOA, 0, 13, 6 };
#endif
#ifdef PSFE_SIGFOX_MONITORING
// USART.
static const GPIO_pin_t GPIO_USART2_TX = { GPIOA, 0, 9, 4 };
static const GPIO_pin_t GPIO_USART2_RX = { GPIOA, 0, 10, 4 };
#endif

/*** GPIO MAPPING global variables ***/

// Analog inputs.
const ADC_gpio_t GPIO_ADC_GPIO = { (const GPIO_pin_t**) &GPIO_ADC_PINS_LIST, GPIO_ADC_CHANNEL_LAST };
// LCD.
const GPIO_pin_t GPIO_LCD_E = { GPIOC, 3, 15, 0 };
const GPIO_pin_t GPIO_LCD_RS = { GPIOC, 3, 14, 0 };
const GPIO_pin_t GPIO_LCD_DB0 = { GPIOA, 0, 1, 0 };
const GPIO_pin_t GPIO_LCD_DB1 = { GPIOA, 0, 2, 0 };
const GPIO_pin_t GPIO_LCD_DB2 = { GPIOA, 0, 3, 0 };
const GPIO_pin_t GPIO_LCD_DB3 = { GPIOA, 0, 4, 0 };
const GPIO_pin_t GPIO_LCD_DB4 = { GPIOA, 0, 5, 0 };
const GPIO_pin_t GPIO_LCD_DB5 = { GPIOA, 0, 6, 0 };
const GPIO_pin_t GPIO_LCD_DB6 = { GPIOA, 0, 7, 0 };
const GPIO_pin_t GPIO_LCD_DB7 = { GPIOA, 0, 8, 0 };
// Current range (TRCS board control).
const GPIO_pin_t GPIO_TRCS_BYPASS = { GPIOB, 1, 7, 0 };
const GPIO_pin_t GPIO_TRCS_RANGE_LOW = { GPIOB, 1, 6, 0 };
const GPIO_pin_t GPIO_TRCS_RANGE_MIDDLE = { GPIOB, 1, 3, 0 };
const GPIO_pin_t GPIO_TRCS_RANGE_HIGH = { GPIOA, 0, 15, 0 };
#ifdef PSFE_SERIAL_MONITORING
// Serial interface.
const LPUART_gpio_t GPIO_SERIAL_LPUART = { &GPIO_LPUART1_TX, &GPIO_LPUART1_RX };
#endif
#ifdef PSFE_SIGFOX_MONITORING
// TD1208.
const USART_gpio_t GPIO_TD1208_USART = { &GPIO_USART2_TX, &GPIO_USART2_RX };
#endif
