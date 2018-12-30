/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"
#include "gpio.h"
#include "lcd.h"
#include "mapping.h"
#include "range.h"
#include "rcc.h"
#include "tim.h"
#include "usart.h"

/*** Main macros ***/

// Power supply selection.
#define ATX_3V3
//#define ATX_5V
//#define ATX_12V

/*** Main structures ***/

// State machine.
typedef enum {
	ATXFE_STATE_INIT,
	ATXFE_STATE_BYPASS,
	ATXFE_STATE_MEASURE,
	ATXFE_STATE_RANGE,
	ATXFE_STATE_LOG
} ATXFE_State;

// Context.
typedef struct {
	ATXFE_State atxfe_state;
	unsigned char atxfe_bypass;
	RANGE_Level atxfe_range_level;
	unsigned int atxfe_mcu_voltage_mv;
	unsigned int atxfe_atx_voltage_mv;
	unsigned int atxfe_atx_current_ma;
	unsigned int atxfe_seconds_count;
} ATXFE_Context;

/*** Main global variables ***/

ATXFE_Context atxfe_ctx;

/*** Main function ***/

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {

	/* Init clock */
	RCC_Init();

	/* Init peripherals */
	TIM21_Init();
	TIM22_Init();
	GPIO_Init();
	ADC1_Init();
	USART2_Init();

	/* Init components */
	LCD_Init();
	LCD_Clear();

	/* Init applicative layers */

	/* Init context and global variables */
	unsigned int adc_result_12bits = 0;
	atxfe_ctx.atxfe_state = ATXFE_STATE_INIT;
	atxfe_ctx.atxfe_bypass = 0;
	atxfe_ctx.atxfe_range_level = RANGE_HIGH;
	atxfe_ctx.atxfe_mcu_voltage_mv = 0;
	atxfe_ctx.atxfe_atx_voltage_mv = 0;
	atxfe_ctx.atxfe_atx_current_ma = 0;
	atxfe_ctx.atxfe_seconds_count = 0;

	/* Main loop */
	while (1) {

		/* Perform state machine */
		switch (atxfe_ctx.atxfe_state) {

		/* Init */
		case ATXFE_STATE_INIT:
			// Print project name and HW/SW versions.
			LCD_Print(0, 1, "ATX-FE", 6);
			LCD_Print(1, 1, "HW 1.0", 6);
			TIM22_WaitMilliseconds(2000);
			LCD_Clear();
			LCD_Print(0, 7, "V", 1);
			LCD_Print(1, 6, "mA", 2);
			// Compute next state.
			atxfe_ctx.atxfe_state = ATXFE_STATE_BYPASS;
			break;

		/* Get bypass switch status */
		case ATXFE_STATE_BYPASS:
			atxfe_ctx.atxfe_bypass = GPIO_Read(GPIO_BYPASS);
			// Compute next state.
			atxfe_ctx.atxfe_state = ATXFE_STATE_MEASURE;
			break;

		/* Analog measurements */
		case ATXFE_STATE_MEASURE:
			// Compute effective supply voltage.
			ADC1_GetChannel12Bits(ADC_BANDGAP_CHANNEL, &adc_result_12bits);
			atxfe_ctx.atxfe_mcu_voltage_mv = (ADC_BANDGAP_VOLTAGE_MV * ADC_FULL_SCALE_12BITS) / (adc_result_12bits);
			// Compute effective ATX output voltage.
			ADC1_GetChannel12Bits(ADC_ATX_VOLTAGE_CHANNEL, &adc_result_12bits);
#ifdef ATX_3V3
			// Resistor divider = (*2).
			atxfe_ctx.atxfe_atx_voltage_mv = 2 * ((adc_result_12bits * atxfe_ctx.atxfe_mcu_voltage_mv) / (ADC_FULL_SCALE_12BITS));
#endif
#ifdef ATX_5V
			// Resistor divider = (*2).
			atxfe_ctx.atxfe_atx_voltage_mv = 2 * ((adc_result_12bits * atxfe_ctx.atxfe_mcu_voltage_mv) / (ADC_FULL_SCALE_12BITS));
#endif
#ifdef ATX_12v
			// Resistor divider = (*6).
			atxfe_ctx.atxfe_atx_voltage_mv = 6 * ((adc_result_12bits * atxfe_ctx.atxfe_mcu_voltage_mv) / (ADC_FULL_SCALE_12BITS));
#endif
			// Compute output current.
			ADC1_GetChannel12Bits(ADC_ATX_CURRENT_CHANNEL, &adc_result_12bits);
			// TBD depending on current range.
			// Compute next state.
			atxfe_ctx.atxfe_state = ATXFE_STATE_RANGE;
			break;

		/* Update current range according to previous measure */
		case ATXFE_STATE_RANGE:
			// TBD.
			// Compute next state.
			atxfe_ctx.atxfe_state = ATXFE_STATE_LOG;
			break;

		/* Print voltage and current on LCD screen and UART */
		case ATXFE_STATE_LOG:
			if (TIM22_GetSeconds() > atxfe_ctx.atxfe_seconds_count) {
				// TBD.
				atxfe_ctx.atxfe_seconds_count = TIM22_GetSeconds();
			}

			// Compute next state.
			atxfe_ctx.atxfe_state = ATXFE_STATE_BYPASS;
			break;

		/* Unknown state */
		default:
			break;
		}
	}

	return 0;
}

/*** Error management ***/
#if (((defined ATX_3V3) && (defined ATX_5V)) || \
	((defined ATX_3V3) && (defined ATX_12V)) || \
	((defined ATX_5V) && (defined ATX_12V)))
#error "Only one power supply must be selected: ATX3V3, ATX5V or ATX12V"
#endif
