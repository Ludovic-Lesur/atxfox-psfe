/*
 * tim.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef TIM_H
#define TIM_H

/*** TIM functions ***/

void TIM21_Init(void);
void TIM22_Init(void);
unsigned int TIM22_GetSeconds(void);
void TIM22_WaitMilliseconds(unsigned int ms_to_wait);

#endif /* TIM_H */
