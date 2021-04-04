/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#include "adc.h"
#include "atxfox.h"
#include "filter.h"
#include "gpio.h"
#include "lcd.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "nvic.h"
#include "rcc.h"
#include "td1208.h"
#include "tim.h"
#include "trcs.h"
#include "usart.h"

/*** Main macros ***/

// ON/OFF threshold voltages.
#define PSFE_ON_VOLTAGE_THRESHOLD_MV			3300
#define PSFE_OFF_VOLTAGE_THRESHOLD_MV			3200
// Voltage threshold.
#define PSFE_OPEN_VOLTAGE_THRESHOLD_MV			100
// Bandgap calibration.
#define PSFE_BANDGAP_MEDIAN_FILTER_LENGTH		9
#define PSFE_BANDGAP_CENTER_AVERAGE_LENGTH		0
#define PSFE_BANDGAP_CALIBRATION_PERIOD_S		60
// ATX voltage.
#define PSFE_ATX_VOLTAGE_MEDIAN_FILTER_LENGTH	9
#define PSFE_ATX_VOLTAGE_CENTER_AVERAGE_LENGTH	0
// ATX current.
#define PSFE_ATX_CURRENT_ERROR_VALUE			0xFFFFFF
// Log.
#define PSFE_LCD_UART_PERIOD_MS					1000
#define PSFE_SIGFOX_PERIOD_S					600
#define PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES	8

/*** Main structures ***/

// State machine.
typedef enum {
	PSFE_STATE_SUPPLY_VOLTAGE_MONITORING,	// Measure supply voltage to start or stop the front-end.
	PSFE_STATE_OFF,							// Power cut detected: switch all peripherals off + send OFF message).
	PSFE_STATE_INIT,						// Power on detected: display initialization screens+ send ON message).
	PSFE_STATE_BYPASS,						// Read bypass input.
	PSFE_STATE_VOLTAGE_MEASURE,				// Perform voltage measurement.
	PSFE_STATE_CURRENT_MEASURE,				// Perform current measurement.
	PSFE_STATE_TIMER,						// Manage timers flags and seconds counters.
	PSFE_STATE_LCD,							// Update LCD screen.
	PSFE_STATE_UART,						// send measurements through UART.
	PSFE_STATE_SIGFOX,						// Send data through Sigfox.
	PSFE_STATE_BANDGAP_CALIBRATION			// Update bandgap calibration value.
} PSFE_State;

// Sigfox uplink data format.
typedef struct {
	union {
		unsigned char raw_frame[PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES];
		struct {
			unsigned atx_voltage_mv : 14;
			unsigned trcs_range : 2;
			unsigned atx_current_ua : 24;
			unsigned mcu_voltage_mv : 16;
			unsigned mcu_temperature_degrees : 8;
		} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed)) field;
	};
} PSFE_SigfoxUplinkData;

// Context.
typedef struct {
	// State machine.
	PSFE_State psfe_state;
	unsigned char psfe_init_done;
	unsigned char psfe_no_input_previous_status;
	unsigned char psfe_no_input_current_status;
	unsigned char psfe_bypass_previous_status;
	unsigned char psfe_bypass_current_status;
	// Analog measurements.
	unsigned int psfe_supply_voltage_mv;
	unsigned int psfe_atx_voltage_mv;
	unsigned int psfe_atx_current_ua;
	TRCS_Range psfe_trcs_range;
	unsigned int psfe_bandgap_result_12bits;
	unsigned int psfe_bandgap_calibration_seconds_count;
	unsigned char psfe_mcu_temperature_degrees;
	// LCD and UART period.
	unsigned char psfe_log_request;
	// Sigfox.
	unsigned int psfe_sigfox_log_seconds_count;
	unsigned char psfe_sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES];
	PSFE_SigfoxUplinkData psfe_sigfox_uplink_data;
} PSFE_Context;

/*** Main global variables ***/

static PSFE_Context psfe_ctx;

/*** MAIN local functions ***/

