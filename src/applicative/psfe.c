/*
 * psfe.c
 *
 *  Created on: Dec 4, 2021
 *      Author: Ludo
 */

#include "psfe.h"

#include "adc.h"
#include "gpio.h"
#include "lcd.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "mode.h"
#include "td1208.h"
#include "tim.h"
#include "trcs.h"

/*** PSFE local macros ***/

#if (PSFE_BOARD_NUMBER == 0) || (PSFE_BOARD_NUMBER > PSFE_NUMBER_OF_BOARDS)
#error "PSFE board number error."
#endif

#define PSFE_ON_VOLTAGE_THRESHOLD_MV			3300
#define PSFE_OFF_VOLTAGE_THRESHOLD_MV			3200

#define PSFE_VOUT_ERROR_THRESHOLD_MV			100
#define PSFE_VOUT_RESISTOR_DIVIDER_COMPENSATION

#define PSFE_IOUT_ERROR_VALUE					0xFFFFFF

#define PSFE_SIGFOX_PERIOD_S					600
#define PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES	8

/*** PSFE local structures ***/

typedef enum {
	PSFE_STATE_VMCU_MONITORING,
	PSFE_STATE_INIT,
	PSFE_STATE_VOUT_MONITORING,
	PSFE_STATE_PERIOD_CHECK,
	PSFE_STATE_SIGFOX,
	PSFE_STATE_OFF,
} PSFE_State;

typedef struct {
	union {
		unsigned char raw_frame[PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES];
		struct {
			unsigned vout_mv : 14;
			unsigned trcs_range : 2;
			unsigned iout_ua : 24;
			unsigned vmcu_mv : 16;
			unsigned tmcu_degrees : 8;
		} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed)) field;
	};
} PSFE_SigfoxUplinkData;

typedef struct {
	// State machine.
	unsigned int lsi_frequency_hz;
	PSFE_State psfe_state;
	unsigned char psfe_init_done;
	volatile unsigned char psfe_no_input_previous_status;
	unsigned char psfe_no_input_current_status;
	volatile unsigned char psfe_bypass_previous_status;
	unsigned char psfe_bypass_current_status;
	// Analog measurements.
	volatile unsigned int psfe_vout_mv;
	volatile unsigned int psfe_iout_ua;
	volatile unsigned int psfe_vmcu_mv;
	volatile unsigned char psfe_tmcu_degrees;
	volatile TRCS_Range psfe_trcs_range;
	unsigned int psfe_adc_calibration_seconds_count;
	// Sigfox.
	unsigned int psfe_sigfox_log_seconds_count;
	unsigned char psfe_sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES];
	PSFE_SigfoxUplinkData psfe_sigfox_uplink_data;
} PSFE_Context;

/*** PSFE local global variables ***/

static PSFE_Context psfe_ctx;
#ifdef PSFE_VOUT_RESISTOR_DIVIDER_COMPENSATION
static const unsigned int psfe_vout_voltage_divider_resistance[PSFE_NUMBER_OF_BOARDS] = {998000, 998000, 599000, 599000, 998000, 998000, 998000, 599000, 599000, 998000};
#endif

/*** PSFE functions ***/

/* INIT PSFE CONTEXT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_Init(void) {
	// Init context.
	psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
	psfe_ctx.psfe_no_input_current_status = 0;
	psfe_ctx.psfe_no_input_previous_status = 0;
	psfe_ctx.psfe_bypass_current_status = 0;
	psfe_ctx.psfe_bypass_previous_status = 0;
	psfe_ctx.psfe_init_done = 0;
	// Analog measurements.
	psfe_ctx.psfe_vout_mv = 0;
	psfe_ctx.psfe_iout_ua = 0;
	psfe_ctx.psfe_vmcu_mv = 0;
	psfe_ctx.psfe_tmcu_degrees = 0;
	psfe_ctx.psfe_trcs_range = TRCS_RANGE_NONE;
	psfe_ctx.psfe_adc_calibration_seconds_count = 0;
}

/* PSFE BOARD MAIN TASK.
 * @param:	None.
 * @return:	None.
 */
