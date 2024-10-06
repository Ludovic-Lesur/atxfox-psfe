/*
 * td1208.c
 *
 *  Created on: 08 jul. 2019
 *      Author: Ludo
 */

#include "td1208.h"

#include "lptim.h"
#include "mode.h"
#include "nvic.h"
#include "parser.h"
#include "string.h"
#include "tim.h"
#include "usart.h"

/*** TD1208 local macros ***/

#define TD1208_BUFFER_LENGTH_BYTES			32
#define TD1208_REPLY_BUFFER_DEPTH			8

#define TD1208_REPLY_PARSING_DELAY_MS		100
#define TD1208_REPLY_PARSING_TIMEOUT_MS		7000

#define TD1208_SIGFOX_EP_ID_SIZE_CHAR		(2 * SIGFOX_DEVICE_ID_LENGTH_BYTES)
#define TD1208_UL_PAYLOAD_SIZE_BYTES_MAX	12

/*** TD1208 local structures ***/

/*******************************************************************/
typedef struct {
	volatile char_t buffer[TD1208_BUFFER_LENGTH_BYTES];
	volatile uint8_t char_idx;
	PARSER_context_t parser;
} TD1208_reply_buffer_t;

/*******************************************************************/
typedef struct {
	// Command buffer.
	uint8_t command_buffer[TD1208_BUFFER_LENGTH_BYTES];
	// Response buffers.
	TD1208_reply_buffer_t reply[TD1208_REPLY_BUFFER_DEPTH];
	uint8_t reply_idx;
} TD1208_context_t;

/*** TD1208 local global variables ***/

#ifdef PSFE_SIGFOX_MONITORING
static TD1208_context_t td1208_ctx;
#endif

