/*
 * exti.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "exti.h"

#include "exti_reg.h"
#include "mapping.h"
#include "mode.h"
#include "nvic.h"
#include "psfe.h"
#include "rcc_reg.h"
#include "syscfg_reg.h"
#include "trcs.h"

/*** EXTI local macros ***/

#define EXTI_RTSR_FTSR_MAX_INDEX	22

/*** EXTI local functions ***/

/* EXTI LINES 4-15 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) EXTI4_15_IRQHandler(void) {
	// Bypass switch (PB7).
	if (((EXTI -> PR) & (0b1 << (GPIO_TRCS_BYPASS.pin_index))) != 0) {
		// Set local flags.
		unsigned char bypass_state = GPIO_read(&GPIO_TRCS_BYPASS);
		PSFE_set_bypass_flag(bypass_state);
		TRCS_set_bypass_flag(bypass_state);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_TRCS_BYPASS.pin_index)); // PIFx='1' (writing '1' clears the bit).
	}
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
	EXTI -> PR |= 0x007BFFFF; // PIFx='1'.
}

/* CONFIGURE A GPIO AS EXTERNAL INTERRUPT SOURCE.
 * @param gpio:		GPIO to be attached to EXTI peripheral.
 * @edge_trigger:	Interrupt edge trigger (see EXTI_trigger_t epin_indexeration in exti.h).
 * @return:			None.
 */
void EXTI_configure_gpio(const GPIO_pin_t* gpio, EXTI_trigger_t edge_trigger) {
	// Select GPIO port.
	SYSCFG -> EXTICR[((gpio -> pin_index) / 4)] &= ~(0b1111 << (4 * ((gpio -> pin_index) % 4)));
	SYSCFG -> EXTICR[((gpio -> pin_index) / 4)] |= ((gpio -> port_index) << (4 * ((gpio -> pin_index) % 4)));
	// Select triggers.
	switch (edge_trigger) {
	// Rising edge only.
	case EXTI_TRIGGER_RISING_EDGE:
		EXTI -> IMR |= (0b1 << ((gpio -> pin_index))); // IMx='1'.
		EXTI -> RTSR |= (0b1 << ((gpio -> pin_index))); // Rising edge enabled.
		EXTI -> FTSR &= ~(0b1 << ((gpio -> pin_index))); // Falling edge disabled.
		break;
	// Falling edge only.
	case EXTI_TRIGGER_FALLING_EDGE:
		EXTI -> IMR |= (0b1 << ((gpio -> pin_index))); // IMx='1'.
		EXTI -> RTSR &= ~(0b1 << ((gpio -> pin_index))); // Rising edge disabled.
		EXTI -> FTSR |= (0b1 << ((gpio -> pin_index))); // Falling edge enabled.
		break;
	// Both edges.
	case EXTI_TRIGGER_ANY_EDGE:
		EXTI -> IMR |= (0b1 << ((gpio -> pin_index))); // IMx='1'.
		EXTI -> RTSR |= (0b1 << ((gpio -> pin_index))); // Rising edge enabled.
		EXTI -> FTSR |= (0b1 << ((gpio -> pin_index))); // Falling edge enabled.
		break;
	// Unknown configuration.
	default:
		EXTI -> IMR &= ~(0b1 << ((gpio -> pin_index))); // IMx='0'.
		EXTI -> RTSR &= ~(0b1 << ((gpio -> pin_index))); // Rising edge disabled.
		EXTI -> FTSR &= ~(0b1 << ((gpio -> pin_index))); // Falling edge disabled.
		break;
	}
	// Clear flag.
	EXTI -> PR |= (0b1 << ((gpio -> pin_index)));
}

/* CONFIGURE A LINE AS INTERNAL INTERRUPT SOURCE.
 * @param line:		Line to configure (see EXTI_line_t enum).
 * @edge_trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @return:			None.
 */
void EXTI_configure_line(EXTI_line_t line, EXTI_trigger_t edge_trigger) {
	// Select triggers.
	switch (edge_trigger) {
	// Rising edge only.
	case EXTI_TRIGGER_RISING_EDGE:
		EXTI -> IMR |= (0b1 << line); // IMx='1'.
		if (line <= EXTI_RTSR_FTSR_MAX_INDEX) {
			EXTI -> RTSR |= (0b1 << line); // Rising edge enabled.
			EXTI -> FTSR &= ~(0b1 << line); // Falling edge disabled.
		}
		break;
	// Falling edge only.
	case EXTI_TRIGGER_FALLING_EDGE:
		EXTI -> IMR |= (0b1 << line); // IMx='1'.
		if (line <= EXTI_RTSR_FTSR_MAX_INDEX) {
			EXTI -> RTSR &= ~(0b1 << line); // Rising edge disabled.
			EXTI -> FTSR |= (0b1 << line); // Falling edge enabled.
		}
		break;
	// Both edges.
	case EXTI_TRIGGER_ANY_EDGE:
		EXTI -> IMR |= (0b1 << line); // IMx='1'.
		if (line <= EXTI_RTSR_FTSR_MAX_INDEX) {
			EXTI -> RTSR |= (0b1 << line); // Rising edge enabled.
			EXTI -> FTSR |= (0b1 << line); // Falling edge enabled.
		}
		break;
	// Unknown configuration.
	default:
		break;
	}
	// Clear flag.
	if (line <= EXTI_RTSR_FTSR_MAX_INDEX) {
		EXTI -> PR |= (0b1 << line);
	}
}

/* CLEAR ALL EXTI FLAGS.
 * @param:	None.
 * @return:	None.
 */
void EXTI_clear_all_flags(void) {
	// Clear all flags.
	EXTI -> PR |= 0x007BFFFF; // PIFx='1'.
}
