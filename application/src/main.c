/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

// Registers.
#include "rcc_reg.h"
// Peripherals.
#include "exti.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "gpio_mapping.h"
#include "mode.h"
#include "nvic_priority.h"
#include "pwr.h"
#include "rcc.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
// Components.
#include "st7066u_driver_flags.h"
#include "td1208.h"
#include "trcs.h"
// Utils.
#include "error.h"
#include "string.h"
#include "terminal.h"
// Middleware
#include "analog.h"
#include "hmi.h"
// Applicative.
#include "error_base.h"
#include "mode.h"
#include "version.h"

/*** MAIN local macros ***/

#define PSFE_COMMON_TIMER_INSTANCE              TIM_INSTANCE_TIM2
#define PSFE_COMMON_TIMER_PERIOD_MS				100
#define PSFE_ADC_SAMPLING_PERIOD_MS				100
#define PSFE_LCD_UART_TERMINAL_PERIOD_MS	    300

#define PSFE_LOG_TERMINAL_INSTANCE              0

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

#if ((PSFE_BOARD_NUMBER == 1) || (PSFE_BOARD_NUMBER == 2) || (PSFE_BOARD_NUMBER == 5) || (PSFE_BOARD_NUMBER == 6) || (PSFE_BOARD_NUMBER == 7) || (PSFE_BOARD_NUMBER == 10))
#define PSFE_VOUT_VOLTAGE_DIVIDER_RESISTANCE    998000
#else
#define PSFE_VOUT_VOLTAGE_DIVIDER_RESISTANCE    599000
#endif

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
	volatile int32_t vout_mv_buf[PSFE_VOUT_BUFFER_LENGTH];
	volatile uint8_t vout_mv_buf_idx;
	volatile int32_t vout_mv;
	volatile int32_t iout_ua;
	volatile int32_t vmcu_mv;
	volatile int32_t tmcu_degrees;
	volatile TRCS_range_t trcs_range;
#ifdef PSFE_SIGFOX_MONITORING
	uint32_t sigfox_next_time_seconds;
	uint8_t sigfox_id[TD1208_SIGFOX_EP_ID_SIZE_BYTES];
	PSFE_sigfox_startup_data_t sigfox_startup_data;
	PSFE_sigfox_monitoring_data_t sigfox_monitoring_data;
	uint8_t sigfox_error_stack_data[PSFE_SIGFOX_ERROR_STACK_DATA_LENGTH];
#endif
} PSFE_context_t;

/*** MAIN local global variables ***/

static PSFE_context_t psfe_ctx;

/*** MAIN local functions ***/