/*** TD1208 local functions ***/

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
static void _TD1208_fill_rx_buffer(uint8_t rx_byte) {
	// Read current index.
	uint8_t char_idx = td1208_ctx.reply[td1208_ctx.reply_idx].char_idx;
	// Store incoming byte.
	td1208_ctx.reply[td1208_ctx.reply_idx].buffer[char_idx] = rx_byte;
	// Manage index.
	char_idx = (char_idx + 1) % TD1208_BUFFER_LENGTH_BYTES;
	td1208_ctx.reply[td1208_ctx.reply_idx].char_idx = char_idx;
	// Check ending characters.
	if (rx_byte == STRING_CHAR_LF) {
		// Switch buffer.
		td1208_ctx.reply_idx = (td1208_ctx.reply_idx + 1) % TD1208_REPLY_BUFFER_DEPTH;
	}
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
static void _TD1208_reset_replies(void) {
	// Local variables.
	uint8_t rep_idx = 0;
    uint8_t char_idx = 0;
    // Reset replies buffers.
    for (rep_idx=0 ; rep_idx<TD1208_REPLY_BUFFER_DEPTH ; rep_idx++) {
    	for (char_idx=0; char_idx<TD1208_BUFFER_LENGTH_BYTES ; char_idx++) {
			td1208_ctx.reply[rep_idx].buffer[char_idx] = STRING_CHAR_NULL;
		}
		td1208_ctx.reply[rep_idx].char_idx = 0;
		// Reset parser.
		td1208_ctx.reply[rep_idx].parser.buffer = (char_t*) td1208_ctx.reply[rep_idx].buffer;
		td1208_ctx.reply[rep_idx].parser.buffer_size = 0;
		td1208_ctx.reply[rep_idx].parser.separator_idx = 0;
		td1208_ctx.reply[rep_idx].parser.start_idx = 0;
    }
    // Reset index and count.
    td1208_ctx.reply_idx = 0;
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
static TD1208_status_t _TD1208_send_command(char_t* command) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	USART_status_t usart2_status = USART_SUCCESS;
	uint8_t char_idx = 0;
	// Check parameter.
	if (command == NULL) {
		status = TD1208_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Fill command buffer.
	while (*command) {
		td1208_ctx.command_buffer[char_idx] = *(command++);
		char_idx++;
	}
	td1208_ctx.command_buffer[char_idx++] = STRING_CHAR_CR;
	td1208_ctx.command_buffer[char_idx++] = STRING_CHAR_NULL;
	// Send command through UART.
	usart2_status = USART2_write(td1208_ctx.command_buffer, char_idx);
	USART2_exit_error(TD1208_ERROR_BASE_USART2);
errors:
	return status;
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
static TD1208_status_t _TD1208_wait_for_string(char_t* ref) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint8_t rep_idx = 0;
	uint32_t parsing_time = 0;
	do {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(TD1208_REPLY_PARSING_DELAY_MS, LPTIM_DELAY_MODE_ACTIVE);
		LPTIM1_exit_error(TD1208_ERROR_BASE_LPTIM1);
		parsing_time += TD1208_REPLY_PARSING_DELAY_MS;
		// Loop on all replies.
		for (rep_idx=0 ; rep_idx<TD1208_REPLY_BUFFER_DEPTH ; rep_idx++) {
			// Parse reply.
			td1208_ctx.reply[rep_idx].parser.buffer_size = td1208_ctx.reply[rep_idx].char_idx;
			parser_status = PARSER_compare(&td1208_ctx.reply[rep_idx].parser, PARSER_MODE_HEADER, ref);
			// Update status.
			if (parser_status == PARSER_SUCCESS) {
				status = TD1208_SUCCESS;
				break;
			}
			else {
				status = (TD1208_ERROR_BASE_PARSER + parser_status);
			}
		}
	}
	while ((status != TD1208_SUCCESS) && (parsing_time < TD1208_REPLY_PARSING_TIMEOUT_MS));
errors:
	return status;
}
#endif

/*** TD1208 functions ***/

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
TD1208_status_t TD1208_init(void) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	USART_status_t usart2_status = USART_SUCCESS;
	// Init buffers.
	_TD1208_reset_replies();
	// Init USART.
	usart2_status = USART2_init(&_TD1208_fill_rx_buffer);
	USART2_exit_error(TD1208_ERROR_BASE_USART2);
errors:
	return status;
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
TD1208_status_t TD1208_reset(void) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	// Reset parser.
	_TD1208_reset_replies();
	// Send command.
	status = _TD1208_send_command("ATZ");
	if (status != TD1208_SUCCESS) goto errors;
	// Wait for command completion.
	status = _TD1208_wait_for_string("OK");
errors:
	return status;
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
TD1208_status_t TD1208_get_sigfox_ep_id(uint8_t* sigfox_ep_id) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	char_t temp_char = STRING_CHAR_NULL;
	uint8_t rep_idx = 0;
	uint8_t line_length = 0;
	uint8_t extracted_length = 0;
	int8_t idx = 0;
	// Check parameter.
	if (sigfox_ep_id == NULL) {
		status = TD1208_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset parser.
	_TD1208_reset_replies();
	// Send command.
	status = _TD1208_send_command("ATI7");
	if (status != TD1208_SUCCESS) goto errors;
	// Wait for command completion.
	status = _TD1208_wait_for_string("OK");
	if (status != TD1208_SUCCESS) goto errors;
	// Search ID in all replies.
	for (rep_idx=0 ; rep_idx<TD1208_REPLY_BUFFER_DEPTH ; rep_idx++) {
		// Replace unwanted characters.
		for (idx=0 ; idx<TD1208_BUFFER_LENGTH_BYTES ; idx++) {
			temp_char = td1208_ctx.reply[rep_idx].buffer[idx];
			if ((temp_char == STRING_CHAR_CR) || (temp_char == STRING_CHAR_LF)) {
				td1208_ctx.reply[rep_idx].buffer[idx] = STRING_CHAR_NULL;
			}
		}
		// Count the number of characters.
		line_length = 0;
		for (idx=0 ; idx<TD1208_BUFFER_LENGTH_BYTES ; idx++) {
			if (td1208_ctx.reply[rep_idx].buffer[idx] == STRING_CHAR_NULL) {
				break;
			}
			else {
				line_length++;
			}
		}
		// Check length.
		if ((line_length > 0) && (line_length <= TD1208_SIGFOX_EP_ID_SIZE_CHAR)) {
			// Shift buffer.
			for (idx=(TD1208_SIGFOX_EP_ID_SIZE_CHAR - 1) ; idx>=(TD1208_SIGFOX_EP_ID_SIZE_CHAR - line_length) ; idx--) {
				td1208_ctx.reply[rep_idx].buffer[idx] = td1208_ctx.reply[rep_idx].buffer[idx - line_length];
			}
			// Pad most significant symbols with zeroes.
			for (; idx>=0 ; idx--) {
				td1208_ctx.reply[rep_idx].buffer[idx] = '0';
			}
			// Try parsing ID.
			td1208_ctx.reply[rep_idx].parser.buffer_size = TD1208_SIGFOX_EP_ID_SIZE_CHAR;
			parser_status = PARSER_get_byte_array(&td1208_ctx.reply[rep_idx].parser, STRING_CHAR_NULL, SIGFOX_DEVICE_ID_LENGTH_BYTES, 1, sigfox_ep_id, &extracted_length);
			// Update status.
			if (parser_status == PARSER_SUCCESS) {
				status = TD1208_SUCCESS;
				break;
			}
			else {
				status = (TD1208_ERROR_BASE_PARSER + parser_status);
			}
		}
		else {
			status = (TD1208_ERROR_BASE_PARSER + PARSER_ERROR_BYTE_ARRAY_SIZE);
		}
	}
errors:
	return status;
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
TD1208_status_t TD1208_send_bit(uint8_t ul_bit) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	USART_status_t usart2_status = USART_SUCCESS;
	// Reset parser.
	_TD1208_reset_replies();
	// Build AT command.
	td1208_ctx.command_buffer[0] = 'A';
	td1208_ctx.command_buffer[1] = 'T';
	td1208_ctx.command_buffer[2] = '$';
	td1208_ctx.command_buffer[3] = 'S';
	td1208_ctx.command_buffer[4] = 'B';
	td1208_ctx.command_buffer[5] = '=';
	td1208_ctx.command_buffer[6] = ul_bit ? '1' : '0';
	td1208_ctx.command_buffer[7] = ',';
	td1208_ctx.command_buffer[8] = '2';
	td1208_ctx.command_buffer[9] = STRING_CHAR_CR;
	// Send command through UART.
	usart2_status = USART2_write(td1208_ctx.command_buffer, 10);
	USART2_exit_error(TD1208_ERROR_BASE_USART2);
	// Wait for reply.
	status = _TD1208_wait_for_string("OK");
errors:
	return status;
}
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*******************************************************************/
TD1208_status_t TD1208_send_frame(uint8_t* ul_payload, uint8_t ul_payload_size_bytes) {
	// Local variables.
	TD1208_status_t status = TD1208_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	USART_status_t usart2_status = USART_SUCCESS;
	uint8_t idx = 0;
	// Check parameters.
	if (ul_payload == NULL) {
		status = TD1208_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (ul_payload_size_bytes > TD1208_UL_PAYLOAD_SIZE_BYTES_MAX) {
		status = TD1208_ERROR_UL_PAYLOAD_LENGTH;
		goto errors;
	}
	// Reset parser.
	_TD1208_reset_replies();
	// Header.
	td1208_ctx.command_buffer[idx++] = 'A';
	td1208_ctx.command_buffer[idx++] = 'T';
	td1208_ctx.command_buffer[idx++] = '$';
	td1208_ctx.command_buffer[idx++] = 'S';
	td1208_ctx.command_buffer[idx++] = 'F';
	td1208_ctx.command_buffer[idx++] = '=';
	// Data.
	string_status = STRING_byte_array_to_hexadecimal_string(ul_payload, ul_payload_size_bytes, 0, (char_t*) &(td1208_ctx.command_buffer[idx]));
	STRING_exit_error(TD1208_ERROR_BASE_STRING);
	// AT command end.
	idx += (2 * ul_payload_size_bytes);
	td1208_ctx.command_buffer[idx++] = STRING_CHAR_CR;
	// Send command through UART.
	usart2_status = USART2_write(td1208_ctx.command_buffer, idx);
	USART2_exit_error(TD1208_ERROR_BASE_USART2);
	// Wait for reply.
	status = _TD1208_wait_for_string("OK");
errors:
	return status;
}
#endif
