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





#include "global.h"
#include "mxc_pins.h"
#include "debug.h"
#include "icc.h"
#include "mxc_sys.h"
#include "mxc_assert.h"
#include "uart.h"
#include "gpio.h"
#include "spi.h"
#include "mxc_delay.h"


#include "qspis.h"
#include "cbuf.h"
#include "boardnew.h"  //CHANGE TO NEW

//static mxc_uart_regs_t * const s_debug_uart = MXC_UART0;
static const mxc_gpio_cfg_t gpio_cfg_ss0 =  { MXC_GPIO0, MXC_GPIO_PIN_4, MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_PULL_UP,MXC_GPIO_VSSEL_VDDIOH };
static const mxc_gpio_cfg_t gpio_cfg_ss1 =  { MXC_GPIO0, MXC_GPIO_PIN_11, MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_PULL_UP,MXC_GPIO_VSSEL_VDDIOH };
static const mxc_gpio_cfg_t gpio_cfg_ss2 =  { MXC_GPIO0, MXC_GPIO_PIN_10, MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_PULL_UP,MXC_GPIO_VSSEL_VDDIOH };
// static const gpio_cfg_t s_led_pin[] =
// {
// 	{ PORT_1, PIN_0, GPIO_FUNC_OUT, GPIO_PAD_NONE },
// 	{ PORT_1, PIN_1, GPIO_FUNC_OUT, GPIO_PAD_NONE },
// //	{ PORT_1, PIN_2, GPIO_FUNC_OUT, GPIO_PAD_NONE },
// };

void board_init_new( void )
{
	// initialize pin configuration, peripheral clocks, and interrupts
	// GPIO_Config( &gpio_cfg_spi17y0 );
	// GPIO_Config( &gpio_cfg_ss0 );
	// GPIO_IntConfig( &gpio_cfg_ss0, GPIO_INT_EDGE, GPIO_INT_FALLING );
	// GPIO_IntEnable( &gpio_cfg_ss0 );
    
	// for( uint32_t i = 0; i < ARRAY_COUNT( s_led_pin ); i++ )
	// {
	// 	board_led( i, board_led_off );
	// 	GPIO_Config( &s_led_pin[i] );
	// }
	// static const gpio_cfg_t gpio_cfg_test[] =
	// {
	// 	{ BOARD_TEST0_PORT_NDX, BOARD_TEST0_PIN, GPIO_FUNC_OUT, GPIO_PAD_NONE },
	// 	{ BOARD_TEST1_PORT_NDX, BOARD_TEST1_PIN, GPIO_FUNC_OUT, GPIO_PAD_NONE }
	// };
	// board_test0_clear();
	// board_test1_clear();
	// for( uint32_t i = 0; i < ARRAY_COUNT( gpio_cfg_test ); i++ )
	// {
	// 	GPIO_Config( &gpio_cfg_test[i] );
	// }

		mxc_spi_pins_t spi_pins;
		//board init
		spi_pins.clock  = TRUE;
		spi_pins.miso   = TRUE;
		spi_pins.mosi   = TRUE;
		spi_pins.vddioh = TRUE;
		spi_pins.sdio2  = TRUE;
		spi_pins.sdio3  = TRUE;
		spi_pins.ss0    = TRUE;
		spi_pins.ss1    = FALSE;
		spi_pins.ss2    = FALSE;


		if (MXC_SPI_Init(SPI, 0, 1, 1, 0, SPI_SPEED, spi_pins) != E_NO_ERROR) {
		     printf("\nSPI INITIALIZATION ERROR\n");
		     while (1) {}
		}


	//MXC_GPIO_Config( &gpio_cfg_spi0 );
	MXC_GPIO_Config( &gpio_cfg_ss0 );
	MXC_GPIO_Config( &gpio_cfg_ss1 );
	MXC_GPIO_Config( &gpio_cfg_ss2 );
	MXC_GPIO_IntConfig( &gpio_cfg_ss0, MXC_GPIO_INT_FALLING );
	MXC_GPIO_EnableInt( gpio_cfg_ss0.port,  MXC_GPIO_PIN_4);
	MXC_SYS_ClockEnable( MXC_SYS_PERIPH_CLOCK_DMA );
	//SYS_ClockEnable( SYS_PERIPH_CLOCK_UART0 );



	// GPIO_Config( &gpio_cfg_uart0 );
	// {
	// 	uint32_t div = PeripheralClock / 115200;
	// 	uint32_t baud0 = div >> 7;
	// 	uint32_t baud1 = div - (baud0 << 7);
	// 	s_debug_uart->ctrl = MXC_S_UART_CTRL_CHAR_SIZE_8;
	// 	s_debug_uart->baud0 = baud0;
	// 	s_debug_uart->baud1 = baud1;
	// 	s_debug_uart->int_en = MXC_F_UART_INT_EN_TX_FIFO_ALMOST_EMPTY;
	// }
	DEBUG_INIT();
	NVIC_EnableIRQ( GPIO0_IRQn );
	//NVIC_EnableIRQ(BusFault_IRQn);
	MXC_ICC_Enable(MXC_ICC0);
}

// void board_led( uint8_t ndx, board_led_t led )
// {
// 	// controls the board LED's.  Note that the red LED shares a pin
// 	// with JTAG TDI and therefore isn't usable unless the JTAG port
// 	// is turned off.  Turning the red LED on will halt the processor
// 	// if a host debugger is not attached.

// 	if( ndx <= ARRAY_COUNT( s_led_pin ) )
// 	{
// 		switch( led )
// 		{
// 			case board_led_on:
// 				GPIO_OutClr( &s_led_pin[ndx] );
// 				break;
// 			case board_led_off:
// 				GPIO_OutSet( &s_led_pin[ndx] );
// 				break;
// 			case board_led_toggle:
// 				GPIO_OutToggle( &s_led_pin[ndx] );
// 				break;
// 		}
// 	}
// }

void board_sleep( void )
{
	DEBUG_PROCESS();
#ifdef LOW_POWER
	// Application sleep logic goes here.
	__WFI();  // This will turn off the JTAG.
#endif
}

void GPIO0_IRQHandler( void )
{
	// GPIO interrupt is used instead of the QSPI SSA interrupt
	// due to the fact that the QSPI SSA interrupt has a latency of about 3us

	if( MXC_GPIO0->intfl & MXC_GPIO_PIN_4 )
	{
		if (!read_cycle){
			qspis_ss0();
			MXC_GPIO0->intfl_clr = MXC_GPIO_PIN_4;
			//NVIC_DisableIRQ(GPIO0_IRQn);
			read_cycle = true;
		} else {
			read_cycle = false;
			MXC_GPIO0->intfl_clr = MXC_GPIO_PIN_4;
		}

	}

}


// uint32_t board_debug_output( const uint8_t * data, uint32_t size )
// {
// 	// moves data to the debug UART FIFO.  returns the number of bytes actually moved.
// 	uint32_t i;
// 	uint32_t avail = MXC_UART_FIFO_DEPTH - ((s_debug_uart->status & MXC_F_UART_STATUS_TX_FIFO_CNT) >> MXC_F_UART_STATUS_TX_FIFO_CNT_POS);
// 	if( size > avail )
// 		size = avail;
// 	for(i=0;i<size;i++)
// 	{
// 		s_debug_uart->fifo = data[i];
// 	}
// 	return size;
// }
