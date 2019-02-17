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
#include "rcc.h"
#include "tim.h"
#include "trcs.h"
#include "tst868u.h"
#include "usart.h"

/*** Main macros ***/

// Resistor divider factor used for voltage measurement.
#define PSFE_RESISTOR_DIVIDER_FACTOR	2
// ON/OFF threshold voltages.
#define PSFE_ON_VOLTAGE_THRESHOLD_MV	3300
#define PSFE_OFF_VOLTAGE_THRESHOLD_MV	3200

// Voltage threshold.
#define PSFE_OPEN_VOLTAGE_THRESHOLD_MV	100

/*** Main structures ***/

// State machine.
typedef enum {
	PSFE_STATE_SUPPLY_VOLTAGE_MONITORING,	// Measure supply voltage to start or stop the front-end.
	PSFE_STATE_OFF,							// Power cut detected: switch all peripherals off + send OFF message).
	PSFE_STATE_INIT,						// Power on detected: display initialization screens+ send ON message).
	PSFE_STATE_BYPASS,						// Read bypass input.
	PSFE_STATE_VOLTAGE_MEASURE,				// Perform voltage measurement.
	PSFE_STATE_CURRENT_MEASURE,				// Perform current measurement.
	PSFE_STATE_DISPLAY,						// Update LCD screen.
	PSFE_STATE_LOG							// Send measurements through UART.
} PSFE_State;

// Context.
typedef struct {
	PSFE_State psfe_state;
	unsigned char psfe_init_done;
	unsigned char psfe_no_input_previous_status;
	unsigned char psfe_no_input_current_status;
	unsigned char psfe_bypass_previous_status;
	unsigned char psfe_bypass_current_status;
	unsigned int psfe_supply_voltage_mv;
	unsigned int psfe_atx_voltage_mv;
	unsigned int psfe_atx_current_ua;
	unsigned int psfe_display_seconds_count;
	unsigned int psfe_log_seconds_count;
	unsigned char psfe_sigfox_id[SIGFOX_ID_LENGTH_BYTES];
} PSFE_Context;

/*** Main global variables ***/

