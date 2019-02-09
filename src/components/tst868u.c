/*
 * tst868u.c
 *
 *  Created on: 1 feb. 2019
 *      Author: Ludovic
 */

#include "tst868u.h"

#include "usart.h"

/*** TST868U local macros ***/

#define TST868U_UPLINK_FRAME_MAX_LENGTH_BYTES	12
#define TST868U_RX_BUFFER_LENGTH_BYTES			32
// Separator character.
#define TST868U_COMMAND_SEPARATOR				';'

/*** TST868U local structures ***/

typedef struct {
	unsigned char rx_buf[TST868U_RX_BUFFER_LENGTH_BYTES]; 	// Receive buffer.
	unsigned int rx_buf_idx; 								// Index in RX buffer.
	unsigned char separator_received;						// Flag set as soon as a separator character was received.
} TST868U_Context;

/*** TST868U local global variables ***/

static volatile TST868U_Context tst868u_ctx;

/*** TST868U functions ***/

/* RETRIEVE SIGFOX MODEM ID.
 * @param sigfox_id:	Byte array that will contain device ID.
 * @return:				None.
 */
void TST868U_GetSigfoxId(unsigned char sigfox_id[TST868U_SIGFOX_ID_LENGTH_BYTES]) {

	/* Build AT command */
	unsigned char at_command[5];
	at_command[0] = 0x00;
	at_command[1] = 'S';
	at_command[2] = 'F';
	at_command[3] = 'p';
	at_command[4] = TST868U_COMMAND_SEPARATOR;
	USART2_Send(at_command, 5);

	/* Read device ID */
	//while (tst868u_ctx.separator_received == 0);
}

/* SEND A BIT OVER SIGFOX NETWORK.
 * @param uplink_bit:	Bit to send.
 * @return:				None.
 */
void TST868U_SendByte(unsigned char uplink_byte) {

	/* Build AT command */
	unsigned char at_command[6];
	at_command[0] = 0x00;
	at_command[1] = 'S';
	at_command[2] = 'F';
	at_command[3] = 'm';
	at_command[4] = uplink_byte;
	at_command[5] = TST868U_COMMAND_SEPARATOR;

	/* Send command through UART */
	USART2_Send(at_command, 6);
}

/* SEND A FRAME OVER SIGFOX NETWORK.
 * @param uplink_data:			Byte array to send.
 * @param uplink_data_length:	Number of bytes to send.
 * @return:						None.
 */
void TST868U_SendFrame(unsigned char* uplink_data, unsigned char uplink_data_length) {
	// Check parameter.
	if (uplink_data_length <= TST868U_UPLINK_FRAME_MAX_LENGTH_BYTES) {

	}
}

/* STORE A NEW BYTE IN RX COMMAND BUFFER.
 * @param rx_byte:	Incoming byte to store.
 * @return:			None.
 */
void TST868U_FillRxBuffer(unsigned char rx_byte) {
	tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx] = rx_byte;
	// Set flag if required.
	if (tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx] == TST868U_COMMAND_SEPARATOR) {
		tst868u_ctx.separator_received = 1;
	}
	// Increment index.
	tst868u_ctx.rx_buf_idx++;
	// Manage roll-over.
	if (tst868u_ctx.rx_buf_idx == TST868U_RX_BUFFER_LENGTH_BYTES) {
		tst868u_ctx.rx_buf_idx = 0;
	}
}