void PSFE_Task(void) {
	// Local variables.
#ifdef PSFE_VOUT_RESISTOR_DIVIDER_COMPENSATION
	unsigned int vout_voltage_divider_current_ua = 0;
#endif
	// Perform state machine.
	switch (psfe_ctx.psfe_state) {
	// Supply voltage monitoring.
	case PSFE_STATE_VMCU_MONITORING:
		// Power-off detection.
		if (psfe_ctx.psfe_vmcu_mv < PSFE_OFF_VOLTAGE_THRESHOLD_MV) {
			psfe_ctx.psfe_state = PSFE_STATE_OFF;
		}
		// Power-on detection.
		if (psfe_ctx.psfe_vmcu_mv > PSFE_ON_VOLTAGE_THRESHOLD_MV) {
			if (psfe_ctx.psfe_init_done == 0) {
				psfe_ctx.psfe_state = PSFE_STATE_INIT;
			}
			else {
				psfe_ctx.psfe_state = PSFE_STATE_VOUT_MONITORING;
			}
		}
		break;
	// Init.
	case PSFE_STATE_INIT:
		// Stop LCD/UART timer.
		TIM22_Stop();
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
		// Restart LCD/UART timer.
		TIM22_Start();
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_VOUT_MONITORING;
		break;
	// Output voltage monitoring.
	case PSFE_STATE_VOUT_MONITORING:
		// Detect open state.
		if (psfe_ctx.psfe_vout_mv < PSFE_VOUT_ERROR_THRESHOLD_MV) {
			psfe_ctx.psfe_no_input_current_status = 1;
		}
		else {
			psfe_ctx.psfe_no_input_current_status = 0;
		}
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_PERIOD_CHECK;
		break;
	case PSFE_STATE_PERIOD_CHECK:
		// Check Sigfox and ADC conversions period with RTC wake-up timer.
		psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
		break;
	// Send data through Sigfox.
	case PSFE_STATE_SIGFOX:
		// Build data.
		psfe_ctx.psfe_sigfox_uplink_data.field.vout_mv = psfe_ctx.psfe_vout_mv;
		if (psfe_ctx.psfe_bypass_current_status != 0) {
			psfe_ctx.psfe_sigfox_uplink_data.field.trcs_range = TRCS_RANGE_NONE;
			psfe_ctx.psfe_sigfox_uplink_data.field.iout_ua = PSFE_IOUT_ERROR_VALUE;
		}
		else {
			psfe_ctx.psfe_sigfox_uplink_data.field.trcs_range = psfe_ctx.psfe_trcs_range;
			psfe_ctx.psfe_sigfox_uplink_data.field.iout_ua = psfe_ctx.psfe_iout_ua;
		}
		psfe_ctx.psfe_sigfox_uplink_data.field.vmcu_mv = psfe_ctx.psfe_vmcu_mv;
		psfe_ctx.psfe_sigfox_uplink_data.field.tmcu_degrees = psfe_ctx.psfe_tmcu_degrees;
		// Send data.
		TD1208_SendFrame(psfe_ctx.psfe_sigfox_uplink_data.raw_frame, PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES);
		// Reset counter.
		psfe_ctx.psfe_sigfox_log_seconds_count = 0;
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
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
		psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
		break;
	// Unknown state.
	default:
		psfe_ctx.psfe_state = PSFE_STATE_OFF;
		break;
	}
}

/* SET TRCS BYPASS STATUS (CALLED BY EXTI INTERRUPT).
 * @param bypass_state:	Bypass switch state.
 * @return:				None.
 */
void PSFE_SetBypassFlag(unsigned char bypass_state) {
	psfe_ctx.psfe_bypass_current_status = bypass_state;
}

/* CALLBACK CALLED BY TIM2 INTERRUPT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_AdcCallback(void) {
	// Perform measurements.
	ADC1_PerformMeasurements();
	TRCS_Task();
	// Update local data.
	ADC1_GetData(ADC_DATA_IDX_VOUT_MV, &psfe_ctx.psfe_vout_mv);
	TRCS_GetIout(&psfe_ctx.psfe_iout_ua);
	TRCS_GetRange((TRCS_Range*) &psfe_ctx.psfe_trcs_range);
#ifdef PSFE_VOUT_RESISTOR_DIVIDER_COMPENSATION
	// Compute ouput voltage divider current.
	unsigned int vout_voltage_divider_current_ua = (psfe_ctx.psfe_vout_mv * 1000) / (psfe_vout_voltage_divider_resistance[PSFE_BOARD_INDEX]);
	// Remove offset current.
	if (psfe_ctx.psfe_iout_ua > vout_voltage_divider_current_ua) {
		psfe_ctx.psfe_iout_ua -= vout_voltage_divider_current_ua;
	}
	else {
		psfe_ctx.psfe_iout_ua = 0;
	}
#endif
	ADC1_GetData(ADC_DATA_IDX_VMCU_MV, &psfe_ctx.psfe_vmcu_mv);
	ADC1_GetTmcuComp1(&psfe_ctx.psfe_tmcu_degrees);
}

/* CALLBACK CALLED BY TIM22 INTERRUPT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_LcdUartCallback(void) {
	// LCD Vout display.
	if (psfe_ctx.psfe_vout_mv > PSFE_VOUT_ERROR_THRESHOLD_MV) {
		if (psfe_ctx.psfe_no_input_previous_status != 0) {
			LCD_Print(0, 0, "       V", 8);
			psfe_ctx.psfe_no_input_previous_status = 0;
		}
		LCD_PrintValue5Digits(0, 0, psfe_ctx.psfe_vout_mv);
	}
	else {
		if (psfe_ctx.psfe_no_input_previous_status == 0) {
			LCD_Print(0, 0, "NO INPUT", 8);
			psfe_ctx.psfe_no_input_previous_status = 1;
		}
	}
	// LCD Iout display.
	if (psfe_ctx.psfe_bypass_current_status == 0) {
		if (psfe_ctx.psfe_bypass_previous_status != 0) {
			LCD_Print(1, 0, "      mA", 8);
			psfe_ctx.psfe_bypass_previous_status = 0;
		}
		LCD_PrintValue5Digits(1, 0, psfe_ctx.psfe_iout_ua);
	}
	else {
		if (psfe_ctx.psfe_bypass_previous_status == 0) {
			LCD_Print(1, 0, " BYPASS ", 8);
			psfe_ctx.psfe_bypass_previous_status = 1;
		}
	}
	// UART.
	LPUART1_SendString("U=");
	LPUART1_SendValue(psfe_ctx.psfe_vout_mv, LPUART_FORMAT_DECIMAL, 0);
	LPUART1_SendString("mV*I=");
	if (psfe_ctx.psfe_bypass_current_status == 0) {
		LPUART1_SendValue(psfe_ctx.psfe_iout_ua, LPUART_FORMAT_DECIMAL, 0);
		LPUART1_SendString("uA*T=");
	}
	else {
		LPUART1_SendString("BYPASS*T=");
	}
	LPUART1_SendValue(psfe_ctx.psfe_tmcu_degrees, LPUART_FORMAT_DECIMAL, 0);
	LPUART1_SendString("dC\n");
}
