/*
 * td1208.c
 *
 *  Created on: 8 july 2019
 *      Author: Ludo
 */

#include "td1208.h"

#include "tim.h"
#include "usart.h"

/*** TD1208 local macros ***/

#define TD1208_BUFFER_LENGTH_BYTES		32
#define TD1208_TIMEOUT_SECONDS			5
// Separator character.
#define TD1208_NULL						'\0'
#define TD1208_CR						'\r'
#define TD1208_LF						'\n'

/*** TD1208 local structures ***/

typedef struct {
	unsigned char td1208_at_tx_buf[TD1208_BUFFER_LENGTH_BYTES];	// TX buffer.
	unsigned char td1208_at_rx_buf[TD1208_BUFFER_LENGTH_BYTES]; // RX buffer.
	unsigned int td1208_at_rx_buf_idx; 							// Index in RX buffer.
	unsigned char td1208_at_response_count;						// Number of responses received from TD module.
	unsigned char td1208_at_parsing_count; 						// Number of parsing function call(s).
	unsigned char td1208_at_received_ok;
	unsigned char td1208_sigfox_device_id[SIGFOX_DEVICE_ID_LENGTH_BYTES];
	unsigned char td1208_at_received_id;
} TD1208_Context;

/*** TD1208 local global variables ***/

static volatile TD1208_Context td1208_ctx;

/*** TD1208 local functions ***/

/* CONVERTS THE ASCII CODE OF AN HEXADECIMAL CHARACTER TO THE CORRESPONDING VALUE.
 * @param c:			Hexadecimal character to convert.
 * @return hexa_value:	Result of conversion.
 */
unsigned char TD1208_AsciiToHexa(unsigned char c) {
	unsigned char value = 0;
	if ((c >= '0') && (c <= '9')) {
		value = c - '0';
	}
	else {
		if ((c >= 'A') && (c <= 'F')) {
			value = c - 'A' + 10;
		}
	}
	return value;
}

/* RETURN THE ASCII CODE OF A GIVEN HEXADECIMAL VALUE.
 * @param hexa_value:	Hexadecimal value (0 to 15).
 * @return ascii_code:	Corresponding ASCII code ('0' to 'F').
 */
char TD1208_HexaToAscii(unsigned char hexa_value) {
	char ascii_code = '\0';
	if (hexa_value <= 9) {
		ascii_code = hexa_value + '0';
	}
	else {
		if ((hexa_value >= 10) && (hexa_value <= 15)) {
			ascii_code = (hexa_value-10) + 'A';
		}
	}
	return ascii_code;
}

/* CHECK IF A GIVEN CHARACTER IS HEXADECIMAL.
 * @param c:	Character to check.
 * @return:		1 in case of hexadecimal character, 0 otherwise.
 */
unsigned char TD1208_IsHexaChar(unsigned char c) {
	unsigned char result = 0;
	if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F'))) {
		result = 1;
	}
	return result;
}

/* RESET PARSING VARIABLES.
 * @param:  None.
 * @return: None.
 */
void TD1208_ResetParser(void) {
    unsigned int idx = 0;
    for (idx=0; idx<TD1208_BUFFER_LENGTH_BYTES ; idx++) {
    	td1208_ctx.td1208_at_rx_buf[idx] = 0;
    	td1208_ctx.td1208_at_tx_buf[idx] = 0;
    }
    td1208_ctx.td1208_at_rx_buf_idx = 0;
    td1208_ctx.td1208_at_response_count = 0;
    td1208_ctx.td1208_at_parsing_count = 0;
    td1208_ctx.td1208_at_received_ok = 0;
    td1208_ctx.td1208_at_received_id = 0;
}

