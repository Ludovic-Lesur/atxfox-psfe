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
#include "math.h"
#include "mode.h"
#include "rtc.h"
#include "string.h"
#include "td1208.h"
#include "tim.h"
#include "trcs.h"

/*** PSFE local macros ***/

#if (PSFE_BOARD_NUMBER == 0) || (PSFE_BOARD_NUMBER > PSFE_NUMBER_OF_BOARDS)
#error "PSFE board number error."
#endif

#define PSFE_ON_VOLTAGE_THRESHOLD_MV			3300
#define PSFE_OFF_VOLTAGE_THRESHOLD_MV			3200

#define PSFE_VOUT_BUFFER_LENGTH					10
#define PSFE_VOUT_ERROR_THRESHOLD_MV			100
#define PSFE_VOUT_RESISTOR_DIVIDER_COMPENSATION

#define PSFE_SIGFOX_PERIOD_S					300
#define PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES	8

#define PSFE_STRING_VALUE_BUFFER_LENGTH			16

/*** PSFE local structures ***/

typedef enum {
	PSFE_STATE_VMCU_MONITORING,
	PSFE_STATE_INIT,
	PSFE_STATE_BYPASS_READ,
	PSFE_STATE_PERIOD_CHECK,
	PSFE_STATE_SIGFOX,
	PSFE_STATE_OFF,
} PSFE_state_t;

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
} PSFE_sigfox_monitoring_data;

typedef struct {
	// State machine.
	unsigned int lsi_frequency_hz;
	PSFE_state_t psfe_state;
	unsigned char psfe_init_done;
	unsigned char psfe_bypass_flag;
	// Analog measurements.
	volatile unsigned int psfe_vout_mv_buf[PSFE_VOUT_BUFFER_LENGTH];
	volatile unsigned int psfe_vout_mv_buf_idx;
	volatile unsigned int psfe_vout_mv;
	volatile unsigned int psfe_iout_ua;
	volatile unsigned int psfe_vmcu_mv;
	volatile signed char psfe_tmcu_degrees;
	volatile TRCS_range_t psfe_trcs_range;
	// Sigfox.
	unsigned char psfe_sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES];
	PSFE_sigfox_monitoring_data psfe_sigfox_uplink_data;
} PSFE_context_t;

/*** PSFE local global variables ***/

static PSFE_context_t psfe_ctx;
#ifdef PSFE_VOUT_RESISTOR_DIVIDER_COMPENSATION
static const unsigned int psfe_vout_voltage_divider_resistance[PSFE_NUMBER_OF_BOARDS] = {998000, 998000, 599000, 599000, 998000, 998000, 998000, 599000, 599000, 998000};
#endif

/*** PSFE functions ***/

/* INIT PSFE CONTEXT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_init(void) {
	// Init context.
	psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
	psfe_ctx.psfe_bypass_flag = GPIO_read(&GPIO_TRCS_BYPASS);
	psfe_ctx.psfe_init_done = 0;
	// Analog measurements.
	unsigned char idx = 0;
	for (idx=0 ; idx<PSFE_VOUT_BUFFER_LENGTH ; idx++) psfe_ctx.psfe_vout_mv_buf[idx] = 0;
	for (idx=0 ; idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; idx++) psfe_ctx.psfe_sigfox_id[idx] = 0x00;
	for (idx=0 ; idx<PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES ; idx++) psfe_ctx.psfe_sigfox_uplink_data.raw_frame[idx] = (0x65 + idx);
	psfe_ctx.psfe_vout_mv_buf_idx = 0;
	psfe_ctx.psfe_vout_mv = 0;
	psfe_ctx.psfe_iout_ua = 0;
	psfe_ctx.psfe_vmcu_mv = 0;
	psfe_ctx.psfe_tmcu_degrees = 0;
	psfe_ctx.psfe_trcs_range = TRCS_RANGE_NONE;
	// Start continuous timers.
	TIM2_start();
	RTC_start_wakeup_timer(PSFE_SIGFOX_PERIOD_S);
}

/* PSFE BOARD MAIN TASK.
 * @param:	None.
 * @return:	None.
 */
