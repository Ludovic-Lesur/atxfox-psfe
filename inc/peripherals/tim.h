/*
 * tim.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef TIM_H
#define TIM_H

/*** TIM functions ***/

void TIM2_Init(unsigned int period_ms);
void TIM2_Start(void);

void TIM21_Init(void);
void TIM21_GetLsiFrequency(unsigned int* lsi_frequency_hz);
void TIM21_Disable(void);

void TIM22_Init(unsigned int period_ms);
void TIM22_Start(void);
void TIM22_Stop(void);

#endif /* TIM_H */
