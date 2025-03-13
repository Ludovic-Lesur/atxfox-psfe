/*
 * mcu_mapping.h
 *
 *  Created on: 06 oct. 2024
 *      Author: Ludo
 */

#ifndef __MCU_MAPPING_H__
#define __MCU_MAPPING_H__

#include "adc.h"
#include "gpio.h"
#include "lpuart.h"
#include "psfe_flags.h"
#include "usart.h"

/*** MCU MAPPING macros ***/

#define ADC_CHANNEL_REF191      ADC_CHANNEL_IN9
#define ADC_CHANNEL_VOUT        ADC_CHANNEL_IN8
#define ADC_CHANNEL_IOUT        ADC_CHANNEL_IN0

#define TIM_INSTANCE_HMI        TIM_INSTANCE_TIM2
#define TIM_INSTANCE_ANALOG     TIM_INSTANCE_TIM21
#define TIM_INSTANCE_TRCS       TIM_INSTANCE_TIM22

#define USART_INSTANCE_TD1208   USART_INSTANCE_USART2

/*** MCU MAPPING structures ***/

/*!******************************************************************
 * \enum GPIO_adc_channel_t
 * \brief GPIO ADC channels list.
 *******************************************************************/
typedef enum {
    ADC_CHANNEL_INDEX_REF191 = 0,
    ADC_CHANNEL_INDEX_VOUT,
    ADC_CHANNEL_INDEX_IOUT,
    ADC_CHANNEL_INDEX_LAST
} ADC_channel_index_t;

/*** MCU MAPPING global variables ***/

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
#ifdef PSFE_SERIAL_MONITORING
// Serial interface.
extern const LPUART_gpio_t LPUART_GPIO_SERIAL;
#endif
#ifdef PSFE_SIGFOX_MONITORING
// TD1208.
extern const USART_gpio_t USART_GPIO_TD1208;
#endif

#endif /* __MCU_MAPPING_H__ */
