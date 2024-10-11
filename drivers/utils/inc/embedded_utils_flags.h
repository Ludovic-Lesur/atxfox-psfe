/*
 * embedded_utils_flags.h
 *
 *  Created on: 06 oct. 2024
 *      Author: Ludo
 */

#ifndef __EMBEDDED_UTILS_FLAGS_H__
#define __EMBEDDED_UTILS_FLAGS_H__

#include "lpuart.h"

/*** Embedded utility functions compilation flags ***/

#define EMBEDDED_UTILS_HW_INTERFACE_ERROR_BASE_LAST     LPUART_ERROR_BASE_LAST

#define EMBEDDED_UTILS_TERMINAL_BUFFER_SIZE             64

#define EMBEDDED_UTILS_ERROR_STACK_DEPTH                32
#define EMBEDDED_UTILS_ERROR_STACK_SUCCESS_VALUE        0
//#define EMBEDDED_UTILS_ERROR_STACK_SIGFOX

#define EMBEDDED_UTILS_MATH_PRECISION                   0
//#define EMBEDDED_UTILS_MATH_COS_TABLE
//#define EMBEDDED_UTILS_MATH_SIN_TABLE
//#define EMBEDDED_UTILS_MATH_ATAN2

#endif /* __EMBEDDED_UTILS_FLAGS_H__ */
