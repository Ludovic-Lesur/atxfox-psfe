/*
 * usart.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef USART_H
#define USART_H

/*** USART functions ***/

void USART2_Init(void);
void USART2_Send(unsigned char* tx_data, unsigned char tx_data_length);

#endif /* USART_H */
