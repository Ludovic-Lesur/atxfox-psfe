/*
 * td1208.h
 *
 *  Created on: 8 juil. 2019
 *      Author: Ludovic
 */

#ifndef TD1208_H
#define TD1208_H

/*** TD1208 macros ***/

#define SIGFOX_DEVICE_ID_LENGTH_BYTES	4

/*** TD1208 functions ***/

void TD1208_init(void);
void TD1208_disable_echo(void);
void TD1208_get_sigfox_id(unsigned char sigfox_device_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]);
void TD1208_send_bit(unsigned char uplink_bit);
void TD1208_send_frame(unsigned char* uplink_data, unsigned char uplink_data_length_bytes);
void TD1208_fill_rx_buffer(unsigned char rx_byte);

#endif /* COMPONENTS_TD1208_H_ */
