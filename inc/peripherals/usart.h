/*
 * usart.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef USART_H
#define USART_H

/*** USART macros ***/

//#define PSFE_SIGFOX_TST868U
#define PSFE_SIGFOX_TD1208

/*** USART functions ***/

void USART2_Init(void);
void USART2_Send(unsigned char* tx_data, unsigned char tx_data_length);

/*** Error management ***/

#if (defined PSFE_SIGFOX_TST868U) && (defined PSFE_SIGFOX_TD1208)
#error "Only one Sigfox module type should be selected."
#endif

#endif /* USART_H */
