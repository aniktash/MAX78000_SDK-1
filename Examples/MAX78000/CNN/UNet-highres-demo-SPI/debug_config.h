/* *****************************************************************************
 * Copyright (C) 2022 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 **************************************************************************** */

#ifndef __DEBUG_CONFIG_H__
#define __DEBUG_CONFIG_H__

// This file configures the debug UART printf facility.
// Implementation of this facility is in debug.c

#include "global.h"
#include "board.h"

#define DEBUG_FORMAT_BUFFER_SIZE 256
#define DEBUG_VARG_SIZE (7*sizeof(uint32_t))

#define DEBUG_OUTPUT(data,size) board_debug_output(data,size)
#define DEBUG_ASSERT(exp) if(!(exp)) { led_assert(); }

// These locks enable DEBUG_PRINTF() to be called from any execution context
// but also impose interrupt latency.  Undefine to reduce latency but doing so
// requires that DEBUG_PRINTF() only be called from code that is synchronized
// in another fashion (such as two interrupts of the same priority).

#define DEBUG_LOCK(x) __disable_irq()
#define DEBUG_UNLOCK(x) __enable_irq()

#endif

