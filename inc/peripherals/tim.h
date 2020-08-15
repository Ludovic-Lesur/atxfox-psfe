/*
 * tim.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef TIM_H
#define TIM_H

/*** TIM functions ***/

void TIM21_Init(unsigned int period_ms);
unsigned char TIM21_GetFlag(void);
void TIM21_ClearFlag(void);

void TIM22_Init(void);
unsigned char TIM22_GetFlag(void);
void TIM22_ClearFlag(void);

#endif /* TIM_H */
