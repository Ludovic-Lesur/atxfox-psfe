/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

// Registers.
#include "rcc_reg.h"
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
#include "psfe.h"
#include "version.h"

/*** MAIN local macros ***/

#define PSFE_TIM2_PERIOD_MS						100
#define PSFE_ADC_SAMPLING_PERIOD_MS				100
#define PSFE_LCD_UART_PRINT_PERIOD_MS			300

#define PSFE_ON_VOLTAGE_THRESHOLD_MV			3300
#define PSFE_OFF_VOLTAGE_THRESHOLD_MV			3200

#define PSFE_VOUT_BUFFER_LENGTH					10
#define PSFE_VOUT_ERROR_THRESHOLD_MV			100

#define PSFE_SIGFOX_PERIOD_SECONDS				300
#define PSFE_SIGFOX_STARTUP_DATA_LENGTH			8
#define PSFE_SIGFOX_MONITORING_DATA_LENGTH		9
#define PSFE_SIGFOX_ERROR_STACK_DATA_LENGTH		12

#define PSFE_STRING_VALUE_BUFFER_LENGTH			16

#define PSFE_ERROR_VALUE_VOLTAGE_16BITS			0xFFFF
#define PSFE_ERROR_VALUE_VOLTAGE_24BITS			0xFFFFFF
#define PSFE_ERROR_VALUE_TEMPERATURE			0x7F

/*** MAIN local structures ***/

/*******************************************************************/
typedef enum {
	PSFE_STATE_VMCU_MONITORING,
	PSFE_STATE_POR,
#ifdef PSFE_SIGFOX_MONITORING
	PSFE_STATE_PERIOD_CHECK,
	PSFE_STATE_SIGFOX,
#endif
	PSFE_STATE_OFF,
} PSFE_state_t;

