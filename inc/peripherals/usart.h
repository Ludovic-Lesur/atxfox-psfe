/*
 * usart.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef USART_H
#define USART_H

/*** USART structures ***/

typedef enum {
	USART_FORMAT_BINARY,
	USART_FORMAT_HEXADECIMAL,
	USART_FORMAT_DECIMAL,
	USART_FORMAT_ASCII
} USART_Format;

/*** USART functions ***/

void USART2_Init(void);
void USART2_SendValue(unsigned int tx_value, USART_Format format, unsigned char print_prefix);
void USART2_SendString(char* tx_string);

#endif /* USART_H */
