/*
 * tst868u.c
 *
 *  Created on: 1 feb. 2019
 *      Author: Ludovic
 */

#include "tst868u.h"

#include "usart.h"

/*** TST868U local macros ***/

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

/* PING THE TST868U MODULE.
 * @param:	None.
 * @return:	None.
 */
void TST868U_Ping(void) {

	/* Build AT command */
	unsigned char at_command[5];
	at_command[0] = 0x00;
	at_command[1] = 'S';
	at_command[2] = 'F';
	at_command[3] = 'P';
	at_command[4] = TST868U_COMMAND_SEPARATOR;

	/* Send command through UART (OK; or KO;) */
	USART2_Send(at_command, 5);

	/* Wait for response */
	while (tst868u_ctx.separator_received == 0);
	tst868u_ctx.separator_received = 0;
}

/* RETRIEVE SIGFOX MODEM ID.
 * @param sigfox_id:	Byte array that will contain device ID.
 * @return:				None.
 */
void TST868U_GetSigfoxId(unsigned char sigfox_id[SIGFOX_ID_LENGTH_BYTES]) {

	/* Build AT command */
	unsigned char at_command[6];
	at_command[0] = 0x00;
	at_command[1] = 'S';
	at_command[2] = 'F';
	at_command[3] = 'I';
	at_command[4] = 'D';
	at_command[5] = TST868U_COMMAND_SEPARATOR;

	/* Send command through UART */
	USART2_Send(at_command, 6);

	/* Wait for response (OK; or KO;) */
	while (tst868u_ctx.separator_received == 0);
	tst868u_ctx.separator_received = 0;

	/* Read device ID */
	sigfox_id[0] = 0x00;
	sigfox_id[1] = 0x00;
	if ((tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx - 3] == 'O') && (tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx - 2] == 'K')) {
		sigfox_id[2] = tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx - 5];
		sigfox_id[3] = tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx - 4];
	}
	else {
		sigfox_id[2] = 0x00;
		sigfox_id[3] = 0x00;
	}
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

	/* Send command through UART (OK; or KO;) */
	USART2_Send(at_command, 6);

	/* Wait for response */
	while (tst868u_ctx.separator_received == 0);
	tst868u_ctx.separator_received = 0;

	/* Wait for transmission to complete (SENT;) */
	while (tst868u_ctx.separator_received == 0);
	tst868u_ctx.separator_received = 0;
}

/* STORE A NEW BYTE IN RX COMMAND BUFFER.
 * @param rx_byte:	Incoming byte to store.
 * @return:			None.
 */
void TST868U_FillRxBuffer(unsigned char rx_byte) {
	tst868u_ctx.rx_buf[tst868u_ctx.rx_buf_idx] = rx_byte;
	// Set flag if required.
	if (rx_byte == TST868U_COMMAND_SEPARATOR) {
		tst868u_ctx.separator_received = 1;
	}
	// Increment index.
	tst868u_ctx.rx_buf_idx++;
	// Manage roll-over.
	if (tst868u_ctx.rx_buf_idx == TST868U_RX_BUFFER_LENGTH_BYTES) {
		tst868u_ctx.rx_buf_idx = 0;
	}
}