/* COMPUTE ATX VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
void PSFE_UpdateAtxVoltage(void) {
	// Local variables.
	unsigned int adc_sample_buf[PSFE_ATX_VOLTAGE_MEDIAN_FILTER_LENGTH];
	unsigned char idx = 0;
	unsigned int adc_median = 0;
	// Get samples.
	for (idx=0 ; idx<PSFE_ATX_VOLTAGE_MEDIAN_FILTER_LENGTH ; idx++) {
		ADC1_SingleConversion(ADC_ATX_VOLTAGE_CHANNEL, &(adc_sample_buf[idx]));
	}
	// Apply filter.
	adc_median = FILTER_ComputeMedianFilter(adc_sample_buf, PSFE_ATX_VOLTAGE_MEDIAN_FILTER_LENGTH, PSFE_ATX_VOLTAGE_CENTER_AVERAGE_LENGTH);
	// Convert to mV.
	psfe_ctx.psfe_atx_voltage_mv = psfe_vout_voltage_divider_ratio[PSFE_BOARD_INDEX] * ((adc_median * ADC_BANDGAP_VOLTAGE_MV) / (psfe_ctx.psfe_bandgap_result_12bits));
}

/* COMPUTE BANDGAP VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
void PSFE_UpdateBandgapResult(void) {
	// Local variables.
	unsigned int adc_sample_buf[PSFE_BANDGAP_MEDIAN_FILTER_LENGTH];
	unsigned char idx = 0;
	// Get samples.
	for (idx=0 ; idx<PSFE_BANDGAP_MEDIAN_FILTER_LENGTH ; idx++) {
		ADC1_SingleConversion(ADC_BANDGAP_CHANNEL, &(adc_sample_buf[idx]));
	}
	// Apply filter.
	psfe_ctx.psfe_bandgap_result_12bits = FILTER_ComputeMedianFilter(adc_sample_buf, PSFE_BANDGAP_MEDIAN_FILTER_LENGTH, PSFE_BANDGAP_CENTER_AVERAGE_LENGTH);
}

/*** MAIN function ***/

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {
	// Init clock.
	RCC_Init();
	// Init peripherals.
	TIM21_Init(PSFE_LCD_UART_PERIOD_MS);
	TIM22_Init();
	LPTIM1_Init();
	GPIO_Init();
	ADC1_Init();
	USART2_Init();
	LPUART1_Init();
	// Init components.
	LCD_Init();
	LCD_Clear();
	TRCS_Init();
	TD1208_Init();
	NVIC_EnableInterrupt(IT_USART2);
	// Init context and global variables.
	unsigned int adc_channel_result_12bits = 0;
	unsigned int vout_voltage_divider_current_ua = 0;
	// State machine.
	psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
	psfe_ctx.psfe_no_input_current_status = 0;
	psfe_ctx.psfe_no_input_previous_status = 0;
	psfe_ctx.psfe_bypass_current_status = 0;
	psfe_ctx.psfe_bypass_previous_status = 0;
	psfe_ctx.psfe_init_done = 0;
	// Analog measurements.
	psfe_ctx.psfe_supply_voltage_mv = 0;
	psfe_ctx.psfe_atx_voltage_mv = 0;
	psfe_ctx.psfe_atx_current_ua = 0;
	psfe_ctx.psfe_trcs_range = TRCS_RANGE_NONE;
	psfe_ctx.psfe_bandgap_result_12bits = 0;
	psfe_ctx.psfe_bandgap_calibration_seconds_count = 0;
	psfe_ctx.psfe_mcu_temperature_degrees = 0;
	// LCD and UART period.
	psfe_ctx.psfe_log_request = 0;
	// Sigfox.
	psfe_ctx.psfe_sigfox_log_seconds_count = 0;
	// Main loop.
	while (1) {
		// Perform state machine.
		switch (psfe_ctx.psfe_state) {
		// Supply voltage monitoring.
		case PSFE_STATE_SUPPLY_VOLTAGE_MONITORING:
			ADC1_GetMcuVoltage(&psfe_ctx.psfe_supply_voltage_mv);
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
		// Power-off.
		case PSFE_STATE_OFF:
			// Send Sigfox message and display OFF on LCD screen.
			if (psfe_ctx.psfe_init_done != 0) {
				TRCS_Off();
				LCD_Clear();
				TD1208_SendBit(0);
			}
			// Update flag.
			psfe_ctx.psfe_init_done = 0;
			// Compute next state
			psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
			break;
		// Init.
		case PSFE_STATE_INIT:
			// Print project name and HW versions.
			LCD_Print(0, 0, " ATXFox ", 8);
			LCD_Print(1, 0, " HW 1.0 ", 8);
			LPTIM1_DelayMilliseconds(2000);
			// Print Sigfox device ID.
			TD1208_DisableEcho();
			TD1208_GetSigfoxId(psfe_ctx.psfe_sigfox_id);
			LCD_Print(0, 0, " SFX ID ", 8);
			LCD_PrintSigfoxId(1, psfe_ctx.psfe_sigfox_id);
			// Send START message through Sigfox.
			TD1208_SendBit(1);
			LPTIM1_DelayMilliseconds(2000);
			// Print units.
			LCD_Print(0, 0, "       V", 8);
			LCD_Print(1, 0, "      mA", 8);
			// Update flags.
			psfe_ctx.psfe_init_done = 1;
			psfe_ctx.psfe_no_input_current_status = 0;
			psfe_ctx.psfe_no_input_previous_status = 0;
			psfe_ctx.psfe_bypass_current_status = 0;
			psfe_ctx.psfe_bypass_previous_status = 0;
			// Get bandgap result.
			PSFE_UpdateBandgapResult();
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_BYPASS;
			break;
		// Get bypass switch status.
		case PSFE_STATE_BYPASS:
			psfe_ctx.psfe_bypass_current_status = GPIO_Read(&GPIO_BYPASS);
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_VOLTAGE_MEASURE;
			break;
		// Analog measurements.
		case PSFE_STATE_VOLTAGE_MEASURE:
			// Compute effective ATX output voltage.
			PSFE_UpdateAtxVoltage();
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
		// Update current range according to previous measure.
		case PSFE_STATE_CURRENT_MEASURE:
			// Use TRCS board.
			TRCS_Task(psfe_ctx.psfe_bandgap_result_12bits, &psfe_ctx.psfe_atx_current_ua, psfe_ctx.psfe_bypass_current_status, &psfe_ctx.psfe_trcs_range);
			// Compute ouput voltage divider current.
			vout_voltage_divider_current_ua = (psfe_ctx.psfe_atx_voltage_mv * 1000) / (psfe_vout_voltage_divider_resistance[PSFE_BOARD_INDEX]);
			// Remove offset current.
			if (psfe_ctx.psfe_atx_current_ua > vout_voltage_divider_current_ua) {
				psfe_ctx.psfe_atx_current_ua -= vout_voltage_divider_current_ua;
			}
			else {
				vout_voltage_divider_current_ua = 0;
			}
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_TIMER;
			break;
		case PSFE_STATE_TIMER:
			// Check timers flag.
			if (TIM21_GetFlag() != 0) {
				// Clear flag and update LCD.
				TIM21_ClearFlag();
				psfe_ctx.psfe_log_request = 1;
			}
			if (TIM22_GetFlag() != 0) {
				// Clear flag and increment counters.
				TIM22_ClearFlag();
				psfe_ctx.psfe_bandgap_calibration_seconds_count++;
				psfe_ctx.psfe_sigfox_log_seconds_count++;
			}
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING; // Default.
			if (psfe_ctx.psfe_log_request != 0) {
				psfe_ctx.psfe_state = PSFE_STATE_LCD;
			}
			else {
				if (psfe_ctx.psfe_sigfox_log_seconds_count >= PSFE_SIGFOX_PERIOD_S) {
					psfe_ctx.psfe_state = PSFE_STATE_SIGFOX;
				}
				else {
					if (psfe_ctx.psfe_bandgap_calibration_seconds_count >= PSFE_BANDGAP_CALIBRATION_PERIOD_S) {
						psfe_ctx.psfe_state = PSFE_STATE_BANDGAP_CALIBRATION;
					}
				}
			}
			break;
		// Print voltage and current on LCD screen.
		case PSFE_STATE_LCD:
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
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_UART;
			break;
		// Send data over UART interface.
		case PSFE_STATE_UART:
			// Get MCU temperature.
			ADC1_GetMcuTemperatureComp1(&psfe_ctx.psfe_mcu_temperature_degrees);
			// Log values on USB connector.
			LPUART1_SendString("U=");
			LPUART1_SendValue(psfe_ctx.psfe_atx_voltage_mv, LPUART_FORMAT_DECIMAL, 0);
			LPUART1_SendString("mV*I=");
			if (psfe_ctx.psfe_bypass_current_status == 0) {
				LPUART1_SendValue(psfe_ctx.psfe_atx_current_ua, LPUART_FORMAT_DECIMAL, 0);
				LPUART1_SendString("uA*T=");
			}
			else {
				LPUART1_SendString("BYPASS*T=");
			}
			LPUART1_SendValue(psfe_ctx.psfe_mcu_temperature_degrees, LPUART_FORMAT_DECIMAL, 0);
			LPUART1_SendString("dC\n");
			// Reset flag.
			psfe_ctx.psfe_log_request = 0;
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING; // Default.
			if (psfe_ctx.psfe_sigfox_log_seconds_count >= PSFE_SIGFOX_PERIOD_S) {
				psfe_ctx.psfe_state = PSFE_STATE_SIGFOX;
			}
			else {
				if (psfe_ctx.psfe_bandgap_calibration_seconds_count >= PSFE_BANDGAP_CALIBRATION_PERIOD_S) {
					psfe_ctx.psfe_state = PSFE_STATE_BANDGAP_CALIBRATION;
				}
			}
			break;
		// Send data through Sigfox.
		case PSFE_STATE_SIGFOX:
			// Get MCU supply voltage and temperature.
			ADC1_GetMcuTemperatureComp1(&psfe_ctx.psfe_mcu_temperature_degrees);
			// Build data.
			psfe_ctx.psfe_sigfox_uplink_data.field.atx_voltage_mv = psfe_ctx.psfe_atx_voltage_mv;
			if (psfe_ctx.psfe_bypass_current_status != 0) {
				psfe_ctx.psfe_sigfox_uplink_data.field.trcs_range = TRCS_RANGE_NONE;
				psfe_ctx.psfe_sigfox_uplink_data.field.atx_current_ua = PSFE_ATX_CURRENT_ERROR_VALUE;
			}
			else {
				psfe_ctx.psfe_sigfox_uplink_data.field.trcs_range = psfe_ctx.psfe_trcs_range;
				psfe_ctx.psfe_sigfox_uplink_data.field.atx_current_ua = psfe_ctx.psfe_atx_current_ua;
			}
			psfe_ctx.psfe_sigfox_uplink_data.field.mcu_voltage_mv = psfe_ctx.psfe_supply_voltage_mv;
			psfe_ctx.psfe_sigfox_uplink_data.field.mcu_temperature_degrees = psfe_ctx.psfe_mcu_temperature_degrees;
			// Send data.
			TD1208_SendFrame(psfe_ctx.psfe_sigfox_uplink_data.raw_frame, PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES);
			// Reset counter.
			psfe_ctx.psfe_sigfox_log_seconds_count = 0;
			// Compute next state.
			if (psfe_ctx.psfe_bandgap_calibration_seconds_count >= PSFE_BANDGAP_CALIBRATION_PERIOD_S) {
				psfe_ctx.psfe_state = PSFE_STATE_BANDGAP_CALIBRATION;
			}
			else {
				psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
			}
			break;
		case PSFE_STATE_BANDGAP_CALIBRATION:
			// Update bandgap result.
			PSFE_UpdateBandgapResult();
			// Reset counter.
			psfe_ctx.psfe_bandgap_calibration_seconds_count = 0;
			// Compute next state.
			psfe_ctx.psfe_state = PSFE_STATE_SUPPLY_VOLTAGE_MONITORING;
			break;
		// Unknown state.
		default:
			psfe_ctx.psfe_state = PSFE_STATE_OFF;
			break;
		}
	}
	return 0;
}