void PSFE_task(void) {
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
				psfe_ctx.psfe_state = PSFE_STATE_BYPASS_READ;
			}
		}
		break;
	// Init.
	case PSFE_STATE_INIT:
		// Print project name and HW versions.
		LCD_print(0, 0, " ATXFox ", 8);
		LCD_print(1, 0, " HW 1.0 ", 8);
		LPTIM1_delay_milliseconds(2000, 0);
		// Print Sigfox device ID.
		TD1208_disable_echo_and_verbose();
		TD1208_get_sigfox_id(psfe_ctx.psfe_sigfox_id);
		LCD_print(0, 0, " SFX ID ", 8);
		LCD_print_sigfox_id(1, psfe_ctx.psfe_sigfox_id);
		// Send START message through Sigfox.
		TD1208_send_bit(1);
		// Delay for Sigfox device ID printing.
		LPTIM1_delay_milliseconds(2000, 0);
		// Update flags.
		psfe_ctx.psfe_init_done = 1;
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_BYPASS_READ;
		break;
	case PSFE_STATE_BYPASS_READ:
		// Static GPIO reading in addition to EXTI interrupt.
		psfe_ctx.psfe_bypass_flag = GPIO_read(&GPIO_TRCS_BYPASS);
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_PERIOD_CHECK;
		break;
	case PSFE_STATE_PERIOD_CHECK:
		// Check Sigfox period with RTC wake-up timer.
		if (RTC_get_wakeup_timer_flag() != 0) {
			// Clear flag.
			RTC_clear_wakeup_timer_flag();
			psfe_ctx.psfe_state = PSFE_STATE_SIGFOX;
		}
		else {
			psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
		}
		break;
	// Send data through Sigfox.
	case PSFE_STATE_SIGFOX:
		// Build data.
		psfe_ctx.psfe_sigfox_uplink_data.field.vout_mv = psfe_ctx.psfe_vout_mv;
		if (psfe_ctx.psfe_bypass_flag != 0) {
			psfe_ctx.psfe_sigfox_uplink_data.field.trcs_range = TRCS_RANGE_NONE;
			psfe_ctx.psfe_sigfox_uplink_data.field.iout_ua = (TRCS_IOUT_ERROR_VALUE & 0x00FFFFFF);
		}
		else {
			psfe_ctx.psfe_sigfox_uplink_data.field.trcs_range = psfe_ctx.psfe_trcs_range;
			psfe_ctx.psfe_sigfox_uplink_data.field.iout_ua = psfe_ctx.psfe_iout_ua;
		}
		psfe_ctx.psfe_sigfox_uplink_data.field.vmcu_mv = psfe_ctx.psfe_vmcu_mv;
		psfe_ctx.psfe_sigfox_uplink_data.field.tmcu_degrees = psfe_ctx.psfe_tmcu_degrees;
		// Send data.
		TD1208_send_frame(psfe_ctx.psfe_sigfox_uplink_data.raw_frame, PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES);
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
		break;
	// Power-off.
	case PSFE_STATE_OFF:
		// Send Sigfox message and clear LCD screen.
		if (psfe_ctx.psfe_init_done != 0) {
			TRCS_off();
			LCD_clear();
			TD1208_send_bit(0);
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
void PSFE_set_bypass_flag(unsigned char bypass_state) {
	psfe_ctx.psfe_bypass_flag = bypass_state;
}

/* CALLBACK CALLED BY TIM2 INTERRUPT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_adc_callback(void) {
	// Perform measurements.
	ADC1_perform_measurements();
	TRCS_task();
	// Update local Vout.
	ADC1_get_data(ADC_DATA_INDEX_VOUT_MV, (unsigned int*) &psfe_ctx.psfe_vout_mv_buf[psfe_ctx.psfe_vout_mv_buf_idx]);
	psfe_ctx.psfe_vout_mv = MATH_average_u32((unsigned int*) psfe_ctx.psfe_vout_mv_buf, PSFE_VOUT_BUFFER_LENGTH);
	psfe_ctx.psfe_vout_mv_buf_idx++;
	if (psfe_ctx.psfe_vout_mv_buf_idx >= PSFE_VOUT_BUFFER_LENGTH) {
		psfe_ctx.psfe_vout_mv_buf_idx = 0;
	}
	// Update local Iout.
	TRCS_get_iout(&psfe_ctx.psfe_iout_ua);
	TRCS_get_range((TRCS_range_t*) &psfe_ctx.psfe_trcs_range);
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
	// Update local Vmcu and Tmcu.
	ADC1_get_data(ADC_DATA_INDEX_VMCU_MV, (unsigned int*) &psfe_ctx.psfe_vmcu_mv);
	ADC1_get_tmcu((signed char*) &psfe_ctx.psfe_tmcu_degrees);
}

/* CALLBACK CALLED BY TIM22 INTERRUPT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_lcd_uart_callback(void) {
	// Local variables.
	char str_value[PSFE_STRING_VALUE_BUFFER_LENGTH];
	// Update display is enabled.
	if (psfe_ctx.psfe_init_done != 0) {
		// LCD Vout display.
		if (psfe_ctx.psfe_vout_mv > PSFE_VOUT_ERROR_THRESHOLD_MV) {
			LCD_print_value_5_digits(0, 0, psfe_ctx.psfe_vout_mv);
			LCD_print(0, 5, "  V", 3);
		}
		else {
			LCD_print(0, 0, "NO INPUT", 8);
		}
		// LCD Iout display.
		if (psfe_ctx.psfe_bypass_flag == 0) {
			LCD_print_value_5_digits(1, 0, psfe_ctx.psfe_iout_ua);
			LCD_print(1, 5, " mA", 3);
		}
		else {
			LCD_print(1, 0, " BYPASS ", 8);
		}
	}
	// UART.
	LPUART1_send_string("Vout=");
	STRING_value_to_string((int) psfe_ctx.psfe_vout_mv, STRING_FORMAT_DECIMAL, 0, str_value);
	LPUART1_send_string(str_value);
	LPUART1_send_string("mV Iout=");
	if (psfe_ctx.psfe_bypass_flag == 0) {
		STRING_value_to_string((int) psfe_ctx.psfe_iout_ua, STRING_FORMAT_DECIMAL, 0, str_value);
		LPUART1_send_string(str_value);
		LPUART1_send_string("uA Vmcu=");
	}
	else {
		LPUART1_send_string("BYPASS Vmcu=");
	}
	STRING_value_to_string((int) psfe_ctx.psfe_vmcu_mv, STRING_FORMAT_DECIMAL, 0, str_value);
	LPUART1_send_string(str_value);
	LPUART1_send_string("mV Tmcu=");
	STRING_value_to_string((int) psfe_ctx.psfe_tmcu_degrees, STRING_FORMAT_DECIMAL, 0, str_value);
	LPUART1_send_string(str_value);
	LPUART1_send_string("dC\n");
}
