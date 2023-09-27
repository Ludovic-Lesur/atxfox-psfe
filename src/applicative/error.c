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
	// Add error code.
	error_ctx.stack[error_ctx.stack_idx] = code;
	// Increment index.
	error_ctx.stack_idx = ((error_ctx.stack_idx + 1) % ERROR_STACK_DEPTH);
}

/*******************************************************************/
ERROR_code_t ERROR_stack_read(void) {
	// Read last error.
	error_ctx.stack_idx = (error_ctx.stack_idx == 0) ? (ERROR_STACK_DEPTH - 1) : (error_ctx.stack_idx - 1);
	ERROR_code_t last_error = error_ctx.stack[error_ctx.stack_idx];
	// Remove error.
	error_ctx.stack[error_ctx.stack_idx] = SUCCESS;
	// Return code.
	return last_error;
}

/*******************************************************************/
uint8_t ERROR_stack_is_empty(void) {
	// Local variables.
	uint8_t is_empty = 1;
	uint8_t idx = 0;
	// Loop on stack.
	for (idx=0 ; idx<ERROR_STACK_DEPTH ; idx++) {
		if (error_ctx.stack[idx] != SUCCESS) {
			is_empty = 0;
			break;
		}
	}
	return is_empty;
}
