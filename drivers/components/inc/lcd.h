/*
 * lcd.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef __LCD_H__
#define __LCD_H__

#include "lptim.h"
#include "string.h"
#include "types.h"

/*** LCD macros ***/

#define LCD_WIDTH_CHAR		8
#define LCD_HEIGHT_CHAR		2

/*** LCD structures ***/

/*!******************************************************************
 * \enum LCD_status_t
 * \brief LCD driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	LCD_SUCCESS = 0,
	LCD_ERROR_NULL_PARAMETER,
	LCD_ERROR_ROW_OVERFLOW,
	LCD_ERROR_COLUMN_OVERFLOW,
	// Low level drivers errors.
	LCD_ERROR_BASE_LPTIM1 = 0x0100,
	LCD_ERROR_BASE_STRING = (LCD_ERROR_BASE_LPTIM1 + LPTIM_ERROR_BASE_LAST),
	// Last base value.
	LCD_ERROR_BASE_LAST = (LCD_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST)
} LCD_status_t;

/*** LCD functions ***/

/*!******************************************************************
 * \fn LCD_status_t LCD_init(void)
 * \brief Init LCD driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LCD_status_t LCD_init(void);

/*!******************************************************************
 * \fn LCD_status_t LCD_clear(void)
 * \brief Clear LCD screen.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LCD_status_t LCD_clear(void);

/*!******************************************************************
 * \fn LCD_status_t LCD_print_string(uint8_t row, uint8_t column, char_t* str)
 * \brief Print a string on LCD screen.
 * \param[in]  	row: Row where to print.
 * \param[in]	column: Column where to print.
 * \param[in]	str: String to print.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LCD_status_t LCD_print_string(uint8_t row, uint8_t column, char_t* str);

/*!******************************************************************
 * \fn LCD_status_t LCD_print_value_5_digits(uint8_t row, uint8_t column, uint32_t value)
 * \brief Print a 5-digits value on LCD screen.
 * \param[in]  	row: Row where to print.
 * \param[in]	column: Column where to print.
 * \param[in]	value: Value to print.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LCD_status_t LCD_print_value_5_digits(uint8_t row, uint8_t column, uint32_t value);

/*******************************************************************/
#define LCD_exit_error(error_base) { if (lcd_status != LCD_SUCCESS) { status = error_base + lcd_status; goto errors; } }

/*******************************************************************/
#define LCD_stack_error(void) { if (lcd_status != LCD_SUCCESS) { ERROR_stack_add(ERROR_BASE_LCD + lcd_status); } }

/*******************************************************************/
#define LCD_stack_exit_error(error_code) { if (lcd_status != LCD_SUCCESS) { ERROR_stack_add(ERROR_BASE_LCD + lcd_status); status = error_code; goto errors; } }

#endif /* __LCD_H__ */
