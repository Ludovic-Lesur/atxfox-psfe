/*
 * psfe.c
 *
 *  Created on: Dec 4, 2021
 *      Author: Ludo
 */

#include "psfe.h"

// Peripherals.
#include "adc.h"
#include "exti.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "mode.h"
#include "nvic.h"
#include "pwr.h"
#include "rcc.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
// Components.
#include "lcd.h"
#include "td1208.h"
#include "trcs.h"
// Utils.
#include "string.h"
// Applicative.
#include "error.h"
#include "version.h"

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

/* COMMON INIT FUNCTION FOR PERIPHERALS AND COMPONENTS.
 * @param:	None.
 * @return:	None.
 */
void PSFE_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	TIM_status_t tim21_status = TIM_SUCCESS;
	RTC_status_t rtc_status = RTC_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	LCD_status_t lcd_status = LCD_SUCCESS;
	unsigned int lsi_frequency_hz = 0;
#ifndef DEBUG
	IWDG_status_t iwdg_status = IWDG_SUCCESS;
#endif
	// Init memory.
	NVIC_init();
	// Init GPIOs.
	GPIO_init();
	EXTI_init();
	// Init clock.
	RCC_init();
	PWR_init();
	// Reset RTC before starting oscillators.
	RTC_reset();
	// Start clocks.
	rcc_status = RCC_switch_to_hsi();
	RCC_error_check();
	rcc_status = RCC_enable_lsi();
	RCC_error_check();
	// Init watchdog.
#ifndef DEBUG
	iwdg_status = IWDG_init();
	IWDG_error_check();
#endif
	IWDG_reload();
	// Get effective LSI frequency.
	TIM21_init();
	tim21_status = TIM21_get_lsi_frequency(&lsi_frequency_hz);
	TIM21_error_check();
	TIM21_disable();
	// Init peripherals.
	rtc_status = RTC_init(lsi_frequency_hz);
	RTC_error_check();
	TIM2_init(PSFE_ADC_CONVERSION_PERIOD_MS);
	LPTIM1_init(lsi_frequency_hz);
	adc1_status = ADC1_init();
	ADC1_error_check();
	USART2_init();
	LPUART1_init();
	// Init components.
	lcd_status = LCD_init();
	LCD_error_check();
	TRCS_init();
	TD1208_init();
}

/* COMMON INIT FUNCTION FOR MAIN CONTEXT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_init_context(void) {
	// Local variables.
	unsigned char idx = 0;
	// Init context.
	psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
	psfe_ctx.psfe_bypass_flag = GPIO_read(&GPIO_TRCS_BYPASS);
	psfe_ctx.psfe_init_done = 0;
	// Analog measurements.
	for (idx=0 ; idx<PSFE_VOUT_BUFFER_LENGTH ; idx++) psfe_ctx.psfe_vout_mv_buf[idx] = 0;
	for (idx=0 ; idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; idx++) psfe_ctx.psfe_sigfox_id[idx] = 0x00;
	for (idx=0 ; idx<PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES ; idx++) psfe_ctx.psfe_sigfox_uplink_data.raw_frame[idx] = (0x65 + idx);
	psfe_ctx.psfe_vout_mv_buf_idx = 0;
	psfe_ctx.psfe_vout_mv = 0;
	psfe_ctx.psfe_iout_ua = 0;
	psfe_ctx.psfe_vmcu_mv = 0;
	psfe_ctx.psfe_tmcu_degrees = 0;
	psfe_ctx.psfe_trcs_range = TRCS_RANGE_NONE;
}

/* START PSFE MODULE.
 * @param:	None.
 * @return:	None.
 */
void PSFE_start(void) {
	// Local variables.
	RTC_status_t rtc_status = RTC_SUCCESS;
	// Start continuous timers.
	TIM2_start();
	rtc_status = RTC_start_wakeup_timer(PSFE_SIGFOX_PERIOD_S);
	RTC_error_check();
}

/* PSFE BOARD MAIN TASK.
 * @param:	None.
 * @return:	None.
 */
