/*
 * td1208.h
 *
 *  Created on: 08 jul. 2019
 *      Author: Ludo
 */

#ifndef __TD1208_H__
#define __TD1208_H__

#include "lptim.h"
#include "mode.h"
#include "parser.h"
#include "string.h"
#include "usart.h"

/*** TD1208 macros ***/

#define SIGFOX_DEVICE_ID_LENGTH_BYTES	4

/*** TD1208 structures ***/

/*!******************************************************************
 * \enum TD1208_status_t
 * \brief TD1208 driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	TD1208_SUCCESS,
	TD1208_ERROR_NULL_PARAMETER,
	TD1208_ERROR_UL_PAYLOAD_LENGTH,
	TD1208_ERROR_TIMEOUT,
	TD1208_ERROR_REPLY,
	// Low level drivers errors.
	TD1208_ERROR_BASE_USART = 0x0100,
	TD1208_ERROR_BASE_LPTIM = (TD1208_ERROR_BASE_USART + USART_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_STRING = (TD1208_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	TD1208_ERROR_BASE_PARSER = (TD1208_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	// Last base value.
	TD1208_ERROR_BASE_LAST = (TD1208_ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST)
} TD1208_status_t;

/*** TD1208 functions ***/

#ifdef PSFE_SIGFOX_MONITORING
/*!******************************************************************
 * \fn void TD1208_init(void)
 * \brief Init TD1208 driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TD1208_status_t TD1208_init(void);
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*!******************************************************************
 * \fn TD1208_status_t TD1208_reset(void)
 * \brief Reset TD1208 chip.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TD1208_status_t TD1208_reset(void);
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*!******************************************************************
 * \fn TD1208_status_t TD1208_get_sigfox_id(uint8_t* sigfox_device_id)
 * \brief Get TD1208 Sigfox EP-ID.
 * \param[in]  	none
 * \param[out] 	sigfox_ep_id: Pointer to byte array that will contain the Sigfox EP-ID.
 * \retval		Function execution status.
 *******************************************************************/
TD1208_status_t TD1208_get_sigfox_ep_id(uint8_t* sigfox_ep_id);
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*!******************************************************************
 * \fn TD1208_status_t TD1208_send_bit(uint8_t ul_bit)
 * \brief Send a bit over Sigfox network.
 * \param[in]  	ul_bit: Bit to send.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TD1208_status_t TD1208_send_bit(uint8_t ul_bit);
#endif

#ifdef PSFE_SIGFOX_MONITORING
/*!******************************************************************
 * \fn TD1208_status_t TD1208_send_frame(uint8_t* ul_payload, uint8_t ul_payload_size_bytes)
 * \brief Send a frame over Sigfox network.
 * \param[in]  	ul_payload: Byte array to send.
 * \param[in]	ul_payload_size_bytes: Number of bytes to send.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TD1208_status_t TD1208_send_frame(uint8_t* ul_payload, uint8_t ul_payload_size_bytes);
#endif

/*******************************************************************/
#define TD1208_exit_error(error_base) { if (td1208_status != TD1208_SUCCESS) { status = error_base + td1208_status; goto errors; } }

/*******************************************************************/
#define TD1208_stack_error(void) { if (td1208_status != TD1208_SUCCESS) { ERROR_stack_add(ERROR_BASE_TD1208 + td1208_status); } }

/*******************************************************************/
#define TD1208_stack_exit_error(error_code) { if (td1208_status != TD1208_SUCCESS) { ERROR_stack_add(ERROR_BASE_TD1208 + td1208_status); status = error_code; goto errors; } }

#endif /* COMPONENTS___TD1208_H___ */
