/*
 * td1208.h
 *
 *  Created on: 8 july. 2019
 *      Author: Ludo
 */

#ifndef __TD1208_H__
#define __TD1208_H__

#include "lptim.h"
#include "parser.h"
#include "string.h"
#include "usart.h"

/*** TD1208 macros ***/

#define SIGFOX_DEVICE_ID_LENGTH_BYTES	4

/*** TD1208 structures ***/

typedef enum {
	TD1208_SUCCESS,
	TD1208_ERROR_NULL_PARAMETER,
	TD1208_ERROR_UL_PAYLOAD_LENGTH,
	TD1208_ERROR_TIMEOUT,
	TD1208_ERROR_REPONSE,
	TD1208_ERROR_BASE_USART = 0x0100,
	TD1208_ERROR_BASE_LPTIM = (TD1208_ERROR_BASE_USART + USART_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_STRING = (TD1208_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_PARSER = (TD1208_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_LAST = (TD1208_ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST)
} TD1208_status_t;

/*** TD1208 functions ***/

void TD1208_init(void);
TD1208_status_t TD1208_reset(void);
TD1208_status_t TD1208_get_sigfox_id(uint8_t* sigfox_device_id);
TD1208_status_t TD1208_send_bit(uint8_t uplink_bit);
TD1208_status_t TD1208_send_frame(uint8_t* uplink_data, uint8_t uplink_data_length_bytes);
void TD1208_fill_rx_buffer(uint8_t rx_byte);

#define TD1208_status_check(error_base) { if (td1208_status != TD1208_SUCCESS) { status = error_base + td1208_status; goto errors; }}
#define TD1208_error_check() { ERROR_status_check(td1208_status, TD1208_SUCCESS, ERROR_BASE_TD1208); }
#define TD1208_error_check_print() { ERROR_status_check_print(td1208_status, TD1208_SUCCESS, ERROR_BASE_TD1208); }

#endif /* COMPONENTS___TD1208_H___ */