/*******************************************************************/
void _PSFE_print_hw_version(void) {
	// Local variables.
	HMI_status_t hmi_status = HMI_SUCCESS;
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	// Print version.
	hmi_status = HMI_print_string(0, 0, " ATXFox ");
	HMI_stack_error(ERROR_BASE_HMI);
	hmi_status = HMI_print_string(1, 0, " HW 1.0 ");
	HMI_stack_error(ERROR_BASE_HMI);
	lptim_status = LPTIM_delay_milliseconds(2000, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_stack_error(ERROR_BASE_LPTIM);
	IWDG_reload();
}

/*******************************************************************/
void _PSFE_print_sw_version(void) {
	// Local variables.
	HMI_status_t hmi_status = HMI_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	char_t sw_version_string[ST7066U_DRIVER_SCREEN_WIDTH];
	// Build string.
	string_status = STRING_integer_to_string((GIT_MAJOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[0]));
	STRING_stack_error(ERROR_BASE_STRING);
	string_status = STRING_integer_to_string((GIT_MAJOR_VERSION - 10 * (GIT_MAJOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[1]));
	STRING_stack_error(ERROR_BASE_STRING);
	sw_version_string[2] = STRING_CHAR_DOT;
	string_status = STRING_integer_to_string((GIT_MINOR_VERSION / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[3]));
	STRING_stack_error(ERROR_BASE_STRING);
	string_status = STRING_integer_to_string((GIT_MINOR_VERSION - 10 * (GIT_MINOR_VERSION / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[4]));
	STRING_stack_error(ERROR_BASE_STRING);
	sw_version_string[5] = STRING_CHAR_DOT;
	string_status = STRING_integer_to_string((GIT_COMMIT_INDEX / 10), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[6]));
	STRING_stack_error(ERROR_BASE_STRING);
	string_status = STRING_integer_to_string((GIT_COMMIT_INDEX - 10 * (GIT_COMMIT_INDEX / 10)), STRING_FORMAT_DECIMAL, 0, &(sw_version_string[7]));
	STRING_stack_error(ERROR_BASE_STRING);
	// Print version.
	if (GIT_DIRTY_FLAG == 0) {
		hmi_status = HMI_print_string(0, 0, "   sw   ");
	}
	else {
		hmi_status = HMI_print_string(0, 0, "sw dirty");
	}
	HMI_stack_error(ERROR_BASE_HMI);
	hmi_status = HMI_print_string(1, 0, sw_version_string);
	HMI_stack_error(ERROR_BASE_HMI);
	lptim_status = LPTIM_delay_milliseconds(2000, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_stack_error(ERROR_BASE_LPTIM);
	IWDG_reload();
}

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
void _PSFE_print_sigfox_ep_id(void) {
	// Local variables.
	TD1208_status_t td1208_status = TD1208_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	HMI_status_t hmi_status = HMI_SUCCESS;
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	uint8_t sigfox_ep_id[TD1208_SIGFOX_EP_ID_SIZE_BYTES];
	char_t sigfox_ep_id_str[TD1208_SIGFOX_EP_ID_SIZE_BYTES * MATH_U8_SIZE_HEXADECIMAL_DIGITS];
	uint8_t idx = 0;
	// Read EP-ID.
	td1208_status = TD1208_get_sigfox_ep_id((uint8_t*) sigfox_ep_id);
	TD1208_stack_error(ERROR_BASE_TD1208);
	// Build corresponding string.
	for (idx=0 ; idx<TD1208_SIGFOX_EP_ID_SIZE_BYTES ; idx++) {
		string_status = STRING_integer_to_string(sigfox_ep_id[idx], STRING_FORMAT_HEXADECIMAL, 0, &(sigfox_ep_id_str[2 * idx]));
		STRING_stack_error(ERROR_BASE_STRING);
	}
	hmi_status = HMI_print_string(0, 0, " EP  ID ");
	HMI_stack_error(ERROR_BASE_HMI);
	// Print ID.
	if (td1208_status == TD1208_SUCCESS) {
		hmi_status = HMI_print_string(1, 0, sigfox_ep_id_str);
	}
	else {
		hmi_status = HMI_print_string(1, 0, "TD ERROR");
	}
	HMI_stack_error(ERROR_BASE_HMI);
	lptim_status = LPTIM_delay_milliseconds(2000, LPTIM_DELAY_MODE_ACTIVE);
	LPTIM_stack_error(ERROR_BASE_LPTIM);
	IWDG_reload();
}
#endif

/*******************************************************************/
void __attribute__((optimize("-O0"))) _PSFE_adc_sampling_callback(void) {
	// Local variables.
	ANALOG_status_t analog_status = ANALOG_SUCCESS;
	TRCS_status_t trcs_status = TRCS_SUCCESS;
	MATH_status_t math_status = MATH_SUCCESS;
	int32_t generic_data_s32 = 0;
	uint32_t generic_data_u32 = 0;
	int32_t vout_voltage_divider_current_ua = 0;
	// Perform measurements.
	trcs_status = TRCS_process(PSFE_ADC_SAMPLING_PERIOD_MS);
	TRCS_stack_error();
	// Update local VOUT.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VOUT_MV, &generic_data_s32);
	ANALOG_stack_error(ERROR_BASE_ANALOG);
	if (analog_status == ANALOG_SUCCESS) {
		// Add value to buffer.
		psfe_ctx.vout_mv_buf[psfe_ctx.vout_mv_buf_idx] = generic_data_s32;
		psfe_ctx.vout_mv_buf_idx++;
		if (psfe_ctx.vout_mv_buf_idx >= PSFE_VOUT_BUFFER_LENGTH) {
			psfe_ctx.vout_mv_buf_idx = 0;
		}
		// Compute average.
		math_status = MATH_average((int32_t*) psfe_ctx.vout_mv_buf, PSFE_VOUT_BUFFER_LENGTH, &generic_data_s32);
		MATH_stack_error(ERROR_BASE_MATH);
		if (math_status == MATH_SUCCESS) {
			psfe_ctx.vout_mv = generic_data_s32;
		}
	}
	// Update local range and IOUT.
	psfe_ctx.trcs_range = TRCS_get_range();
	psfe_ctx.iout_ua = TRCS_get_iout();
	// Compute output voltage divider current.
	vout_voltage_divider_current_ua = (psfe_ctx.vout_mv * 1000) / (PSFE_VOUT_VOLTAGE_DIVIDER_RESISTANCE);
	// Remove offset current.
	if (psfe_ctx.iout_ua > vout_voltage_divider_current_ua) {
		psfe_ctx.iout_ua -= vout_voltage_divider_current_ua;
	}
	else {
		psfe_ctx.iout_ua = 0;
	}
	// Update local VMCU and TMCU.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VMCU_MV, &generic_data_s32);
	ANALOG_stack_error(ERROR_BASE_ANALOG);
	psfe_ctx.vmcu_mv = (analog_status == ANALOG_SUCCESS) ? generic_data_s32 : PSFE_ERROR_VALUE_VOLTAGE_16BITS;
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_TMCU_DEGREES, &generic_data_s32);
	ANALOG_stack_error(ERROR_BASE_ANALOG);
	if (analog_status == ANALOG_SUCCESS) {
		// Convert to 1-complement.
		math_status = MATH_integer_to_signed_magnitude(generic_data_s32, 7, &generic_data_u32);
		MATH_stack_error(ERROR_BASE_MATH);
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
	HMI_status_t hmi_status = HMI_SUCCESS;
#ifdef PSFE_SERIAL_MONITORING
	TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
#endif
	// Update display if enabled.
	if (psfe_ctx.por_flag == 0) {
		// LCD VOUT display.
		if (psfe_ctx.vout_mv > PSFE_VOUT_ERROR_THRESHOLD_MV) {
			hmi_status = HMI_print_value(0, psfe_ctx.vout_mv, 3, "V");
			HMI_stack_error(ERROR_BASE_HMI);
		}
		else {
			hmi_status = HMI_print_string(0, 0, "NO INPUT");
			HMI_stack_error(ERROR_BASE_HMI);
		}
		// LCD IOUT display.
		if (TRCS_get_bypass_switch_state() == 0) {
		    if (psfe_ctx.iout_ua < 1000) {
		        hmi_status = HMI_print_value(1, psfe_ctx.iout_ua, 0, "ua");
                HMI_stack_error(ERROR_BASE_HMI);
		    }
		    else {
		        hmi_status = HMI_print_value(1, psfe_ctx.iout_ua, 3, "ma");
                HMI_stack_error(ERROR_BASE_HMI);
		    }
		}
		else {
			hmi_status = HMI_print_string(1, 0, " BYPASS ");
			HMI_stack_error(ERROR_BASE_HMI);
		}
	}
#ifdef PSFE_SERIAL_MONITORING
	// UART print.
	terminal_status = TERMINAL_print_string(PSFE_LOG_TERMINAL_INSTANCE, "Vout=");
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	terminal_status = TERMINAL_print_integer(PSFE_LOG_TERMINAL_INSTANCE, psfe_ctx.vout_mv, STRING_FORMAT_DECIMAL, 0);
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	terminal_status = TERMINAL_print_string(PSFE_LOG_TERMINAL_INSTANCE, "mV Iout=");
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	if (TRCS_get_bypass_switch_state() == 0) {
	    terminal_status = TERMINAL_print_integer(PSFE_LOG_TERMINAL_INSTANCE, psfe_ctx.iout_ua, STRING_FORMAT_DECIMAL, 0);
	    TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	    terminal_status = TERMINAL_print_string(PSFE_LOG_TERMINAL_INSTANCE, "uA Vmcu=");
	    TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	}
	else {
	    terminal_status = TERMINAL_print_string(PSFE_LOG_TERMINAL_INSTANCE, "BYPASS Vmcu=");
	    TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	}
	terminal_status = TERMINAL_print_integer(PSFE_LOG_TERMINAL_INSTANCE, psfe_ctx.vmcu_mv, STRING_FORMAT_DECIMAL, 0);
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	terminal_status = TERMINAL_print_string(PSFE_LOG_TERMINAL_INSTANCE, "mV Tmcu=");
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	terminal_status = TERMINAL_print_integer(PSFE_LOG_TERMINAL_INSTANCE, psfe_ctx.tmcu_degrees, STRING_FORMAT_DECIMAL, 0);
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
	terminal_status = TERMINAL_print_string(PSFE_LOG_TERMINAL_INSTANCE, "dC\n");
	TERMINAL_stack_error(ERROR_BASE_TERMINAL);
#endif
}

/*******************************************************************/
static void _PSFE_common_timer_irq_callback(void) {
	// Increment counts.
	psfe_ctx.adc_sampling_count_ms += PSFE_COMMON_TIMER_PERIOD_MS;
	psfe_ctx.lcd_uart_count_ms += PSFE_COMMON_TIMER_PERIOD_MS;
	// Check periods.
	if (psfe_ctx.adc_sampling_count_ms >= PSFE_ADC_SAMPLING_PERIOD_MS) {
		_PSFE_adc_sampling_callback();
		psfe_ctx.adc_sampling_count_ms = 0;
	}
	if (psfe_ctx.lcd_uart_count_ms >= PSFE_LCD_UART_TERMINAL_PERIOD_MS) {
		_PSFE_lcd_uart_callback();
		psfe_ctx.lcd_uart_count_ms = 0;
	}
}

/*******************************************************************/
static void _PSFE_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	ANALOG_status_t analog_status = ANALOG_SUCCESS;
	TIM_status_t tim_status = TIM_SUCCESS;
#ifdef PSFE_SERIAL_MONITORING
	TERMINAL_status_t terminal_status = TERMINAL_SUCCESS;
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
	// Init power module and clock tree.
	PWR_init();
	rcc_status = RCC_init(NVIC_PRIORITY_CLOCK);
	RCC_stack_error(ERROR_BASE_RCC);
	// Init GPIOs.
    GPIO_init();
    EXTI_init();
#ifndef DEBUG
    // Start independent watchdog.
    iwdg_status = IWDG_init();
    IWDG_stack_error(ERROR_BASE_IWDG);
#endif
    // High speed oscillator.
	rcc_status = RCC_switch_to_hsi();
	RCC_stack_error(ERROR_BASE_RCC);
	// Calibrate clocks.
	rcc_status = RCC_calibrate_internal_clocks(NVIC_PRIORITY_CLOCK_CALIBRATION);
	RCC_stack_error(ERROR_BASE_RCC);
#ifdef PSFE_SIGFOX_MONITORING
	// Init RTC.
	rtc_status = RTC_init(NULL, NVIC_PRIORITY_RTC);
	RTC_stack_error(ERROR_BASE_RTC);
#endif
	// Init delay timer.
	LPTIM_init(NVIC_PRIORITY_DELAY);
	// Init peripherals.
	tim_status = TIM_STD_init(PSFE_COMMON_TIMER_INSTANCE, NVIC_PRIORITY_ADC_SAMPLING_TIMER);
	TIM_stack_error(ERROR_BASE_TIM_ADC_SAMPLING);
	analog_status = ANALOG_init();
	ANALOG_stack_error(ERROR_BASE_ANALOG);
	analog_status = ANALOG_calibrate();
    ANALOG_stack_error(ERROR_BASE_ANALOG);
	// Init components.
	TRCS_init();
#ifdef PSFE_SERIAL_MONITORING
    terminal_status = TERMINAL_open(PSFE_LOG_TERMINAL_INSTANCE, NULL);
    TERMINAL_stack_error(ERROR_BASE_TERMINAL);
#endif
#ifdef PSFE_SIGFOX_MONITORING
	td1208_status = TD1208_init();
	TD1208_stack_error(ERROR_BASE_TD1208);
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
	for (idx=0 ; idx<TD1208_SIGFOX_EP_ID_SIZE_BYTES ; idx++) psfe_ctx.sigfox_id[idx] = 0x00;
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
	HMI_status_t hmi_status = HMI_SUCCESS;
#ifdef PSFE_SIGFOX_MONITORING
	TD1208_status_t td1208_status = TD1208_SUCCESS;
	ERROR_code_t error_code = 0;
	uint8_t idx = 0;
#endif
	// Start measurements.
    TIM_status_t tim_status = TIM_STD_start(PSFE_COMMON_TIMER_INSTANCE, (PSFE_COMMON_TIMER_PERIOD_MS * MATH_POWER_10[6]), &_PSFE_common_timer_irq_callback);
    TIM_stack_error(ERROR_BASE_TIM_ADC_SAMPLING);
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
		    // Init HMI.
		    hmi_status = HMI_init();
            HMI_stack_error(ERROR_BASE_HMI);
			// Init TRCS board to supply output.
			TRCS_init();
#ifdef PSFE_SIGFOX_MONITORING
			// Reset Sigfox module.
			td1208_status = TD1208_reset();
			TD1208_stack_error(ERROR_BASE_TD1208);
			IWDG_reload();
#endif
			// Init screens.
			_PSFE_print_hw_version();
			_PSFE_print_sw_version();
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
				TD1208_stack_error(ERROR_BASE_TD1208);
				// Update flag.
				psfe_ctx.startup_frame_sent = 1;
			}
			// Send start message through Sigfox.
			hmi_status = HMI_print_string(0, 0, " ATXFox ");
			HMI_stack_error(ERROR_BASE_HMI);
			hmi_status = HMI_print_string(1, 0, "starting");
			HMI_stack_error(ERROR_BASE_HMI);
			IWDG_reload();
			// Check status.
			td1208_status = TD1208_send_bit(1);
			TD1208_stack_error(ERROR_BASE_TD1208);
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
			if (RTC_get_uptime_seconds() >= psfe_ctx.sigfox_next_time_seconds) {
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
			TD1208_stack_error(ERROR_BASE_TD1208);
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
				TD1208_stack_error(ERROR_BASE_TD1208);
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
				hmi_status = HMI_clear();
				HMI_stack_error(ERROR_BASE_HMI);
#ifdef PSFE_SIGFOX_MONITORING
				// Send bit 0.
				td1208_status = TD1208_send_bit(0);
				TD1208_stack_error(ERROR_BASE_TD1208);
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
