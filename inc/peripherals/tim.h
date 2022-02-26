/*
 * tim.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef TIM_H
#define TIM_H

/*** TIM functions ***/

void TIM2_init(unsigned int period_ms);
void TIM2_start(void);

void TIM21_init(void);
void TIM21_get_lsi_frequency(unsigned int* lsi_frequency_hz);
void TIM21_disable(void);

void TIM22_init(unsigned int period_ms);
void TIM22_start(void);
void TIM22_stop(void);

#endif /* TIM_H */
