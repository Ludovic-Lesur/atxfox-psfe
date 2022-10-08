/*
 * td1208.c
 *
 *  Created on: 8 july 2019
 *      Author: Ludo
 */

#include "td1208.h"

#include "nvic.h"
#include "parser.h"
#include "string.h"
#include "tim.h"
#include "usart.h"

/*** TD1208 local macros ***/

#define TD1208_BUFFER_LENGTH_BYTES			32
#define TD1208_TIMEOUT_COUNT				1000000
#define TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR	(2 * SIGFOX_DEVICE_ID_LENGTH_BYTES)

/*** TD1208 local structures ***/

typedef struct {
	// Command buffer.
	char command_buf[TD1208_BUFFER_LENGTH_BYTES];
	// Response buffer.
	volatile char response_buf[TD1208_BUFFER_LENGTH_BYTES];
	volatile uint32_t response_buf_idx;
	volatile uint8_t line_end_flag;
	PARSER_context_t parser;
} TD1208_context_t;

/*** TD1208 local global variables ***/

static TD1208_context_t td1208_ctx;

/*** TD1208 local functions ***/

/* RESET PARSING VARIABLES.
 * @param:  None.
 * @return: None.
 */
static void TD1208_reset_parser(void) {
	// Local variabless.
    uint32_t idx = 0;
    // Reset buffers and indexes.
    for (idx=0; idx<TD1208_BUFFER_LENGTH_BYTES ; idx++) {
    	td1208_ctx.response_buf[idx] = STRING_CHAR_NULL;
    	td1208_ctx.command_buf[idx] = STRING_CHAR_NULL;
    }
    td1208_ctx.response_buf_idx = 0;
    td1208_ctx.line_end_flag = 0;
    // Reset parser.
    td1208_ctx.parser.rx_buf = (char*) td1208_ctx.response_buf;
	td1208_ctx.parser.rx_buf_length = 0;
	td1208_ctx.parser.separator_idx = 0;
	td1208_ctx.parser.start_idx = 0;
}

/* WAIT FOR RECEIVING A RESPONSE.
 * @param:			None.
 * @return status:	Function execution status.
 */
static TD1208_status_t TD1208_wait_for_response(void) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	uint32_t loop_count = 0;
	// Wait for AT command separator.
	while (td1208_ctx.line_end_flag == 0) {
		// Exit if timeout.
		loop_count++;
		if (loop_count > TD1208_TIMEOUT_COUNT) {
			status = TD1208_ERROR_TIMEOUT;
			break;
		}
	}
	return status;
}

/* WAIT FOR RECEIVING OK RESPONSE.
 * @param:			None.
 * @return status:	Function execution status.
 */
static TD1208_status_t TD1208_wait_for_ok(void) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	// Wait for OK.
	status = TD1208_wait_for_response();
	if (status != TD1208_SUCCESS) goto errors;
	// Parse response.
	td1208_ctx.parser.rx_buf_length = td1208_ctx.response_buf_idx;
	parser_status = PARSER_compare(&td1208_ctx.parser, PARSER_MODE_COMMAND, "0");
	PARSER_status_check(TD1208_ERROR_BASE_PARSER);
errors:
	return status;
}

/*** TD1208 functions ***/

/* INIT TD1208 INTERFACE
 * @param:	None.
 * @return:	None.
 */
void TD1208_init(void) {
	// Init buffers.
	TD1208_reset_parser();
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_INTERRUPT_USART2);
}

/* DISABLE TD1208 ECHO ON UART.
 * @param:	None.
 * @return:	None.
 */
TD1208_status_t TD1208_disable_echo_and_verbose(void) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	USART_status_t usart_status = USART_SUCCESS;
	// Reset parser.
	TD1208_reset_parser();
	// Build AT command.
	td1208_ctx.command_buf[0] = 'A';
	td1208_ctx.command_buf[1] = 'T';
	td1208_ctx.command_buf[2] = 'E';
	td1208_ctx.command_buf[3] = '0';
	td1208_ctx.command_buf[4] = STRING_CHAR_CR;
	td1208_ctx.command_buf[5] = STRING_CHAR_NULL;
	// Send command through UART.
	usart_status = USART2_send_string(td1208_ctx.command_buf);
	USART_status_check(TD1208_ERROR_BASE_USART);
	// Wait for response.
	status = TD1208_wait_for_response();
	if (status != TD1208_SUCCESS) goto errors;
	// Reset parser.
	TD1208_reset_parser();
	// Build AT command.
	td1208_ctx.command_buf[0] = 'A';
	td1208_ctx.command_buf[1] = 'T';
	td1208_ctx.command_buf[2] = 'V';
	td1208_ctx.command_buf[3] = '0';
	td1208_ctx.command_buf[4] = STRING_CHAR_CR;
	td1208_ctx.command_buf[5] = STRING_CHAR_NULL;
	// Send command through UART.
	usart_status = USART2_send_string(td1208_ctx.command_buf);
	USART_status_check(TD1208_ERROR_BASE_USART);
	// Wait for response.
	status = TD1208_wait_for_response();
errors:
	return status;
}

/* RETRIEVE SIGFOX MODULE ID.
 * @param sigfox_device_id:	Byte array that will contain device ID.
 * @return status:			Function execution status.
 */