/*******************************************************************/
typedef union {
	uint8_t frame[PSFE_SIGFOX_STARTUP_DATA_LENGTH];
	struct {
		unsigned reset_reason : 8;
		unsigned major_version : 8;
		unsigned minor_version : 8;
		unsigned commit_index : 8;
		unsigned commit_id : 28;
		unsigned dirty_flag : 4;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} PSFE_sigfox_startup_data_t;

/*******************************************************************/
typedef struct {
	union {
		uint8_t frame[PSFE_SIGFOX_MONITORING_DATA_LENGTH];
		struct {
			unsigned vout_mv : 16;
			unsigned trcs_range : 8;
			unsigned iout_ua : 24;
			unsigned vmcu_mv : 16;
			unsigned tmcu_degrees : 8;
		} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
	};
} PSFE_sigfox_monitoring_data_t;

/*******************************************************************/
typedef struct {
	// State machine.
	uint32_t lsi_frequency_hz;
	PSFE_state_t state;
	uint8_t por_flag;
	uint8_t startup_frame_sent;
	// Periods management.
	uint32_t adc_sampling_count_ms;
	uint32_t lcd_uart_count_ms;
	// Analog measurements.
	volatile uint32_t vout_mv_buf[PSFE_VOUT_BUFFER_LENGTH];
	volatile uint32_t vout_mv_buf_idx;
	volatile uint32_t vout_mv;
	volatile uint32_t iout_ua;
	volatile uint32_t vmcu_mv;
	volatile uint8_t tmcu_degrees;
	volatile TRCS_range_t trcs_range;
#ifdef PSFE_SIGFOX_MONITORING
	uint32_t sigfox_next_time_seconds;
	uint8_t sigfox_id[SIGFOX_DEVICE_ID_LENGTH_BYTES];
	PSFE_sigfox_startup_data_t sigfox_startup_data;
	PSFE_sigfox_monitoring_data_t sigfox_monitoring_data;
	uint8_t sigfox_error_stack_data[PSFE_SIGFOX_ERROR_STACK_DATA_LENGTH];
#endif
} PSFE_context_t;

/*** MAIN local global variables ***/

static const uint32_t PSFE_VOUT_VOLTAGE_DIVIDER_RESISTANCE[PSFE_NUMBER_OF_BOARDS] = {998000, 998000, 599000, 599000, 998000, 998000, 998000, 599000, 599000, 998000};

static PSFE_context_t psfe_ctx;

/*** MAIN local functions ***/

/*******************************************************************/
void _PSFE_print_hw_version(void) {
	// Local variables.
	LCD_status_t lcd_status = LCD_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	// Print version.
	lcd_status = LCD_print_string(0, 0, " ATXFox ");
	LCD_stack_error();
	lcd_status = LCD_print_string(1, 0, " HW 1.0 ");
	LCD_stack_error();
	lptim1_status = LPTIM1_delay_milliseconds(2000, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_stack_error();
	IWDG_reload();
}

/*******************************************************************/
void _PSFE_print_sw_version(void) {
	// Local variables.
	LCD_status_t lcd_status = LCD_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	char_t sw_version_string[LCD_WIDTH_CHAR];
	// Build string.
	string_status = STRING_value_to_string((GIT_MAJOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[0]));
	STRING_stack_error();
	string_status = STRING_value_to_string((GIT_MAJOR_VERSION - 10 * (GIT_MAJOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[1]));
	STRING_stack_error();
	sw_version_string[2] = STRING_CHAR_DOT;
	string_status = STRING_value_to_string((GIT_MINOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[3]));
	STRING_stack_error();
	string_status = STRING_value_to_string((GIT_MINOR_VERSION - 10 * (GIT_MINOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[4]));
	STRING_stack_error();
	sw_version_string[5] = STRING_CHAR_DOT;
	string_status = STRING_value_to_string((GIT_COMMIT_INDEX / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[6]));
	STRING_stack_error();
	string_status = STRING_value_to_string((GIT_COMMIT_INDEX - 10 * (GIT_COMMIT_INDEX / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[7]));
	STRING_stack_error();
	// Print version.
	if (GIT_DIRTY_FLAG == 0) {
		lcd_status = LCD_print_string(0, 0, "   SW   ");
	}
	else {
		lcd_status = LCD_print_string(0, 0, "  SW !  ");
	}
	LCD_stack_error();
	lcd_status = LCD_print_string(1, 0, sw_version_string);
	LCD_stack_error();
	lptim1_status = LPTIM1_delay_milliseconds(2000, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_stack_error();
	IWDG_reload();
}

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
void _PSFE_print_sigfox_ep_id(void) {
	// Local variables.
	TD1208_status_t td1208_status = TD1208_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LCD_status_t lcd_status = LCD_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint8_t sigfox_ep_id[SIGFOX_DEVICE_ID_LENGTH_BYTES];
	char_t sigfox_ep_id_str[2 * SIGFOX_DEVICE_ID_LENGTH_BYTES];
	uint8_t idx = 0;
	// Read EP-ID.
	td1208_status = TD1208_get_sigfox_ep_id((uint8_t*) sigfox_ep_id);
	TD1208_stack_error();
	// Build corresponding string.
	for (idx=0 ; idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; idx++) {
		string_status = STRING_value_to_string(sigfox_ep_id[idx], STRING_FORMAT_HEXADECIMAL, 0, &(sigfox_ep_id_str[2 * idx]));
		STRING_stack_error();
	}
	lcd_status = LCD_print_string(0, 0, " EP  ID ");
	LCD_stack_error();
	// Print ID.
	if (td1208_status == TD1208_SUCCESS) {
		lcd_status = LCD_print_string(1, 0, sigfox_ep_id_str);
	}
	else {
		lcd_status = LCD_print_string(1, 0, "TD ERROR");
	}
	LCD_stack_error();
	lptim1_status = LPTIM1_delay_milliseconds(2000, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM1_stack_error();
	IWDG_reload();
}
#endif

/*******************************************************************/
void _PSFE_start(void) {
	// Start continuous timers.
	TIM2_start();
}

/*******************************************************************/
void __attribute__((optimize("-O0"))) _PSFE_adc_sampling_callback(void) {
	// Local variables.
	ADC_status_t adc1_status = ADC_SUCCESS;
	TRCS_status_t trcs_status = TRCS_SUCCESS;
	MATH_status_t math_status = MATH_SUCCESS;
	uint32_t generic_data_u32 = 0;
	int8_t temperature = 0;
	uint32_t vout_voltage_divider_current_ua = 0;
	// Perform measurements.
	adc1_status = ADC1_perform_measurements();
	ADC1_stack_error();
	trcs_status = TRCS_process(PSFE_ADC_SAMPLING_PERIOD_MS);
	TRCS_stack_error();
	// Update local VOUT.
	adc1_status = ADC1_get_data(ADC_DATA_INDEX_VOUT_MV, &generic_data_u32);
	ADC1_stack_error();
	if (adc1_status == ADC_SUCCESS) {
		// Add value to buffer.
		psfe_ctx.vout_mv_buf[psfe_ctx.vout_mv_buf_idx] = generic_data_u32;
		psfe_ctx.vout_mv_buf_idx++;
		if (psfe_ctx.vout_mv_buf_idx >= PSFE_VOUT_BUFFER_LENGTH) {
			psfe_ctx.vout_mv_buf_idx = 0;
		}
		// Compute average.
		math_status = MATH_average_u32((uint32_t*) psfe_ctx.vout_mv_buf, PSFE_VOUT_BUFFER_LENGTH, &generic_data_u32);
		MATH_stack_error();
		if (math_status == MATH_SUCCESS) {
			psfe_ctx.vout_mv = generic_data_u32;
		}
	}
	// Update local range and IOUT.
	psfe_ctx.trcs_range = TRCS_get_range();
	psfe_ctx.iout_ua = TRCS_get_iout();
	// Compute output voltage divider current.
	vout_voltage_divider_current_ua = (psfe_ctx.vout_mv * 1000) / (PSFE_VOUT_VOLTAGE_DIVIDER_RESISTANCE[PSFE_BOARD_INDEX]);
	// Remove offset current.
	if (psfe_ctx.iout_ua > vout_voltage_divider_current_ua) {
		psfe_ctx.iout_ua -= vout_voltage_divider_current_ua;
	}
	else {
		psfe_ctx.iout_ua = 0;
	}
	// Update local VMCU and TMCU.
	adc1_status = ADC1_get_data(ADC_DATA_INDEX_VMCU_MV, &generic_data_u32);
	ADC1_stack_error();
	psfe_ctx.vmcu_mv = (adc1_status == ADC_SUCCESS) ? generic_data_u32 : PSFE_ERROR_VALUE_VOLTAGE_16BITS;
	adc1_status = ADC1_get_tmcu(&temperature);
	ADC1_stack_error();
	if (adc1_status == ADC_SUCCESS) {
		// Convert to 1-complement.
		math_status = MATH_int32_to_signed_magnitude((int32_t) temperature, 7, &generic_data_u32);
		MATH_stack_error();
		if (math_status == MATH_SUCCESS) {
			psfe_ctx.tmcu_degrees = (uint8_t) generic_data_u32;
		}
		else {
			psfe_ctx.tmcu_degrees = PSFE_ERROR_VALUE_TEMPERATURE;
		}
	}
	else {
		psfe_ctx.tmcu_degrees = PSFE_ERROR_VALUE_TEMPERATURE;
	}
}

/*******************************************************************/
void _PSFE_lcd_uart_callback(void) {
	// Local variables.
	LCD_status_t lcd_status = LCD_SUCCESS;
#ifdef PSFE_SERIAL_MONITORING
	STRING_status_t string_status = STRING_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	char str_value[PSFE_STRING_VALUE_BUFFER_LENGTH];
#endif
	// Update display if enabled.
	if (psfe_ctx.por_flag == 0) {
		// LCD VOUT display.
		if (psfe_ctx.vout_mv > PSFE_VOUT_ERROR_THRESHOLD_MV) {
			lcd_status = LCD_print_value_5_digits(0, 0, psfe_ctx.vout_mv);
			LCD_stack_error();
			lcd_status = LCD_print_string(0, 5, "  V");
			LCD_stack_error();
		}
		else {
			lcd_status = LCD_print_string(0, 0, "NO INPUT");
			LCD_stack_error();
		}
		// LCD IOUT display.
		if (TRCS_get_bypass_switch_state() == 0) {
			lcd_status = LCD_print_value_5_digits(1, 0, psfe_ctx.iout_ua);
			LCD_stack_error();
			lcd_status = LCD_print_string(1, 5, " mA");
			LCD_stack_error();
		}
		else {
			lcd_status = LCD_print_string(1, 0, " BYPASS ");
			LCD_stack_error();
		}
	}
#if (defined PSFE_SERIAL_MONITORING) && !(defined DEBUG)
	// UART print.
	lpuart1_status = LPUART1_write_string("Vout=");
	LPUART1_stack_error();
	string_status = STRING_value_to_string((int32_t) psfe_ctx.vout_mv, STRING_FORMAT_DECIMAL, 0, str_value);
	STRING_stack_error();
	lpuart1_status = LPUART1_write_string(str_value);
	LPUART1_stack_error();
	lpuart1_status = LPUART1_write_string("mV Iout=");
	LPUART1_stack_error();
	if (TRCS_get_bypass_switch_state() == 0) {
		STRING_value_to_string((int32_t) psfe_ctx.iout_ua, STRING_FORMAT_DECIMAL, 0, str_value);
		STRING_stack_error();
		lpuart1_status = LPUART1_write_string(str_value);
		LPUART1_stack_error();
		lpuart1_status = LPUART1_write_string("uA Vmcu=");
		LPUART1_stack_error();
	}
	else {
		lpuart1_status = LPUART1_write_string("BYPASS Vmcu=");
		LPUART1_stack_error();
	}
	string_status = STRING_value_to_string((int32_t) psfe_ctx.vmcu_mv, STRING_FORMAT_DECIMAL, 0, str_value);
	STRING_stack_error();
	lpuart1_status = LPUART1_write_string(str_value);
	LPUART1_stack_error();
	lpuart1_status = LPUART1_write_string("mV Tmcu=");
	LPUART1_stack_error();
	string_status = STRING_value_to_string((int32_t) psfe_ctx.tmcu_degrees, STRING_FORMAT_DECIMAL, 0, str_value);
	STRING_stack_error();
	lpuart1_status = LPUART1_write_string(str_value);
	LPUART1_stack_error();
	lpuart1_status = LPUART1_write_string("dC\n");
	LPUART1_stack_error();
#endif
}

/*******************************************************************/
static void _PSFE_tim2_irq_callback(void) {
	// Increment counts.
	psfe_ctx.adc_sampling_count_ms += PSFE_TIM2_PERIOD_MS;
	psfe_ctx.lcd_uart_count_ms += PSFE_TIM2_PERIOD_MS;
	// Check periods.
	if (psfe_ctx.adc_sampling_count_ms >= PSFE_ADC_SAMPLING_PERIOD_MS) {
		_PSFE_adc_sampling_callback();
		psfe_ctx.adc_sampling_count_ms = 0;
	}
	if (psfe_ctx.lcd_uart_count_ms >= PSFE_LCD_UART_PRINT_PERIOD_MS) {
		_PSFE_lcd_uart_callback();
		psfe_ctx.lcd_uart_count_ms = 0;
	}
}

/*******************************************************************/
static void _PSFE_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	TIM_status_t tim2_status = TIM_SUCCESS;
	LCD_status_t lcd_status = LCD_SUCCESS;
#if (defined PSFE_SERIAL_MONITORING) && !(defined DEBUG)
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
#endif
#ifdef PSFE_SIGFOX_MONITORING
	RTC_status_t rtc_status = RTC_SUCCESS;
	TD1208_status_t td1208_status = TD1208_SUCCESS;
#endif
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
	// Start clocks.
	rcc_status = RCC_switch_to_hsi();
	RCC_stack_error();
	// Init watchdog.
#ifndef DEBUG
	iwdg_status = IWDG_init();
	IWDG_stack_error();
	IWDG_reload();
#endif
	// Init peripherals.
#ifdef PSFE_SIGFOX_MONITORING
	rtc_status = RTC_init();
	RTC_stack_error();
#endif
	tim2_status = TIM2_init(PSFE_TIM2_PERIOD_MS, &_PSFE_tim2_irq_callback);
	TIM2_stack_error();
	lptim1_status = LPTIM1_init();
	LPTIM1_stack_error();
	adc1_status = ADC1_init();
	ADC1_stack_error();
#if (defined PSFE_SERIAL_MONITORING) && !(defined DEBUG)
	lpuart1_status = LPUART1_init(NULL);
	LPUART1_stack_error();
#endif
	// Init components.
	lcd_status = LCD_init();
	LCD_stack_error();
	TRCS_init();
#ifdef PSFE_SIGFOX_MONITORING
	td1208_status = TD1208_init();
	TD1208_stack_error();
#endif
}

/*******************************************************************/
void _PSFE_init_context(void) {
	// Local variables.
	uint8_t idx = 0;
	// Init context.
	psfe_ctx.state = PSFE_STATE_VMCU_MONITORING;
	psfe_ctx.adc_sampling_count_ms = 0;
	psfe_ctx.lcd_uart_count_ms = 0;
	psfe_ctx.por_flag = 1;
	for (idx=0 ; idx<PSFE_VOUT_BUFFER_LENGTH ; idx++) psfe_ctx.vout_mv_buf[idx] = 0;
	psfe_ctx.vout_mv_buf_idx = 0;
	psfe_ctx.vout_mv = PSFE_ERROR_VALUE_VOLTAGE_16BITS;
	psfe_ctx.iout_ua = PSFE_ERROR_VALUE_VOLTAGE_24BITS;
	psfe_ctx.vmcu_mv = PSFE_ERROR_VALUE_VOLTAGE_16BITS;
	psfe_ctx.tmcu_degrees = PSFE_ERROR_VALUE_TEMPERATURE;
	psfe_ctx.trcs_range = TRCS_RANGE_NONE;
#ifdef PSFE_SIGFOX_MONITORING
	psfe_ctx.startup_frame_sent = 0;
	psfe_ctx.sigfox_next_time_seconds = PSFE_SIGFOX_PERIOD_SECONDS;
	for (idx=0 ; idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; idx++) psfe_ctx.sigfox_id[idx] = 0x00;
	for (idx=0 ; idx<PSFE_SIGFOX_MONITORING_DATA_LENGTH ; idx++) psfe_ctx.sigfox_monitoring_data.frame[idx] = 0x00;
	for (idx=0 ; idx<PSFE_SIGFOX_ERROR_STACK_DATA_LENGTH ; idx++) psfe_ctx.sigfox_error_stack_data[idx] = 0x00;
#endif
}

/*** MAIN function ***/

/*******************************************************************/
int main(void) {
	// Init board.
	_PSFE_init_hw();
	_PSFE_init_context();
	// Local variables.
	LCD_status_t lcd_status = LCD_SUCCESS;
#ifdef PSFE_SIGFOX_MONITORING
	TD1208_status_t td1208_status = TD1208_SUCCESS;
	ERROR_code_t error_code = 0;
	uint8_t idx = 0;
#endif
	// Start measurements.
	_PSFE_start();
	// Main loop.
	while (1) {
		// Perform state machine.
		switch (psfe_ctx.state) {
		case PSFE_STATE_VMCU_MONITORING:
			// Power-off detection.
			if (psfe_ctx.vmcu_mv < PSFE_OFF_VOLTAGE_THRESHOLD_MV) {
				psfe_ctx.state = PSFE_STATE_OFF;
			}
			// Power-on detection.
			if (psfe_ctx.vmcu_mv > PSFE_ON_VOLTAGE_THRESHOLD_MV) {
				if (psfe_ctx.por_flag != 0) {
					psfe_ctx.state = PSFE_STATE_POR;
				}
#ifdef PSFE_SIGFOX_MONITORING
				else {
					// Compute next state.
					psfe_ctx.state = PSFE_STATE_PERIOD_CHECK;
				}
#endif
			}
			IWDG_reload();
			break;
		case PSFE_STATE_POR:
			// Init TRCS board to supply output.
			TRCS_init();
#ifdef PSFE_SIGFOX_MONITORING
			// Reset Sigfox module.
			td1208_status = TD1208_reset();
			TD1208_stack_error();
			IWDG_reload();
#endif
			// Init screens.
			_PSFE_print_hw_version();
			_PSFE_print_sw_version();
			IWDG_reload();
#ifdef PSFE_SIGFOX_MONITORING
			// Read Sigfox EP-ID from module.
			_PSFE_print_sigfox_ep_id();
			// Send startup_message if needed.
			if (psfe_ctx.startup_frame_sent == 0) {
				// Build startup frame.
				psfe_ctx.sigfox_startup_data.reset_reason = ((RCC -> CSR) >> 24) & 0xFF;
				psfe_ctx.sigfox_startup_data.major_version = GIT_MAJOR_VERSION;
				psfe_ctx.sigfox_startup_data.minor_version = GIT_MINOR_VERSION;
				psfe_ctx.sigfox_startup_data.commit_index = GIT_COMMIT_INDEX;
				psfe_ctx.sigfox_startup_data.commit_id = GIT_COMMIT_ID;
				psfe_ctx.sigfox_startup_data.dirty_flag = GIT_DIRTY_FLAG;
				// Send startup message through Sigfox.
				td1208_status = TD1208_send_frame(psfe_ctx.sigfox_startup_data.frame, PSFE_SIGFOX_STARTUP_DATA_LENGTH);
				TD1208_stack_error();
				// Update flag.
				psfe_ctx.startup_frame_sent = 1;
			}
			// Send start message through Sigfox.
			lcd_status = LCD_print_string(0, 0, " ATXFox ");
			LCD_stack_error();
			lcd_status = LCD_print_string(1, 0, "starting");
			LCD_stack_error();
			IWDG_reload();
			// Check status.
			td1208_status = TD1208_send_bit(1);
			TD1208_stack_error();
#endif
			// Update flags.
			psfe_ctx.por_flag = 0;
			// Compute next state.
#ifdef PSFE_SIGFOX_MONITORING
			psfe_ctx.state = PSFE_STATE_PERIOD_CHECK;
#else
			psfe_ctx.state = PSFE_STATE_VMCU_MONITORING;
#endif
			IWDG_reload();
			break;
#ifdef PSFE_SIGFOX_MONITORING
		case PSFE_STATE_PERIOD_CHECK:
			// Check Sigfox period with RTC wake-up timer.
			if (RTC_get_time_seconds() >= psfe_ctx.sigfox_next_time_seconds) {
				// Update next time.
				psfe_ctx.sigfox_next_time_seconds += PSFE_SIGFOX_PERIOD_SECONDS;
				psfe_ctx.state = PSFE_STATE_SIGFOX;
			}
			else {
				psfe_ctx.state = PSFE_STATE_VMCU_MONITORING;
			}
			IWDG_reload();
			break;
		case PSFE_STATE_SIGFOX:
			// Build monitoring data.
			psfe_ctx.sigfox_monitoring_data.vout_mv = psfe_ctx.vout_mv;
			if (TRCS_get_bypass_switch_state() != 0) {
				psfe_ctx.sigfox_monitoring_data.trcs_range = TRCS_RANGE_NONE;
				psfe_ctx.sigfox_monitoring_data.iout_ua = PSFE_ERROR_VALUE_VOLTAGE_24BITS;
			}
			else {
				psfe_ctx.sigfox_monitoring_data.trcs_range = psfe_ctx.trcs_range;
				psfe_ctx.sigfox_monitoring_data.iout_ua = psfe_ctx.iout_ua;
			}
			psfe_ctx.sigfox_monitoring_data.vmcu_mv = psfe_ctx.vmcu_mv;

			psfe_ctx.sigfox_monitoring_data.tmcu_degrees = psfe_ctx.tmcu_degrees;
			// Send monitoring data.
			td1208_status = TD1208_send_frame(psfe_ctx.sigfox_monitoring_data.frame, PSFE_SIGFOX_MONITORING_DATA_LENGTH);
			TD1208_stack_error();
			// Check stack.
			if (ERROR_stack_is_empty() == 0) {
				// Read error stack.
				for (idx=0 ; idx<(PSFE_SIGFOX_ERROR_STACK_DATA_LENGTH / 2) ; idx++) {
					error_code = ERROR_stack_read();
					psfe_ctx.sigfox_error_stack_data[(2 * idx) + 0] = (uint8_t) ((error_code >> 8) & 0x00FF);
					psfe_ctx.sigfox_error_stack_data[(2 * idx) + 1] = (uint8_t) ((error_code >> 0) & 0x00FF);
				}
				// Send error stack data.
				td1208_status = TD1208_send_frame(psfe_ctx.sigfox_error_stack_data, PSFE_SIGFOX_ERROR_STACK_DATA_LENGTH);
				TD1208_stack_error();
				// Reset error stack.
				ERROR_stack_init();
			}
			// Compute next state.
			psfe_ctx.state = PSFE_STATE_VMCU_MONITORING;
			IWDG_reload();
			break;
#endif
		case PSFE_STATE_OFF:
			// Send Sigfox message and clear LCD screen.
			if (psfe_ctx.por_flag == 0) {
				// Turn current sensor off.
				TRCS_de_init();
				// Update flag.
				psfe_ctx.por_flag = 1;
				// Clear LCD.
				lcd_status = LCD_clear();
				LCD_stack_error();
#ifdef PSFE_SIGFOX_MONITORING
				// Send bit 0.
				td1208_status = TD1208_send_bit(0);
				TD1208_stack_error();
#endif
			}
			// Compute next state
			psfe_ctx.state = PSFE_STATE_VMCU_MONITORING;
			IWDG_reload();
			break;
		// Unknown state.
		default:
			psfe_ctx.state = PSFE_STATE_OFF;
			break;
		}
	}
	return 0;
}
