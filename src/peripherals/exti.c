/*
 * exti.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "exti.h"

#include "exti_reg.h"
#include "gpio.h"
#include "mapping.h"
#include "nvic.h"
#include "psfe.h"
#include "rcc_reg.h"
#include "syscfg_reg.h"
#include "trcs.h"

/*** EXTI local macros ***/

#define EXTI_RTSR_FTSR_RESERVED_INDEX	18
#define EXTI_RTSR_FTSR_MAX_INDEX		22

/*** EXTI local functions ***/

/* EXTI LINES 4-15 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) EXTI4_15_IRQHandler(void) {
	// Local variables.
	uint8_t bypass_state = 0;
	// Bypass switch (PB7).
	if (((EXTI -> PR) & (0b1 << (GPIO_TRCS_BYPASS.pin))) != 0) {
		// Set local flags.
		bypass_state = GPIO_read(&GPIO_TRCS_BYPASS);
		PSFE_set_bypass_flag(bypass_state);
		TRCS_set_bypass_flag(bypass_state);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_TRCS_BYPASS.pin)); // PIFx='1' (writing '1' clears the bit).
	}
}

/* SET EXTI TRIGGER.
 * @param trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @param line_idx:	Line index.
 * @return:			None.
 */
static void _EXTI_set_trigger(EXTI_trigger_t trigger, uint8_t line_idx) {
	// Select triggers.
	switch (trigger) {
	// Rising edge only.
	case EXTI_TRIGGER_RISING_EDGE:
		EXTI -> RTSR |= (0b1 << line_idx); // Rising edge enabled.
		EXTI -> FTSR &= ~(0b1 << line_idx); // Falling edge disabled.
		break;
	// Falling edge only.
	case EXTI_TRIGGER_FALLING_EDGE:
		EXTI -> RTSR &= ~(0b1 << line_idx); // Rising edge disabled.
		EXTI -> FTSR |= (0b1 << line_idx); // Falling edge enabled.
		break;
	// Both edges.
	case EXTI_TRIGGER_ANY_EDGE:
		EXTI -> RTSR |= (0b1 << line_idx); // Rising edge enabled.
		EXTI -> FTSR |= (0b1 << line_idx); // Falling edge enabled.
		break;
	// Unknown configuration.
	default:
		break;
	}
	// Clear flag.
	EXTI -> PR |= (0b1 << line_idx);
}

/*** EXTI functions ***/

/* INIT EXTI PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void EXTI_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 0); // SYSCFEN='1'.
	// Mask all sources by default.
	EXTI -> IMR = 0;
	// Clear all flags.
	EXTI_clear_all_flags();
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_4_15, 3);
}

/* CONFIGURE A GPIO AS EXTERNAL INTERRUPT SOURCE.
 * @param gpio:		GPIO to be attached to EXTI peripheral.
 * @param trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @return:			None.
 */
void EXTI_configure_gpio(const GPIO_pin_t* gpio, EXTI_trigger_t trigger) {
	// Select GPIO port.
	SYSCFG -> EXTICR[((gpio -> pin) / 4)] &= ~(0b1111 << (4 * ((gpio -> pin) % 4)));
	SYSCFG -> EXTICR[((gpio -> pin) / 4)] |= ((gpio -> port_index) << (4 * ((gpio -> pin) % 4)));
	// Set mask.
	EXTI -> IMR |= (0b1 << ((gpio -> pin))); // IMx='1'.
	// Select triggers.
	_EXTI_set_trigger(trigger, (gpio -> pin));
}

/* CONFIGURE A LINE AS INTERNAL INTERRUPT SOURCE.
 * @param line:		Line to configure (see EXTI_line_t enum).
 * @param trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @return:			None.
 */
void EXTI_configure_line(EXTI_line_t line, EXTI_trigger_t trigger) {
	// Set mask.
	EXTI -> IMR |= (0b1 << line); // IMx='1'.
	// Select triggers.
	if ((line != EXTI_RTSR_FTSR_RESERVED_INDEX) || (line <= EXTI_RTSR_FTSR_MAX_INDEX)) {
		_EXTI_set_trigger(trigger, line);
	}
}

/* CLEAR EXTI FLAG.
 * @param line:	Line to clear (see EXTI_line_t enum).
 * @return:		None.
 */
void EXTI_clear_flag(EXTI_line_t line) {
	// Clear flag.
	EXTI -> PR |= line; // PIFx='1'.
}

/* CLEAR ALL EXTI FLAGS.
 * @param:	None.
 * @return:	None.
 */
void EXTI_clear_all_flags(void) {
	// Clear all flags.
	EXTI -> PR |= 0x007BFFFF; // PIFx='1'.
}
