/*
 * lpuart.h
 *
 *  Created on: 9 juil. 2019
 *      Author: Ludo
 */

#ifndef __LPUART_H__
#define __LPUART_H__

/*** LPUART functions ***/

void LPUART1_init(void);
void LPUART1_send_string(char* tx_string);

#endif /* __LPUART_H__ */
