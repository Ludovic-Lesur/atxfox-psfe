/*
 * tst868u.h
 *
 *  Created on: 1 feb. 2019
 *      Author: Ludovic
 */

#ifndef TST868U_H
#define TST868U_H

/*** TST868U macros ***/

#define TST868U_SIGFOX_ID_LENGTH_BYTES	2

/*** TST868U functions ***/

void TST868U_GetSigfoxId(unsigned char sigfox_id[TST868U_SIGFOX_ID_LENGTH_BYTES]);
void TST868U_SendByte(unsigned char uplink_byte);
void TST868U_SendFrame(unsigned char* uplink_data, unsigned char uplink_data_length);

/*** TST868U utility functions ***/

void TST868U_FillRxBuffer(unsigned char rx_byte);

#endif /* TST868U_H */