/* PARSE AT RESPONSE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void TD1208_ParseAtRxBuffer(void) {
    unsigned int idx;
    unsigned char sigfox_device_id_ascii_raw[2 * SIGFOX_DEVICE_ID_LENGTH_BYTES] = {0x00};
    unsigned char sigfox_device_id_ascii[2 * SIGFOX_DEVICE_ID_LENGTH_BYTES] = {0x00};
    unsigned char sigfox_device_id_idx = 0;
    unsigned char sigfox_device_id_length = 0;
    // OK.
    for (idx=0 ; idx<(td1208_ctx.td1208_at_rx_buf_idx - 1) ; idx++) {
        if ((td1208_ctx.td1208_at_rx_buf[idx] == 'O') && (td1208_ctx.td1208_at_rx_buf[idx+1] == 'K')) {
            td1208_ctx.td1208_at_received_ok = 1;
            break;
        }
    }
    // Device ID.
	if (td1208_ctx.td1208_at_received_ok == 0) {
		for (idx=0 ; idx<(td1208_ctx.td1208_at_rx_buf_idx) ; idx++) {
			// Check character.
			if (TD1208_IsHexaChar(td1208_ctx.td1208_at_rx_buf[idx]) != 0) {
				sigfox_device_id_ascii_raw[sigfox_device_id_length] = td1208_ctx.td1208_at_rx_buf[idx];
				// Increment index.
				sigfox_device_id_length++;
			}
		}
		if (sigfox_device_id_length > 0) {
			// Pad MSB with zeroes.
			for (idx=((2 * SIGFOX_DEVICE_ID_LENGTH_BYTES) - sigfox_device_id_length) ; idx<(2 * SIGFOX_DEVICE_ID_LENGTH_BYTES) ; idx++) {
				sigfox_device_id_ascii[idx] = sigfox_device_id_ascii_raw[sigfox_device_id_idx];
				sigfox_device_id_idx++;
			}
			// Convert ASCII to byte array.
			for (idx=0 ; idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; idx++) {
				td1208_ctx.td1208_sigfox_device_id[idx] = 0;
				td1208_ctx.td1208_sigfox_device_id[idx] |= TD1208_AsciiToHexa(sigfox_device_id_ascii[(2 * idx) + 0]) << 4;
				td1208_ctx.td1208_sigfox_device_id[idx] |= TD1208_AsciiToHexa(sigfox_device_id_ascii[(2 * idx) + 1]);
			}
			// Set flag.
			td1208_ctx.td1208_at_received_id = 1;
		}
	}
    // Update parsing count.
    td1208_ctx.td1208_at_parsing_count++;
}

/* WAIT FOR RECEIVING OK RESPONSE.
 * @param:	None.
 * @return:	None.
 */
void TD1208_WaitForOk(void) {
	unsigned int loop_start_time = TIM22_GetSeconds();
	while (td1208_ctx.td1208_at_received_ok == 0) {
		// Decode buffer.
		if (td1208_ctx.td1208_at_response_count != td1208_ctx.td1208_at_parsing_count) {
			TD1208_ParseAtRxBuffer();
		}
		// Exit if timeout.
		if (TIM22_GetSeconds() > (loop_start_time + TD1208_TIMEOUT_SECONDS)) break;
	}
}

/* WAIT FOR RECEIVING OK RESPONSE.
 * @param:	None.
 * @return:	None.
 */
void TD1208_WaitForId(void) {
	unsigned int loop_start_time = TIM22_GetSeconds();
	while (td1208_ctx.td1208_at_received_id == 0) {
		// Decode buffer.
		if (td1208_ctx.td1208_at_response_count != td1208_ctx.td1208_at_parsing_count) {
			TD1208_ParseAtRxBuffer();
		}
		// Exit if timeout.
		if (TIM22_GetSeconds() > (loop_start_time + TD1208_TIMEOUT_SECONDS)) break;
	}
}

/*** TD1208 functions ***/

/* INIT TD1208 INTERFACE
 * @param:	None.
 * @return:	None.
 */
void TD1208_Init(void) {
	// Init buffers.
	TD1208_ResetParser();
}

/* DISABLE TD1208 ECHO ON UART.
 * @param:	None.
 * @return:	None.
 */
void TD1208_DisableEcho(void) {
	// Reset parser.
	TD1208_ResetParser();
	// Build AT command.
	td1208_ctx.td1208_at_tx_buf[0] = 'A';
	td1208_ctx.td1208_at_tx_buf[1] = 'T';
	td1208_ctx.td1208_at_tx_buf[2] = 'E';
	td1208_ctx.td1208_at_tx_buf[3] = '0';
	td1208_ctx.td1208_at_tx_buf[4] = TD1208_CR;
	// Send command through UART.
	USART2_Send((unsigned char*) td1208_ctx.td1208_at_tx_buf, 5);
	// Wait for response.
	TD1208_WaitForOk();
}

/* RETRIEVE SIGFOX MODULE ID.
 * @param td1208_sigfox_device_id:	Byte array that will contain device ID.
 * @return:							None.
 */
