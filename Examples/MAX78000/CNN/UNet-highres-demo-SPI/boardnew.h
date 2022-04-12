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

#ifndef __BOARDNEW_H__
#define __BOARDNEW_H__
#include "global.h"
#include "mxc_sys.h"

void board_init_new( void );
void board_sleep( void );
uint32_t board_debug_output( const uint8_t * data, uint32_t size );

// typedef enum
// {
//     board_led_off,
//     board_led_on,
//     board_led_toggle
// }
// board_led_t;
extern volatile bool read_cycle;

#define BOARD_LED_BLUE  0 // used with board_led()
#define BOARD_LED_GREEN 1
// #define BOARD_LED_RED   2

//void board_led( uint8_t board_led_ndx, board_led_t led );

// system DMA allocation macros
#define BOARD_DMA_CHANNEL_QSPI_OUT		1
#define BOARD_DMA_CHANNEL_QSPI_IN		2
#define BOARD_DMA_CHANNEL_DMACPY		3

#define BOARD_DMA_CHANNEL_QSPI_IN_ISR	DMA2_IRQHandler
#define BOARD_DMA_CHANNEL_QSPI_IN_IRQ   DMA2_IRQn
#define BOARD_PIN_SS0					MXC_GPIO_PIN_4 //ai85
#define SPI         	MXC_SPI0
#define SPI_IRQ     	SPI0_IRQn
#define SPI_SPEED       0 //NULL     // Bit Rate
#define SPI_INSTANCE_NUM    1

// test pins and related functions can be used profile code using a logic analyzer

//#define BOARD_TEST0_PIN         PIN_6 // J3.11, P1.6
//#define BOARD_TEST1_PIN         PIN_7 // J3.12, P1.7
//#define BOARD_TEST1_PORT_NDX    PORT_1
//#define BOARD_TEST1_PORT        MXC_GPIO_GET_GPIO(BOARD_TEST1_PORT_NDX)
//#define BOARD_TEST0_PORT_NDX    PORT_1
//#define BOARD_TEST0_PORT        MXC_GPIO_GET_GPIO(BOARD_TEST0_PORT_NDX)

// static inline void board_test0_set( void )
// {
//     BOARD_TEST0_PORT->out_set = BOARD_TEST0_PIN;
// }
// static inline void board_test0_clear( void )
// {
//     BOARD_TEST0_PORT->out_clr = BOARD_TEST0_PIN;
// }

// static inline void board_test1_set( void )
// {
//     BOARD_TEST1_PORT->out_set = BOARD_TEST1_PIN;
// }
// static inline void board_test1_clear( void )
// {
//     BOARD_TEST1_PORT->out_clr = BOARD_TEST1_PIN;
// }

#endif