void PSFE_task(void) {
	// Local variables.
	LCD_status_t lcd_status = LCD_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	TD1208_status_t td1208_status = TD1208_SUCCESS;
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
		// Init TRCS board to supply output.
		TRCS_init();
		// Print board name and HW version.
		lcd_status = LCD_print_hw_version();
		LCD_error_check();
		lptim1_status = LPTIM1_delay_milliseconds(2000, 0);
		LPTIM1_error_check();
		// Print SW version.
		LCD_print_sw_version(GIT_MAJOR_VERSION, GIT_MINOR_VERSION, GIT_COMMIT_INDEX, GIT_DIRTY_FLAG);
		LCD_error_check();
		lptim1_status = LPTIM1_delay_milliseconds(3000, 0);
		LPTIM1_error_check();
		// Print Sigfox device ID.
		td1208_status = TD1208_disable_echo_and_verbose();
		TD1208_error_check();
		td1208_status = TD1208_get_sigfox_id(psfe_ctx.psfe_sigfox_id);
		TD1208_error_check();
		lcd_status = LCD_print_sigfox_id(psfe_ctx.psfe_sigfox_id);
		LCD_error_check();
		// Send START message through Sigfox.
		td1208_status = TD1208_send_bit(1);
		TD1208_error_check();
		// Delay for Sigfox device ID printing.
		lptim1_status = LPTIM1_delay_milliseconds(2000, 0);
		LPTIM1_error_check();
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
		td1208_status = TD1208_send_frame(psfe_ctx.psfe_sigfox_uplink_data.raw_frame, PSFE_SIGFOX_UPLINK_DATA_LENGTH_BYTES);
		TD1208_error_check();
		// Compute next state.
		psfe_ctx.psfe_state = PSFE_STATE_VMCU_MONITORING;
		break;
	// Power-off.
	case PSFE_STATE_OFF:
		// Send Sigfox message and clear LCD screen.
		if (psfe_ctx.psfe_init_done != 0) {
			// Turn current sensor off.
			TRCS_off();
			// Update flag.
			psfe_ctx.psfe_init_done = 0;
			// Clear LCD.
			lcd_status = LCD_clear();
			LCD_error_check();
			// Send bit 0.
			td1208_status = TD1208_send_bit(0);
			TD1208_error_check();
		}
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
	// Local variables.
	ADC_status_t adc1_status = ADC_SUCCESS;
	TRCS_status_t trcs_status = TRCS_SUCCESS;
	// Perform measurements.
	adc1_status = ADC1_perform_measurements();
	ADC1_error_check();
	trcs_status = TRCS_task();
	TRCS_error_check();
	// Update local Vout.
	adc1_status = ADC1_get_data(ADC_DATA_INDEX_VOUT_MV, (unsigned int*) &psfe_ctx.psfe_vout_mv_buf[psfe_ctx.psfe_vout_mv_buf_idx]);
	ADC1_error_check();
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
	adc1_status = ADC1_get_data(ADC_DATA_INDEX_VMCU_MV, (unsigned int*) &psfe_ctx.psfe_vmcu_mv);
	ADC1_error_check();
	ADC1_get_tmcu((signed char*) &psfe_ctx.psfe_tmcu_degrees);
}

/* CALLBACK CALLED BY TIM22 INTERRUPT.
 * @param:	None.
 * @return:	None.
 */
void PSFE_lcd_uart_callback(void) {
	// Local variables.
	LCD_status_t lcd_status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	char str_value[PSFE_STRING_VALUE_BUFFER_LENGTH];
	// Update display if enabled.
	if (psfe_ctx.psfe_init_done != 0) {
		// LCD Vout display.
		if (psfe_ctx.psfe_vout_mv > PSFE_VOUT_ERROR_THRESHOLD_MV) {
			lcd_status = LCD_print_value_5_digits(0, 0, psfe_ctx.psfe_vout_mv);
			LCD_error_check();
			lcd_status = LCD_print(0, 5, "  V", 3);
			LCD_error_check();
		}
		else {
			lcd_status = LCD_print(0, 0, "NO INPUT", 8);
			LCD_error_check();
		}
		// LCD Iout display.
		if (psfe_ctx.psfe_bypass_flag == 0) {
			lcd_status = LCD_print_value_5_digits(1, 0, psfe_ctx.psfe_iout_ua);
			LCD_error_check();
			lcd_status = LCD_print(1, 5, " mA", 3);
			LCD_error_check();
		}
		else {
			lcd_status = LCD_print(1, 0, " BYPASS ", 8);
			LCD_error_check();
		}
	}
	// UART.
	lpuart1_status = LPUART1_send_string("Vout=");
	LPUART1_error_check();
	string_status = STRING_value_to_string((int) psfe_ctx.psfe_vout_mv, STRING_FORMAT_DECIMAL, 0, str_value);
	STRING_error_check();
	lpuart1_status = LPUART1_send_string(str_value);
	LPUART1_error_check();
	lpuart1_status = LPUART1_send_string("mV Iout=");
	LPUART1_error_check();
	if (psfe_ctx.psfe_bypass_flag == 0) {
		STRING_value_to_string((int) psfe_ctx.psfe_iout_ua, STRING_FORMAT_DECIMAL, 0, str_value);
		STRING_error_check();
		lpuart1_status = LPUART1_send_string(str_value);
		LPUART1_error_check();
		lpuart1_status = LPUART1_send_string("uA Vmcu=");
		LPUART1_error_check();
	}
	else {
		lpuart1_status = LPUART1_send_string("BYPASS Vmcu=");
		LPUART1_error_check();
	}
	string_status = STRING_value_to_string((int) psfe_ctx.psfe_vmcu_mv, STRING_FORMAT_DECIMAL, 0, str_value);
	STRING_error_check();
	lpuart1_status = LPUART1_send_string(str_value);
	LPUART1_error_check();
	lpuart1_status = LPUART1_send_string("mV Tmcu=");
	LPUART1_error_check();
	string_status = STRING_value_to_string((int) psfe_ctx.psfe_tmcu_degrees, STRING_FORMAT_DECIMAL, 0, str_value);
	STRING_error_check();
	lpuart1_status = LPUART1_send_string(str_value);
	LPUART1_error_check();
	lpuart1_status = LPUART1_send_string("dC\n");
	LPUART1_error_check();
}