TD1208_status_t TD1208_get_sigfox_id(uint8_t* sigfox_device_id) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	USART_status_t usart_status = USART_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	uint8_t idx = 0;
	char temp_id[TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR];
	// Reset parser.
	TD1208_reset_parser();
	// Build AT command.
	td1208_ctx.command_buf[0] = 'A';
	td1208_ctx.command_buf[1] = 'T';
	td1208_ctx.command_buf[2] = 'I';
	td1208_ctx.command_buf[3] = '7';
	td1208_ctx.command_buf[4] = STRING_CHAR_CR;
	td1208_ctx.command_buf[5] = STRING_CHAR_NULL;
	// Send command through UART.
	usart_status = USART2_send_string(td1208_ctx.command_buf);
	USART_status_check(TD1208_ERROR_BASE_USART);
	// Wait for ID.
	status = TD1208_wait_for_response();
	if (status != TD1208_SUCCESS) goto errors;
	// Copy ID to temporary buffer.
	for (idx=0 ; idx<td1208_ctx.response_buf_idx ; idx++) {
		temp_id[idx] = td1208_ctx.response_buf[idx];
	}
	for (; idx<TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR ; idx++) {
		temp_id[idx] = '0';
	}
	// Pad most significant bits with zeroes.
	for (idx=0 ; idx<TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR ; idx++) {
		if (idx < (TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR - td1208_ctx.response_buf_idx)) {
			td1208_ctx.response_buf[idx] = '0';
		}
		else {
			td1208_ctx.response_buf[idx] = temp_id[idx - ((TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR - td1208_ctx.response_buf_idx))];
		}
	}
	td1208_ctx.response_buf[TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR] = STRING_CHAR_NULL;
	// Read ID.
	td1208_ctx.parser.rx_buf_length = TD1208_SIGFOX_DEVICE_ID_LENGTH_CHAR;
	parser_status = PARSER_get_byte_array(&td1208_ctx.parser, STRING_CHAR_NULL, SIGFOX_DEVICE_ID_LENGTH_BYTES, 1, sigfox_device_id, &idx);
	PARSER_status_check(TD1208_ERROR_BASE_PARSER);
errors:
	return status;
}

/* SEND A BIT OVER SIGFOX NETWORK.
 * @param uplink_bit:	Bit to send.
 * @return:				None.
 */
TD1208_status_t TD1208_send_bit(uint8_t uplink_bit) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	USART_status_t usart_status = USART_SUCCESS;
	// Reset parser.
	TD1208_reset_parser();
	// Build AT command.
	td1208_ctx.command_buf[0] = 'A';
	td1208_ctx.command_buf[1] = 'T';
	td1208_ctx.command_buf[2] = '$';
	td1208_ctx.command_buf[3] = 'S';
	td1208_ctx.command_buf[4] = 'B';
	td1208_ctx.command_buf[5] = '=';
	td1208_ctx.command_buf[6] = uplink_bit ? '1' : '0';
	td1208_ctx.command_buf[7] = ',';
	td1208_ctx.command_buf[8] = '2';
	td1208_ctx.command_buf[9] = STRING_CHAR_CR;
	td1208_ctx.command_buf[10] = STRING_CHAR_NULL;
	// Send command through UART.
	usart_status = USART2_send_string(td1208_ctx.command_buf);
	USART_status_check(TD1208_ERROR_BASE_USART);
	// Wait for response.
	status = TD1208_wait_for_ok();
errors:
	return status;
}

/* SEND A FRAME OVER SIGFOX NETWORK.
 * @param uplink_frame:			Byte array to send.
 * @param uplink_frame_length:	Number of bytes to send.
 * @return:						None.
 */
TD1208_status_t TD1208_send_frame(uint8_t* uplink_data, uint8_t uplink_data_length_bytes) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	USART_status_t usart_status = USART_SUCCESS;
	uint8_t idx = 0;
	// Reset parser.
	TD1208_reset_parser();
	// Header.
	td1208_ctx.command_buf[idx++] = 'A';
	td1208_ctx.command_buf[idx++] = 'T';
	td1208_ctx.command_buf[idx++] = '$';
	td1208_ctx.command_buf[idx++] = 'S';
	td1208_ctx.command_buf[idx++] = 'F';
	td1208_ctx.command_buf[idx++] = '=';
	// Data.
	string_status = STRING_byte_array_to_hexadecimal_string(uplink_data, uplink_data_length_bytes, 0, &(td1208_ctx.command_buf[idx]));
	STRING_status_check(TD1208_ERROR_BASE_STRING);
	// AT command end.
	idx += (2 * uplink_data_length_bytes);
	td1208_ctx.command_buf[idx++] = STRING_CHAR_CR;
	td1208_ctx.command_buf[idx++] = STRING_CHAR_NULL;
	// Send command through UART.
	usart_status = USART2_send_string(td1208_ctx.command_buf);
	USART_status_check(TD1208_ERROR_BASE_USART);
	// Wait for response.
	status = TD1208_wait_for_ok();
errors:
	return status;
}

/* STORE A NEW BYTE IN RX COMMAND BUFFER.
 * @param rx_byte:	Incoming byte to store.
 * @return:			None.
 */
void TD1208_fill_rx_buffer(uint8_t rx_byte) {
	// Append byte if LF flag is not allready set.
	if (td1208_ctx.line_end_flag == 0) {
		// Check ending characters.
		if ((rx_byte == STRING_CHAR_CR) || (rx_byte == STRING_CHAR_LF)) {
			td1208_ctx.response_buf[td1208_ctx.response_buf_idx] = STRING_CHAR_NULL;
			td1208_ctx.line_end_flag = 1;
		}
		else {
			// Store new byte.
			td1208_ctx.response_buf[td1208_ctx.response_buf_idx] = rx_byte;
			// Manage index.
			td1208_ctx.response_buf_idx++;
			if (td1208_ctx.response_buf_idx >= TD1208_BUFFER_LENGTH_BYTES) {
				td1208_ctx.response_buf_idx = 0;
			}
		}
	}
}
