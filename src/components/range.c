/*
 * range.c
 *
 *  Created on: 30 déc. 2018
 *      Author: Ludovic
 */

#include "range.h"

#include "gpio.h"
#include "mapping.h"
#include "tim.h"

/*** RANGE local macros ***/

#define RANGE_RECOVERY_TIME_MS	100

/*** RANGE functions ***/

/* INIT ALL GPIOs CONTROLLING RANGE RELAYS.
 * @param:	None.
 * @return:	None.
 */
void RANGE_Init(void) {

	/* Init GPIOs */
	GPIO_Configure(GPIO_RANGE_LOW, Output, PushPull, HighSpeed, NoPullUpNoPullDown);
	GPIO_Configure(GPIO_RANGE_MIDDLE, Output, PushPull, HighSpeed, NoPullUpNoPullDown);
	GPIO_Configure(GPIO_RANGE_HIGH, Output, PushPull, HighSpeed, NoPullUpNoPullDown);
}

/* CONTROL ALL RELAYS TO ACTIVATE A GIVEN CURRENT RANGE.
 * @param range_level:	Range to activate (see RANGE_Level structures in range.h).
 * @return:				None.
 */
void RANGE_Set(RANGE_Level range_level) {

	/* Configure relays */
	switch (range_level) {

	case RANGE_NONE:
		GPIO_Write(GPIO_RANGE_LOW, 0);
		GPIO_Write(GPIO_RANGE_MIDDLE, 0);
		GPIO_Write(GPIO_RANGE_HIGH, 0);
		break;

	case RANGE_LOW:
		GPIO_Write(GPIO_RANGE_LOW, 1);
		TIM22_WaitMilliseconds(RANGE_RECOVERY_TIME_MS);
		GPIO_Write(GPIO_RANGE_MIDDLE, 0);
		GPIO_Write(GPIO_RANGE_HIGH, 0);
		break;

	case RANGE_MIDDLE:
		GPIO_Write(GPIO_RANGE_MIDDLE, 1);
		TIM22_WaitMilliseconds(RANGE_RECOVERY_TIME_MS);
		GPIO_Write(GPIO_RANGE_LOW, 0);
		GPIO_Write(GPIO_RANGE_HIGH, 0);
		break;

	case RANGE_HIGH:
		GPIO_Write(GPIO_RANGE_HIGH, 1);
		TIM22_WaitMilliseconds(RANGE_RECOVERY_TIME_MS);
		GPIO_Write(GPIO_RANGE_LOW, 0);
		GPIO_Write(GPIO_RANGE_MIDDLE, 0);
		break;

	default:
		// Unknwon parameter.
		break;
	}
}
