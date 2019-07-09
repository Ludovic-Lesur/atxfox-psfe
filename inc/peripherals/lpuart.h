/*
 * lpuart.h
 *
 *  Created on: 9 juil. 2019
 *      Author: Ludo
 */

#ifndef LPUART_H
#define LPUART_H

/*** LPUART structures ***/

typedef enum {
	LPUART_FORMAT_BINARY,
	LPUART_FORMAT_HEXADECIMAL,
	LPUART_FORMAT_DECIMAL,
	LPUART_FORMAT_ASCII
} LPUART_Format;

/*** LPUART functions ***/

void LPUART1_Init(void);
void LPUART1_SendString(char* tx_string);
void LPUART1_SendValue(unsigned int tx_value, LPUART_Format format, unsigned char print_prefix);

#endif /* LPUART_H */