static PSFE_Context psfe_ctx;

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
	TRCS_Init();

	/* Init context and global variables */
	unsigned int adc_channel_result_12bits = 0;
	unsigned int adc_bandgap_result_12bits = 0;
	psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
	psfe_ctx.psfe_no_input_current_status = 0;
	psfe_ctx.psfe_no_input_previous_status = 0;
	psfe_ctx.psfe_bypass_current_status = 0;
	psfe_ctx.psfe_bypass_previous_status = 0;
	psfe_ctx.psfe_init_done = 0;
	psfe_ctx.psfe_supply_voltage_mv = 0;
	psfe_ctx.psfe_atx_voltage_mv = 0;
	psfe_ctx.psfe_atx_current_ua = 0;
	psfe_ctx.psfe_display_seconds_count = 0;
	psfe_ctx.psfe_log_seconds_count = 0;

	/* Main loop */
	while (1) {

		/* Perform state machine */
		switch (psfe_ctx.psfe_state) {

		/* Supply voltage monitoring */
		case PSFE_STATE_SUPPLY_VOLTAGE_MONITORING:
			ADC_GetSupplyVoltageMv(&psfe_ctx.psfe_supply_voltage_mv);
			// Power-off detection.
			if (psfe_ctx.psfe_supply_voltage_mv < PSFE_OFF_VOLTAGE_THRESHOLD_MV) {
				psfe_ctx.psfe_state = PSFE_STATE_OFF;
			}
			// Power-on detection.
			if (psfe_ctx.psfe_supply_voltage_mv > PSFE_ON_VOLTAGE_THRESHOLD_MV) {
				if (psfe_ctx.psfe_init_done == 0) {
					psfe_ctx.psfe_state = PSFE_STATE_INIT;
				}
				else {
					psfe_ctx.psfe_state = PSFE_STATE_BYPASS;
				}
			}
			break;

		/* Power-off */
		case PSFE_STATE_OFF:
			// Send Sigfox message and display OFF on LCD screen.
			if (psfe_ctx.psfe_init_done != 0) {
				TRCS_Off();
				LCD_Clear();
				TST868U_SendByte(0x00);
			}
			// Update flag.
			psfe_ctx.psfe_init_done = 0;
			// Compute next state
			psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
			break;

		/* Init */
		case PSFE_STATE_INIT:
			// Print project name and HW/SW versions.
			LCD_Print(0, 0, "  PSFE  ", 8);
			LCD_Print(1, 0, " HW 1.0 ", 8);
			TIM22_WaitMilliseconds(2000);
			// Print Sigfox device ID.
			TST868U_Ping();
			TST868U_GetSigfoxId(psfe_ctx.psfe_sigfox_id);
			LCD_Print(0, 0, " SFX ID ", 8);
			LCD_PrintSigfoxId(1, psfe_ctx.psfe_sigfox_id);
			// Send START message through Sigfox.
			TST868U_SendByte(0x01);
			// Print units.
			LCD_Print(0, 0, "       V", 8);
			LCD_Print(1, 0, "      mA", 8);
			// Update flag.
			psfe_ctx.psfe_init_done = 1;
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_BYPASS;
			break;

		/* Get bypass switch status */
		case PSFE_STATE_BYPASS:
			psfe_ctx.psfe_bypass_current_status = GPIO_Read(GPIO_BYPASS);
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_VOLTAGE_MEASURE;
			break;

		/* Analog measurements */
		case PSFE_STATE_VOLTAGE_MEASURE:
			// Compute effective ATX output voltage.
			ADC1_GetChannel12Bits(ADC_BANDGAP_CHANNEL, &adc_bandgap_result_12bits);
			ADC1_GetChannel12Bits(ADC_ATX_VOLTAGE_CHANNEL, &adc_channel_result_12bits);
			// Compensate resistor divider.
			psfe_ctx.psfe_atx_voltage_mv = PSFE_RESISTOR_DIVIDER_FACTOR * ((adc_channel_result_12bits * ADC_BANDGAP_VOLTAGE_MV) / (adc_bandgap_result_12bits));
			// Detect open state.
			if (psfe_ctx.psfe_atx_voltage_mv < PSFE_OPEN_VOLTAGE_THRESHOLD_MV) {
				psfe_ctx.psfe_no_input_current_status = 1;
			}
			else {
				psfe_ctx.psfe_no_input_current_status = 0;
			}
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_CURRENT_MEASURE;
			break;

		/* Update current range according to previous measure */
		case PSFE_STATE_CURRENT_MEASURE:
			// Use TRCS board.
			TRCS_Task(&psfe_ctx.psfe_atx_current_ua, psfe_ctx.psfe_bypass_current_status);
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_DISPLAY;
			break;

		/* Print voltage and current on LCD screen */
		case PSFE_STATE_DISPLAY:
			if (TIM22_GetSeconds() > psfe_ctx.psfe_display_seconds_count) {
				// Voltage display.
				if (psfe_ctx.psfe_no_input_current_status == 0) {
					if (psfe_ctx.psfe_no_input_previous_status != 0) {
						LCD_Print(0, 0, "       V", 8);
						psfe_ctx.psfe_no_input_previous_status = 0;
					}
					LCD_PrintValue5Digits(0, 0, psfe_ctx.psfe_atx_voltage_mv);
				}
				else {
					if (psfe_ctx.psfe_no_input_previous_status == 0) {
						LCD_Print(0, 0, "NO INPUT", 8);
						psfe_ctx.psfe_no_input_previous_status = 1;
					}
				}
				// Current display.
				if (psfe_ctx.psfe_bypass_current_status == 0) {
					if (psfe_ctx.psfe_bypass_previous_status != 0) {
						LCD_Print(1, 0, "      mA", 8);
						psfe_ctx.psfe_bypass_previous_status = 0;
					}
					LCD_PrintValue5Digits(1, 0, psfe_ctx.psfe_atx_current_ua);
				}
				else {
					if (psfe_ctx.psfe_bypass_previous_status == 0) {
						LCD_Print(1, 0, " BYPASS ", 8);
						psfe_ctx.psfe_bypass_previous_status = 1;
					}
				}
				psfe_ctx.psfe_display_seconds_count = TIM22_GetSeconds();
			}
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_LOG;
			break;

		/* Send voltage and current on UART */
		case PSFE_STATE_LOG:
			if (TIM22_GetSeconds() > psfe_ctx.psfe_log_seconds_count) {
				// TBD.
				psfe_ctx.psfe_log_seconds_count = TIM22_GetSeconds();
			}
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
			break;

		/* Unknown state */
		default:
			psfe_ctx.psfe_state = PSFE_STATE_OFF;
			break;
		}
	}
	return 0;
}
