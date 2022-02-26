/*
 * main.c
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

// Peripherals.
#include "adc.h"
#include "exti.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
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
// Applicative.
#include "psfe.h"

/*** MAIN function ***/

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {
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
	RCC_switch_to_hsi();
	RCC_enable_lsi();
	// Init watchdog.
#ifndef DEBUG
	IWDG_init();
#endif
	IWDG_reload();
	// Get effective LSI frequency.
	unsigned int lsi_frequency_hz = 0;
	TIM21_init();
	TIM21_get_lsi_frequency(&lsi_frequency_hz);
	TIM21_disable();
	// Init peripherals.
	RTC_init(lsi_frequency_hz);
	TIM2_init(PSFE_ADC_CONVERSION_PERIOD_MS);
	LPTIM1_init(lsi_frequency_hz);
	ADC1_init();
	USART2_init();
	LPUART1_init();
	// Init components.
	LCD_init();
	TRCS_init();
	TD1208_init();
	// Init applicative layer.
	PSFE_init();
	// Main loop.
	while (1) {
		PSFE_task();
		IWDG_reload();
	}
	return 0;
}
