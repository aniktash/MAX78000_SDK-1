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
#include "board.h"

#include "boardnew.h"
#include "qspis.h"
#include "qp.h"
#include "debug.h"
#include "cbuf.h"

#include "dmacpy.h"
#include "mxc.h"
#include "cnn.h"
#include "mxc_delay.h"
#include "camera_util.h"
#include "camera.h"

// To use camera, disable PATTERN_GEN in camera_util.h



#define USE_SPI   // for testing to disble SPI

#define DATA_BLOCK  64  // size of block of data block to transfer by qspi

#define QSPIS_CBUF_SIZE 32768	// size of the in/out QSPI cbuf's
#define TIMER_FREQ_HZ	100  // frequency of the systick timer which drives the timer_ functions

#define NUM_PIXELS 7744 // 88x88
#define NUM_IN_CHANNLES 48
#define NUM_OUT_CHANNLES 64

volatile uint32_t cnn_time; // Stopwatch
volatile uint32_t cnn_unload_time; // Stopwatch

extern uint32_t RED_VALUE;

typedef uint32_t (*process_cb_t)( void *dst, const void *src, uint32_t size );
volatile bool read_cycle = false;



static uint32_t loopback_cb( void *dst, const void *src, uint32_t size )
{
	// called to process a block of data in loopback mode
	dmacpy( dst, src, size );
	return size;
}


static void process_stream( process_cb_t proc_cb )
{
	// loops through the QSPIS in/out cbuf's and process
	// host data via proc_cb()

	void *p_in;
	void *p_out;
	cbuf_size_t size;

	cbuf_size_t in_size;
	cbuf_size_t out_size;

	while( !qspis_reset() )
	{
		out_size = qspis_write_aquire( (void **)&p_out );
		in_size = qspis_read_aquire( (void **)&p_in );

		// p_out and out_size describe the output buffer currently
		// available for writing data out to the host.

		// p_in and in_size describe the input buffer currently available
		// for reading data in from the host.

		// The minimum of in_size and out_size is the number of bytes
		// that can be processed in this iteration.
		if( in_size > out_size  )
			size = out_size;
		else
			size = in_size;
		if( size )
		{
//			DEBUG_PRINTF(("B: out=%04x, in=%04x, size=%x\r\n",*(uint32_t *)p_out,*(uint32_t *)p_in, size));
//			printf("B: out=%04x, in=%04x, size=%x\n",*(uint32_t *)p_out,*(uint32_t *)p_in, size);
			size = proc_cb( p_out, p_in, size );	// perform whichever data processing function specified previously
//			DEBUG_PRINTF(("A: out=%04x, in=%04x, size=%x\r\n",*(uint32_t *)p_out,*(uint32_t *)p_in, size));
//			printf("A: out=%04x, in=%04x, size=%x\n",*(uint32_t *)p_out,*(uint32_t *)p_in, size);

			// adjust each cbuf to reflect the amount of data processed.
			qspis_read_release( size );
			qspis_write_release( size );
		}
		else
		{
			// nothing to do this iteration
			board_sleep();
		}
	}
}

