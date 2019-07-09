/*
 * td1208.c
 *
 *  Created on: 8 juil. 2019
 *      Author: Ludovic
 */

#include "td1208.h"

#include "lptim.h"
#include "tim.h"
#include "usart.h"

/*** TD1208 local macros ***/

#define TD1208_RX_BUFFER_LENGTH_BYTES	32
#define TD1208_TIMEOUT_SECONDS			5
// Separator character.
#define TD1208_NULL						'\0'
#define TD1208_CR						'\r'
#define TD1208_LF						'\n'

/*** TD1208 local structures ***/

typedef struct {
	unsigned char rx_buf[TD1208_RX_BUFFER_LENGTH_BYTES]; 	// Receive buffer.
	unsigned int rx_buf_idx; 								// Index in RX buffer.
	unsigned char separator_received;						// Flag set as soon as a separator character was received.
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

/*** TD1208 functions ***/

/* DISABLE TD1208 ECHO ON UART.
 * @param:	None.
 * @return:	None.
 */
void TD1208_DisableEcho(void) {

	/* Build AT command */
	unsigned char at_command[5];
	at_command[0] = 'A';
	at_command[1] = 'T';
	at_command[2] = 'E';
	at_command[3] = '0';
	at_command[4] = TD1208_CR;

	/* Send command through UART */
	USART2_Send(at_command, 5);

	/* Flush buffer */
	LPTIM1_DelayMilliseconds(500);
	unsigned int idx = 0;
	for (idx=0 ; idx<TD1208_RX_BUFFER_LENGTH_BYTES ; idx++) td1208_ctx.rx_buf[idx] = 0;
	td1208_ctx.rx_buf_idx = 0;
	td1208_ctx.separator_received = 0;
}

/* RETRIEVE SIGFOX MODULE ID.
 * @param sigfox_id:	Byte array that will contain device ID.
 * @return:				None.
 */
void TD1208_GetSigfoxId(unsigned char sigfox_id[SIGFOX_ID_LENGTH_BYTES]) {

	/* Build AT command */
	unsigned char at_command[5];
	at_command[0] = 'A';
	at_command[1] = 'T';
	at_command[2] = 'I';
	at_command[3] = '7';
	at_command[4] = TD1208_CR;

	/* Send command through UART */
	USART2_Send(at_command, 5);

	/* Wait for ID */
	unsigned int loop_start_time = TIM22_GetSeconds();
	while (td1208_ctx.separator_received == 0) {
		if (TIM22_GetSeconds() > (loop_start_time + TD1208_TIMEOUT_SECONDS)) break;
	}
	td1208_ctx.separator_received = 0;

	/* Wait for OK */
	while (td1208_ctx.separator_received == 0) {
		if (TIM22_GetSeconds() > (loop_start_time + TD1208_TIMEOUT_SECONDS)) break;
	}
	td1208_ctx.separator_received = 0;

	/* Read device ID */
	unsigned char sigfox_id_ascii[(SIGFOX_ID_LENGTH_BYTES * 2)] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char sigfox_id_ascii_idx = (SIGFOX_ID_LENGTH_BYTES * 2) - 1;
	unsigned int idx = 0;
	for (idx=td1208_ctx.rx_buf_idx ; idx>0 ; idx--) {
		// Select all characters except CR and LF.
		if ((td1208_ctx.rx_buf[idx] != TD1208_CR) && (td1208_ctx.rx_buf[idx] != TD1208_LF) && (td1208_ctx.rx_buf[idx] != TD1208_NULL)) {
			sigfox_id_ascii[sigfox_id_ascii_idx] = td1208_ctx.rx_buf[idx];
			sigfox_id_ascii_idx--;
			if (sigfox_id_ascii_idx == 0) {
				break;
			}
		}
	}

	/* Convert to byte array */
	for (idx=0 ; idx<SIGFOX_ID_LENGTH_BYTES ; idx++) {
		sigfox_id[idx] = (TD1208_AsciiToHexa(sigfox_id_ascii[2*idx]) << 4) + TD1208_AsciiToHexa(sigfox_id_ascii[2*idx+1]);
	}

}

/* SEND A BIT OVER SIGFOX NETWORK.
 * @param uplink_bit:	Bit to send.
 * @return:				None.
 */
void TD1208_SendBit(unsigned char uplink_bit) {

	/* Build AT command */
	unsigned char at_command[10];
	at_command[0] = 'A';
	at_command[1] = 'T';
	at_command[2] = '$';
	at_command[3] = 'S';
	at_command[4] = 'B';
	at_command[5] = '=';
	at_command[6] = uplink_bit ? '1' : '0';
	at_command[7] = ',';
	at_command[8] = '2';
	at_command[9] = TD1208_CR;

	/* Send command through UART */
	USART2_Send(at_command, 10);
}

/* SEND A FRAME OVER SIGFOX NETWORK.
 * @param uplink_frame:			Byte array to send.
 * @param uplink_frame_length:	Number of bytes to send.
 * @return:						None.
 */
void TD1208_SendFrame(unsigned char* uplink_data, unsigned char uplink_data_length_bytes) {

	/* Build AT command */
	unsigned int idx = 0;
	unsigned char at_command[64];
	// Header.
	at_command[idx++] = 'A';
	at_command[idx++] = 'T';
	at_command[idx++] = '$';
	at_command[idx++] = 'S';
	at_command[idx++] = 'F';
	at_command[idx++] = '=';
	// Data.
	unsigned int data_idx = 0;
	for (data_idx=0 ; data_idx<uplink_data_length_bytes ; data_idx++) {
		at_command[idx++] = TD1208_HexaToAscii((uplink_data[data_idx] & 0xF0) >> 4);
		at_command[idx++] = TD1208_HexaToAscii((uplink_data[data_idx] & 0x0F));
	}
	// AT command end.
	at_command[idx++] = TD1208_CR;

	/* Send command through UART */
	USART2_Send(at_command, idx);
}

/* STORE A NEW BYTE IN RX COMMAND BUFFER.
 * @param rx_byte:	Incoming byte to store.
 * @return:			None.
 */
void TD1208_FillRxBuffer(unsigned char rx_byte) {
	td1208_ctx.rx_buf[td1208_ctx.rx_buf_idx] = rx_byte;
	// Set flag if required.
	if (rx_byte == TD1208_CR) {
		td1208_ctx.separator_received = 1;
	}
	// Increment index.
	td1208_ctx.rx_buf_idx++;
	// Manage roll-over.
	if (td1208_ctx.rx_buf_idx == TD1208_RX_BUFFER_LENGTH_BYTES) {
		td1208_ctx.rx_buf_idx = 0;
	}
}
