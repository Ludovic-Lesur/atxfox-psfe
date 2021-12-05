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
	if (((EXTI -> PR) & (0b1 << (GPIO_TRCS_BYPASS.gpio_num))) != 0) {
		// Set local flags.
		unsigned char bypass_state = GPIO_Read(&GPIO_TRCS_BYPASS);
		PSFE_SetBypassFlag(bypass_state);
		TRCS_SetBypassFlag(bypass_state);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_TRCS_BYPASS.gpio_num)); // PIFx='1' (writing '1' clears the bit).
	}
}

/*** EXTI functions ***/

/* INIT EXTI PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void EXTI_Init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 0); // SYSCFEN='1'.
	// Mask all sources by default.
	EXTI -> IMR = 0;
	// Clear all flags.
	EXTI -> PR |= 0x007BFFFF; // PIFx='1'.
}

/* CONFIGURE A GPIO AS EXTERNAL INTERRUPT SOURCE.
 * @param gpio:		GPIO to be attached to EXTI peripheral.
 * @edge_trigger:	Interrupt edge trigger (see EXTI_Trigger egpio_numeration in exti.h).
 * @return:			None.
 */
void EXTI_ConfigureGpio(const GPIO* gpio, EXTI_Trigger edge_trigger) {
	// Select GPIO port.
	SYSCFG -> EXTICR[((gpio -> gpio_num) / 4)] &= ~(0b1111 << (4 * ((gpio -> gpio_num) % 4)));
	SYSCFG -> EXTICR[((gpio -> gpio_num) / 4)] |= ((gpio -> gpio_port_index) << (4 * ((gpio -> gpio_num) % 4)));
	// Select triggers.
	switch (edge_trigger) {
	// Rising edge only.
	case EXTI_TRIGGER_RISING_EDGE:
		EXTI -> IMR |= (0b1 << ((gpio -> gpio_num))); // IMx='1'.
		EXTI -> RTSR |= (0b1 << ((gpio -> gpio_num))); // Rising edge enabled.
		EXTI -> FTSR &= ~(0b1 << ((gpio -> gpio_num))); // Falling edge disabled.
		break;
	// Falling edge only.
	case EXTI_TRIGGER_FALLING_EDGE:
		EXTI -> IMR |= (0b1 << ((gpio -> gpio_num))); // IMx='1'.
		EXTI -> RTSR &= ~(0b1 << ((gpio -> gpio_num))); // Rising edge disabled.
		EXTI -> FTSR |= (0b1 << ((gpio -> gpio_num))); // Falling edge enabled.
		break;
	// Both edges.
	case EXTI_TRIGGER_ANY_EDGE:
		EXTI -> IMR |= (0b1 << ((gpio -> gpio_num))); // IMx='1'.
		EXTI -> RTSR |= (0b1 << ((gpio -> gpio_num))); // Rising edge enabled.
		EXTI -> FTSR |= (0b1 << ((gpio -> gpio_num))); // Falling edge enabled.
		break;
	// Unknown configuration.
	default:
		EXTI -> IMR &= ~(0b1 << ((gpio -> gpio_num))); // IMx='0'.
		EXTI -> RTSR &= ~(0b1 << ((gpio -> gpio_num))); // Rising edge disabled.
		EXTI -> FTSR &= ~(0b1 << ((gpio -> gpio_num))); // Falling edge disabled.
		break;
	}
	// Clear flag.
	EXTI -> PR |= (0b1 << ((gpio -> gpio_num)));
}

/* CONFIGURE A LINE AS INTERNAL INTERRUPT SOURCE.
 * @param line:		Line to configure (see EXTI_Line enum).
 * @edge_trigger:	Interrupt edge trigger (see EXTI_Trigger enum).
 * @return:			None.
 */
void EXTI_ConfigureLine(EXTI_Line line, EXTI_Trigger edge_trigger) {
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
void EXTI_ClearAllFlags(void) {
	// Clear all flags.
	EXTI -> PR |= 0x007BFFFF; // PIFx='1'.
}
