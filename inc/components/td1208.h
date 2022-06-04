/*
 * td1208.h
 *
 *  Created on: 8 july. 2019
 *      Author: Ludo
 */

#ifndef __TD1208_H__
#define __TD1208_H__

#include "parser.h"
#include "usart.h"

/*** TD1208 macros ***/

#define SIGFOX_DEVICE_ID_LENGTH_BYTES	4

/*** TD1208 structures ***/

typedef enum {
	TD1208_SUCCESS,
	TD1208_ERROR_TIMEOUT,
	TD1208_ERROR_REPONSE,
	TD1208_ERROR_BASE_USART = 0x0100,
	TD1208_ERROR_BASE_STRING = (TD1208_ERROR_BASE_USART + USART_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_PARSER = (TD1208_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_LAST = (TD1208_ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST)
} TD1208_status_t;

/*** TD1208 functions ***/

void TD1208_init(void);
TD1208_status_t TD1208_disable_echo_and_verbose(void);
TD1208_status_t TD1208_get_sigfox_id(unsigned char sigfox_device_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]);
TD1208_status_t TD1208_send_bit(unsigned char uplink_bit);
TD1208_status_t TD1208_send_frame(unsigned char* uplink_data, unsigned char uplink_data_length_bytes);
void TD1208_fill_rx_buffer(unsigned char rx_byte);

#endif /* COMPONENTS___TD1208_H___ */
