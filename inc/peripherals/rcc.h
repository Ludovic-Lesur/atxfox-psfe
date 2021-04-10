/*
 * rcc.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef RCC_H
#define RCC_H

/*** RCC macros ***/

#define RCC_HSI_FREQUENCY_KHZ	16000

/*** RCC functions ***/

void RCC_Init(void);
unsigned int RCC_GetSysclkKhz(void);
unsigned char RCC_SwitchToHsi(void);

#endif /* RCC_H */