void TD1208_GetSigfoxId(unsigned char td1208_sigfox_device_id[SIGFOX_DEVICE_ID_LENGTH_BYTES]) {
	// Reset parser.
	TD1208_ResetParser();
	// Build AT command.
	td1208_ctx.td1208_at_tx_buf[0] = 'A';
	td1208_ctx.td1208_at_tx_buf[1] = 'T';
	td1208_ctx.td1208_at_tx_buf[2] = 'I';
	td1208_ctx.td1208_at_tx_buf[3] = '7';
	td1208_ctx.td1208_at_tx_buf[4] = TD1208_CR;
	// Send command through UART.
	USART2_Send((unsigned char*) td1208_ctx.td1208_at_tx_buf, 5);
	// Wait for ID.
	TD1208_WaitForId();
	TD1208_WaitForOk();
	// Return ID.
	unsigned char idx = 0;
	for (idx=0 ; idx<SIGFOX_DEVICE_ID_LENGTH_BYTES ; idx++) {
		td1208_sigfox_device_id[idx] = td1208_ctx.td1208_sigfox_device_id[idx];
	}
}

/* SEND A BIT OVER SIGFOX NETWORK.
 * @param uplink_bit:	Bit to send.
 * @return:				None.
 */
void TD1208_SendBit(unsigned char uplink_bit) {
	// Build AT command.
	td1208_ctx.td1208_at_tx_buf[0] = 'A';
	td1208_ctx.td1208_at_tx_buf[1] = 'T';
	td1208_ctx.td1208_at_tx_buf[2] = '$';
	td1208_ctx.td1208_at_tx_buf[3] = 'S';
	td1208_ctx.td1208_at_tx_buf[4] = 'B';
	td1208_ctx.td1208_at_tx_buf[5] = '=';
	td1208_ctx.td1208_at_tx_buf[6] = uplink_bit ? '1' : '0';
	td1208_ctx.td1208_at_tx_buf[7] = ',';
	td1208_ctx.td1208_at_tx_buf[8] = '2';
	td1208_ctx.td1208_at_tx_buf[9] = TD1208_CR;
	// Send command through UART.
	USART2_Send((unsigned char*) td1208_ctx.td1208_at_tx_buf, 10);
	// Wait for response.
	TD1208_WaitForOk();
}

/* SEND A FRAME OVER SIGFOX NETWORK.
 * @param uplink_frame:			Byte array to send.
 * @param uplink_frame_length:	Number of bytes to send.
 * @return:						None.
 */
void TD1208_SendFrame(unsigned char* uplink_data, unsigned char uplink_data_length_bytes) {
	// Build AT command.
	unsigned int idx = 0;
	// Header.
	td1208_ctx.td1208_at_tx_buf[idx++] = 'A';
	td1208_ctx.td1208_at_tx_buf[idx++] = 'T';
	td1208_ctx.td1208_at_tx_buf[idx++] = '$';
	td1208_ctx.td1208_at_tx_buf[idx++] = 'S';
	td1208_ctx.td1208_at_tx_buf[idx++] = 'F';
	td1208_ctx.td1208_at_tx_buf[idx++] = '=';
	// Data.
	unsigned int data_idx = 0;
	for (data_idx=0 ; data_idx<uplink_data_length_bytes ; data_idx++) {
		td1208_ctx.td1208_at_tx_buf[idx++] = TD1208_HexaToAscii((uplink_data[data_idx] & 0xF0) >> 4);
		td1208_ctx.td1208_at_tx_buf[idx++] = TD1208_HexaToAscii((uplink_data[data_idx] & 0x0F));
	}
	// AT command end.
	td1208_ctx.td1208_at_tx_buf[idx++] = TD1208_CR;
	// Send command through UART.
	USART2_Send((unsigned char*) td1208_ctx.td1208_at_tx_buf, idx);
}

/* STORE A NEW BYTE IN RX COMMAND BUFFER.
 * @param rx_byte:	Incoming byte to store.
 * @return:			None.
 */
void TD1208_FillRxBuffer(unsigned char rx_byte) {
	td1208_ctx.td1208_at_rx_buf[td1208_ctx.td1208_at_rx_buf_idx] = rx_byte;
	// Set flag if required.
	if (rx_byte == TD1208_LF) {
		td1208_ctx.td1208_at_response_count++;
	}
	// Increment index.
	td1208_ctx.td1208_at_rx_buf_idx++;
	// Manage roll-over.
	if (td1208_ctx.td1208_at_rx_buf_idx >= TD1208_BUFFER_LENGTH_BYTES) {
		td1208_ctx.td1208_at_rx_buf_idx = 0;
	}
}
