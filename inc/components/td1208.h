/*
 * td1208.h
 *
 *  Created on: 8 juil. 2019
 *      Author: Ludovic
 */

#ifndef TD1208_H
#define TD1208_H

/*** TD1208 macros ***/

#define SIGFOX_ID_LENGTH_BYTES	4

/*** TD1208 functions ***/

void TD1208_DisableEcho(void);
void TD1208_GetSigfoxId(unsigned char sigfox_id[SIGFOX_ID_LENGTH_BYTES]);
void TD1208_SendBit(unsigned char uplink_bit);
void TD1208_SendFrame(unsigned char* uplink_data, unsigned char uplink_data_length_bytes);

/*** TD1208 utility functions ***/

void TD1208_FillRxBuffer(unsigned char rx_byte);

#endif /* COMPONENTS_TD1208_H_ */
