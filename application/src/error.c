/*
 * error.c
 *
 *  Created on: 25 sep. 2022
 *      Author: Ludo
 */

#include "error.h"

#include "types.h"

/*** ERROR local macros ***/

#define ERROR_STACK_DEPTH	32

/*** ERROR local structures ***/

/*******************************************************************/
typedef struct {
	ERROR_code_t stack[ERROR_STACK_DEPTH];
	uint8_t stack_idx;
} ERROR_context_t;

/*** ERROR local global variables ***/

static ERROR_context_t error_ctx;

/*** ERROR functions ***/

/*******************************************************************/
void ERROR_stack_init(void) {
	// Reset stack.
	for (error_ctx.stack_idx=0 ; error_ctx.stack_idx<ERROR_STACK_DEPTH ; error_ctx.stack_idx++) error_ctx.stack[error_ctx.stack_idx] = SUCCESS;
	error_ctx.stack_idx = 0;
}

/*******************************************************************/
void ERROR_stack_add(ERROR_code_t code) {
	// Check index.
	if (error_ctx.stack_idx < ERROR_STACK_DEPTH) {
		// Add error code.
		error_ctx.stack[error_ctx.stack_idx] = code;
		error_ctx.stack_idx++;
	}
}

/*******************************************************************/
ERROR_code_t ERROR_stack_read(void) {
	// Local variables.
	ERROR_code_t last_error = SUCCESS;
	// Check index.
	if (error_ctx.stack_idx > 0) {
		// Read last error.
		error_ctx.stack_idx--;
		last_error = error_ctx.stack[error_ctx.stack_idx];
		// Remove error.
		error_ctx.stack[error_ctx.stack_idx] = SUCCESS;
	}
	return last_error;
}

/*******************************************************************/
uint8_t ERROR_stack_is_empty(void) {
	// Local variables.
	uint8_t is_empty = (error_ctx.stack_idx == 0) ? 1 : 0;
	// Return flag.
	return is_empty;
}
