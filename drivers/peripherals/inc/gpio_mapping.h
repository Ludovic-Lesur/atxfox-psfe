/*
 * gpio_mapping.h
 *
 *  Created on: 06 oct. 2024
 *      Author: Ludo
 */

#ifndef __GPIO_MAPPING_H__
#define __GPIO_MAPPING_H__

#include "adc.h"
#include "gpio.h"
#include "lpuart.h"
#include "usart.h"

/*** GPIO MAPPING structures ***/

/*!******************************************************************
 * \enum GPIO_adc_channel_t
 * \brief GPIO ADC channels list.
 *******************************************************************/
typedef enum {
    GPIO_ADC_CHANNEL_BANDGAP_MEASURE = 0,
    GPIO_ADC_CHANNEL_VOUT_MEASURE,
    GPIO_ADC_CHANNEL_IOUT_MEASURE,
    GPIO_ADC_CHANNEL_LAST
} GPIO_adc_channel_t;

/*** GPIO MAPPING global variables ***/

// Analog inputs.
extern const ADC_gpio_t GPIO_ADC_GPIO;
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
// TD1208.
extern const USART_gpio_t GPIO_TD1208_USART;
// Log interface.
extern const LPUART_gpio_t GPIO_LOG_LPUART;

#endif /* __GPIO_MAPPING_H__ */