int main( void )
{
	// The QSPI peripheral is configured in slave mode and is referred to as "qspis" throughout this code.
	// The master of the QSPI bus is the FT4222H, an FTDI USB-to-QSPI bridge.  Host software, "qspim.exe", 
	// drives data to the MAX78000 via the bridge using a simple protocol.  See the firmware user's
	// guide for details.
	uint32_t dummy_count = 0;
	typedef struct
	{
		process_cb_t 		cb;
		qp_mode_t 			mode;
		const char 			*name;
//		led_state_t         led_state;
	}
	dispatch_t;
	static const dispatch_t mode_dispatch_table[] =
	{
		{ .cb = loopback_cb, .mode = qp_mode_loopback, "qp_mode_loopback"},
	};

	board_init_new();

    printf("\033[H\033[JMAX78000\r\nQSPIS demonstration firmware v1.0_RA\r\n");
    printf("[x] Compiled: %s, %s\n",__DATE__,__TIME__);

    //RED_VALUE =175;

    MXC_ICC_Enable(MXC_ICC0); // Enable cache

    // Switch to 100 MHz clock
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

#ifndef PATTERN_GEN
      initialize_camera();
      MXC_Delay(SEC(2));
#endif

	// stream data from the true random number generator peripheral
	cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);
	//printf("\n*** CNN Inference Test ***\n");
	cnn_init(); // Bring state machine into consistent state
	cnn_load_weights(); // Load kernels
	cnn_load_bias();
	cnn_configure(); // Configure state machine


	while( 1 )
	{
#ifdef USE_SPI
		static uint8_t qspis_out[QSPIS_CBUF_SIZE], qspis_in[QSPIS_CBUF_SIZE];


		qspis_init( &qspis_in, sizeof(qspis_in), &qspis_out, sizeof(qspis_out) ); // initialize the QSPI peripheral and it's in and out circular buffers (cbuf's)

		uint32_t mode = 0xFF;
		process_cb_t cb = NULL;


		while( !qspis_reset() && qspis_read( &mode, sizeof(mode) ) != sizeof(mode) )
		{
			// this will loop until the host has sent a mode set command or a reset signal.
			// reset signals consist of an slave select/deselect cycle that contains no data.
			board_sleep();
		}
		printf("mode is set to %d\n\r",mode);

		if( qspis_reset() )
		 	continue;	// loop if the QSPIS as been reset by the host.
///////////// qp_mode_trng /////////////////////
		if( mode == qp_mode_trng )
		{
		 	// stream data from the true random number generator peripheral
			dummy_count=0;
			printf("In function mode ""trng"": %d  dummy: %d\n",mode, dummy_count);

		 	while( !qspis_reset() )
		 	{
		 		uint32_t *p;
		 		if( qspis_write_aquire( (void **)&p ) >= sizeof(uint32_t) )
		 		{
		 			//MXC_Delay(100); //to test slow rate of transmission
		 			for( int i =0 ; i<32; i++)
		 			{
		 				*p++ = dummy_count++;
		 			}
		 			qspis_write_release( 32*sizeof(uint32_t) );
		 		}
		 		board_sleep();
				LED_Toggle(LED2);
		 	}
		 	continue;
		 }
///////////// qp_mode_streaming /////////////////////
		 if( mode == qp_mode_streaming )
		 {
			 dummy_count=0;

     		//printf("\n*** CNN Inference Test ***\n");
			printf("In function mode ""streaming"" %d\n",mode);
#endif
#ifndef PATTERN_GEN
		  //  camera_sleep(0); // disable sleep
		    camera_start_capture_image();
#endif
#ifdef USE_SPI
		    while( !qspis_reset() )
		    {
#endif
		 		uint32_t *p;

		 		LED_Toggle(LED1);

		 		load_input_camera();
		 		//MXC_Delay(50000);

//		 		dump_cnn();

		 		// start inference
		 		cnn_start(); // Start CNN processing

		 		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // SLEEPDEEP=0
		 		while (cnn_time == 0)
		 			__WFI(); // Wait for CNN

		 		// unloading inference
				// dump_inference();
#ifdef USE_SPI
		 		uint32_t *data_addr = (uint32_t *) 0x50400000;

		 		for (int ch=0; ch < NUM_OUT_CHANNLES; ch +=4)
		 		{
		 		//	printf("Channel: %d\n",ch);
		 			for(int pix=0; pix < 1*NUM_PIXELS; pix+=DATA_BLOCK)
		 			{

		 				//printf("%d\n",buf_avail);
		 				while( qspis_write_aquire( (void **)&p ) < DATA_BLOCK*sizeof(uint32_t)){
		 					 if (qspis_reset())
		 						 goto out_streaming;
		 					 // this is to catch the keyboard interrupt from the host and gracefully
		 					 // terminate the loop.
		 				}

						for (uint32_t n=0; n< DATA_BLOCK; n++)
						{
							//*p++ = dummy_count++;
		 				    *p++ = *(data_addr+pix+n); // putting data in buffer
						}

						qspis_write_release( DATA_BLOCK*sizeof(uint32_t) ); // release the buffer to the QSPI

						//LED_Toggle(LED2);

		 				board_sleep();

		 			}
		 			//console_uart_send_bytes(data_addr, 4*NUM_PIXELS); //4x7744=30976
		 			//frame size in bits: 3,964,928,  495,616 bytes
		 			// data_addr += 0x8000;
		 			//printf("1.  ch=%d,DAddr =0x%08x \n",ch, data_addr);
		 			data_addr += 0x2000;
		 			//printf("2.  ch=%d,DAddr =0x%08x \n",ch, data_addr);
		 			if (	(data_addr == (uint32_t *)0x50420000) ||
		 			   		(data_addr == (uint32_t *)0x50820000) ||
		 					(data_addr == (uint32_t *)0x50c20000))
		 			{
		 			   //data_addr += 0x003e0000;
		 			   	data_addr += 0x000F8000;
		 			   	//printf("3a. ch=%d,DAddr =0x%08x \n",ch, data_addr);
		 			}
		 			//printf("3.  ch=%d,DAddr =0x%08x \n",ch, data_addr);
		 		}
		 	//	printf("unloading finished\n");
#endif
#ifndef PATTERN_GEN
		  //camera_sleep(0); // disable sleep
		  camera_start_capture_image();
#endif

		 	}
#ifdef USE_SPI
		    out_streaming:
		 	 continue;
		 }
///////////// qp_mode_cnn_in /////////////////////
		 if( mode == qp_mode_cnn_in )
		 {
			 dummy_count=0;

     		//printf("\n*** CNN Inference Test ***\n");
			printf("In function mode ""cnn_in"" %d\n",mode);
#endif
#ifndef PATTERN_GEN
		  //  camera_sleep(0); // disable sleep
		    camera_start_capture_image();
#endif
#ifdef USE_SPI
		    while( !qspis_reset() )
		    {
#endif
		 		uint32_t *p;

		 		LED_Toggle(LED1);

		 		load_input_camera();
		 		//MXC_Delay(50000);


		 		uint32_t * data_addr1[12] = { (uint32_t *) 0x50400700, (uint32_t *) 0x50408700, (uint32_t *) 0x50410700,  (uint32_t *) 0x50418700,
		 									 (uint32_t *) 0x50800700, (uint32_t *) 0x50808700, (uint32_t *) 0x50810700,  (uint32_t *) 0x50818700,
		 									 (uint32_t *) 0x50c00700, (uint32_t *) 0x50c08700, (uint32_t *) 0x50c10700,  (uint32_t *) 0x50c18700 };
		 		static uint32_t sum=0;
		 		sum=0;
		 		for (int i=0; i<12; i++)
		 		{
		 			for (int j=0; j<7744; j+= 16)
		 			{
				 		while( qspis_write_aquire( (void **)&p ) < 16*sizeof(uint32_t)){
				 			if (qspis_reset())
				 				goto cnn_in_out;
				 				 // this is to catch the keyboard interrupt from the host and gracefully
				 				 // terminate the loop.
				 		}

		 				//printf("\n%08X: ",data_addr[i]);
		 				for(int k=0; k<16; k++)
		 				{
		 					//printf("%08X ", *data_addr[i]);
		 					//*p++ = dummy_count++;
		 					sum += *data_addr1[i];
		 					*p++ = *data_addr1[i];
		 					data_addr1[i]++;
		 				}
						qspis_write_release( 16*sizeof(uint32_t) ); // release the buffer to the QSPI
		 				board_sleep();
		 			} //for j
		 		} //for i

		 		printf("SUM: %08X \n", sum);

		 		// start inference
//		 		cnn_start(); // Start CNN processing
//
//		 		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // SLEEPDEEP=0
//		 		while (cnn_time == 0)
//		 			__WFI(); // Wait for CNN

		 		// unloading inference
				// dump_inference();

#ifndef PATTERN_GEN
		  //camera_sleep(0); // disable sleep
		  camera_start_capture_image();
#endif

		 	}
#ifdef USE_SPI
		    cnn_in_out:
		 	 continue;
		 }



		for( uint32_t i = 0; i < ARRAY_COUNT( mode_dispatch_table ); i++ )
		{
			if( mode_dispatch_table[i].mode == mode )
			{
				cb = mode_dispatch_table[i].cb;
				DEBUG_PRINTF( ("qp_mode_t = %s\r\n", mode_dispatch_table[i].name) );
	//			set_led_state( mode_dispatch_table[i].led_state );
				while( !qspis_reset() )
				{
					process_stream( cb );
					board_sleep();
				}
                break;
			}
		}
	}
#endif
	return 0;
}

volatile bool debug_break = true;   // prevent compiler from optimizing the return out
									// makes it easier to walk back up the stack during debug
void led_assert( void )
{
	// fatal error catch-all for this firmware
	__disable_irq();
//	board_led( BOARD_LED_BLUE, board_led_on );
//	board_led( BOARD_LED_GREEN, board_led_on );

	while( debug_break )
	{
		DEBUG_PROCESS();
	}
}
