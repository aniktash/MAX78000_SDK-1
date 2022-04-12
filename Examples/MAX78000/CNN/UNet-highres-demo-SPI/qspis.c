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
#include "boardnew.h"
#include "debug.h"
#include "qp.h"
#include "cbuf.h"
#include "spi.h"
#include "dma.h"

// The code in this module is always compiled with maximum optimaztion "-O3".  This is necessary to ensure acceptable ISR latency
// relative to the timing constraints of the FTDI FT4222 QSPI/USB bridge.  Breakpoints or extra code in these routines can cause
// QSPI transfer failures.

#define DMA_XFER_SIZE       4       // Minimum number of bytes to be transferred between the QSPI and memory via the DMA engine.
									// A 32-bit granularity is assumed throughout the firmware and the host application (qspim.exe).
									// No other values have been tested and a different value here could break logic.

// Pointers to the DMA channel registers allocated to the QSPI in board.h
__IO mxc_dma_ch_regs_t * const DMA_OUT = (__IO mxc_dma_ch_regs_t *)(MXC_BASE_DMA + MXC_R_DMA_CH + (BOARD_DMA_CHANNEL_QSPI_OUT * sizeof(mxc_dma_ch_regs_t)));
__IO mxc_dma_ch_regs_t * const DMA_IN  = (__IO mxc_dma_ch_regs_t *)(MXC_BASE_DMA + MXC_R_DMA_CH + (BOARD_DMA_CHANNEL_QSPI_IN * sizeof(mxc_dma_ch_regs_t)));


static cbuf_t		s_cbuf_in;  	// handles communication from the master and to the main thread
static cbuf_t		s_cbuf_out; 	// handles communication from the main thread and to the master
static uint32_t 	s_out_cnt;		// value programmed into DMA_OUT->cnt before the transfer commenced
static qp_flow_t 	s_flow;			// handles flow control logic
static bool 		s_ready;		// high-level state of the QSPIS module


void qspis_init( void *in_buf, uint32_t in_size, void *out_buf, uint32_t out_size )
{
	// initialize the QSPI driver


	s_ready = false;

	cbuf_init( &s_cbuf_in, in_buf, in_size );
	cbuf_init( &s_cbuf_out, out_buf, out_size );

	// enable peripherals used by the QSPI driver
	MXC_GCR->pclkdis1 &= ~(MXC_F_GCR_PCLKDIS1_SPI0);
	MXC_GCR->pclkdis0 &= ~(MXC_F_GCR_PCLKDIS0_DMA);

	// QSPI_MSTR_CNTL
	MXC_SPI0->ctrl0 = MXC_F_SPI_CTRL0_EN;
	// QSPI_STATIC_CONFIG
	MXC_SPI0->ctrl2 = (QP_SPI_MODE_POLARITY << MXC_F_SPI_CTRL2_CLKPOL_POS) |
					  (QP_SPI_MODE_PHASE << MXC_F_SPI_CTRL2_CLKPHA_POS) |
					  (MXC_S_SPI_CTRL2_DATA_WIDTH_QUAD) |
					  (8 << MXC_F_SPI_CTRL2_NUMBITS_POS);

	MXC_SPI0->dma  = ((DMA_XFER_SIZE - 1) << MXC_F_SPI_DMA_RX_THD_VAL_POS) |
					  MXC_F_SPI_DMA_RX_FIFO_EN;

	MXC_SPI0->intfl = MXC_SPI0->intfl;

	MXC_SPI0->inten = MXC_F_SPI_INTFL_ABORT | MXC_F_SPI_INTFL_RX_OV |
					  MXC_F_SPI_INTFL_TX_OV | MXC_F_SPI_INTFL_FAULT |
					  MXC_F_SPI_INTFL_RX_THD |MXC_F_SPI_INTFL_SSD;

	MXC_DMA->inten |= (1 << BOARD_DMA_CHANNEL_QSPI_IN);

//	NVIC_SetPriority(SPI0_IRQn,0x20);
	NVIC_EnableIRQ( SPI0_IRQn );
	NVIC_EnableIRQ( BOARD_DMA_CHANNEL_QSPI_IN_IRQ );
	s_ready = true;
//	printf("init: \n");
//	print_SPI();


}

static inline uint32_t dma_out_count( void )
{
	// returns the number of bytes transferred from the qspis out cbuf to the master during
	// the last DMA operation

	uint32_t dma_xfer_count;
	uint32_t tx_fifo_count = (MXC_SPI0->dma & MXC_F_SPI_DMA_TX_LVL) >> MXC_F_SPI_DMA_TX_LVL_POS; // count of bytes unsent still in the TX FIFO

	if( DMA_OUT->status & MXC_F_DMA_STATUS_RLD_IF )
	{
        uint32_t rld = DMA_OUT->cntrld & MXC_F_DMA_CNTRLD_CNT;
		dma_xfer_count = s_out_cnt + rld - DMA_OUT->cnt;
	}
	else
	{
		dma_xfer_count = s_out_cnt - DMA_OUT->cnt;
	}
	return dma_xfer_count - tx_fifo_count;
}

// static inline void dump_out_dma( void )
// {
// 	DEBUG_PRINTF(("DMA_OUT(src=%8.8X,cnt=%d,src_rld=%8.8X,cnt_rld=%d,st=%8.8X)\r\n", DMA_OUT->src, DMA_OUT->cnt, DMA_OUT->src_rld, DMA_OUT->cnt_rld & MXC_F_DMA_CNT_RLD_CNT_RLD, DMA_OUT->st ));
// }
// static inline void dump_in_dma( void )
// {
// 	DEBUG_PRINTF(("DMA_IN(dst=%8.8X,cnt=%d,dst_rld=%8.8X,cnt_rld=%d,r=%d)\r\n", DMA_IN->dst, DMA_IN->cnt, DMA_IN->dst_rld, DMA_IN->cnt_rld, DMA_IN->cnt_rld & MXC_F_DMA_CNT_RLD_RLDEN ? 1 : 0));
// }

void BOARD_DMA_CHANNEL_QSPI_IN_ISR( void )
{
	// end-of-transfer inbound (master writing to qspis) DMA interrupt.
	// At this point the QSPI bus must be "turned around", that is, changed from reading data to writing data.
	// This is a time-critical operation and generally must occur within 2-3us.  The timing of the FT4222 drives this.
	// The design of the qspis module is such that a minimum amount of code is required to turn the bus around during this ISR.
	// Much of the setup-work has already been done during the slave select (SSA) interrupt.
	// This code must be kept efficient.  If the bus isn't turned around in time, the qspi transfer will fail unrecoverably.
	// The board_test1_*() functions facilitate debugging the QSPI bus with a logic analyzer to verify proper timing.

	MXC_SPI0->dma = MXC_F_SPI_DMA_TX_FIFO_EN |
					((MXC_SPI_FIFO_DEPTH - DMA_XFER_SIZE + 1) << MXC_F_SPI_DMA_TX_THD_VAL_POS); // enable the TX FIFO

	MXC_SPI0->fifo32 = *((uint32_t *)&s_flow); // output protocol information.
//    board_test1_set();
//	MXC_Delay (1);
//	LED_On(LED2);
	DMA_IN->status = ~0;
	// The out phase of the QSPI transaction starts here
	// The following register assignment triggers the outgoing DMA transfer
	MXC_SPI0->dma = MXC_F_SPI_DMA_TX_FIFO_EN | MXC_F_SPI_DMA_DMA_TX_EN |
					((MXC_SPI_FIFO_DEPTH - DMA_XFER_SIZE + 1) << MXC_F_SPI_DMA_TX_THD_VAL_POS);

	//    board_test1_clear();
	read_cycle = true;
//	LED_Off(LED2);
}

void SPI0_IRQHandler(void)
{
//    MXC_SPI_AsyncHandler(SPI);
	// This ISR is not as time-critical as BOARD_DMA_CHANNEL_QSPI_IN_ISR(), but the code should be kept efficient.  It is still
	// easy to push execution out to the point that BOARD_DMA_CHANNEL_QSPI_IN_ISR() fails to execute in time.
    static bool data_present;
	static uint32_t in_count;
//	printf("SPI_handler: IN!\n");
	uint32_t fl = MXC_SPI0->intfl & MXC_SPI0->inten;
	if( fl & MXC_F_SPI_INTFL_RX_THD )
	{
		// The QSPI RX FIFO has received 1 32-bit word of data which contains protocol information.
        data_present = true;
//        board_test1_set(); // set test1 pin for observation with a logic analyzer

		in_count = MXC_SPI0->fifo32; // The host (qspim.exe) has sent the number of bytes it will send in this write (always 4-byte granularity)
		// This logic assumes the host knows how much it can write via the flow control mechanism.  A bug on the host side, can
		// crash the slave.  Clamping logic could be added here, but latency constraints are very tight.
		if( in_count )
		{
			uint32_t cnt = DMA_IN->cnt;
			// Finalize the configuration of the inbound DMA channel to read the upcoming data written by the host into the input cbuf.
			// This is phase 2 of 2 of DMA_IN configuration.  Phase 1 occurs in qspis_ss0()
			if( in_count <= cnt )
			{
				// the DMA reload feature is not required (this is the typical case).
				DMA_IN->cnt = in_count;
				DMA_IN->ctrl = MXC_F_DMA_CTRL_EN | MXC_S_DMA_CTRL_SRCWD_WORD |
						MXC_S_DMA_CTRL_DSTWD_WORD | ((DMA_XFER_SIZE - 1) << MXC_F_DMA_CTRL_BURST_SIZE_POS) |
						MXC_F_DMA_CTRL_DIS_IE | MXC_S_DMA_CTRL_REQUEST_SPI0RX | MXC_F_DMA_CTRL_DSTINC;
			}
			else
			{
				// The DMA reload feature is required to "wrap around" the cbuf (rare, depending on the size of the cbuf and incomming packet size).
				DMA_IN->cntrld = in_count - cnt;
				DMA_IN->ctrl = MXC_F_DMA_CTRL_EN | MXC_S_DMA_CTRL_SRCWD_WORD |
						MXC_S_DMA_CTRL_DSTWD_WORD | ((DMA_XFER_SIZE - 1) << MXC_F_DMA_CTRL_BURST_SIZE_POS) |
						MXC_F_DMA_CTRL_DIS_IE | MXC_S_DMA_CTRL_REQUEST_SPI0RX | MXC_F_DMA_CTRL_DSTINC |
						MXC_F_DMA_CTRL_RLDEN;
			}
			// The following register write will trigger BOARD_DMA_CHANNEL_QSPI_IN_ISR() upon DMA transfer completion
			MXC_SPI0->dma = MXC_F_SPI_DMA_RX_FIFO_EN | MXC_F_SPI_DMA_DMA_RX_EN |
							((DMA_XFER_SIZE - 1) << MXC_F_SPI_DMA_RX_THD_VAL_POS);
			s_flow.writeable -= in_count;
		}
		else
		{
			// The host had no data to send so immediately transition to out phase
			BOARD_DMA_CHANNEL_QSPI_IN_ISR();
		}
//        board_test1_clear();

		MXC_CLRBIT( &MXC_SPI0->inten, MXC_F_SPI_INTEN_RX_THD_POS ); // disable this interrupt
	}
	if( (fl & MXC_F_SPI_INTFL_SSD) && !read_cycle )
	{
//		LED_On(LED1);
		// The master has de-asserted the SS_QSPI signal (see schematic) and thereby ended the QSPI write/read.
        if( data_present )
        {
        	static bool temp=false;
        	static uint32_t tmpval = 0;
            data_present = false;
            if( in_count )
            {
//        		LED_On(LED1);
            	// release data received from the master into the input cbuf.  This will make it available to the main thread
				// via cbuf_read_aquire().
        		tmpval = cbuf_free(  &s_cbuf_in );
				temp = (tmpval >= in_count );
            	//ASSERT( cbuf_free(  &s_cbuf_in ) >= in_count );
				if( !temp )
					printf("\nError deassert in_count = 0x%08x, 0x%08x, 0x%08x\n\n", in_count,tmpval,MXC_SPI0->intfl);
				ASSERT(temp);
                cbuf_write_release_async( &s_cbuf_in, in_count );
            }
            uint32_t sent = dma_out_count();
            if( sent )
            {
				// release data transmitted to the master from the output cbuf.  This will free up space in the cbuf
				// for more data from the main thread via cbuf_write_aquire()
                cbuf_read_release_async( &s_cbuf_out, sent );
				s_out_cnt = 0;
            }
            MXC_SETBIT( &MXC_SPI0->intfl, MXC_F_SPI_INTFL_RX_THD_POS );
            MXC_SETBIT( &MXC_SPI0->inten, MXC_F_SPI_INTEN_RX_THD_POS );
			// The QSPI TX FIFO will likely have data remaining in it; we flush both FIFO's, but flushing RX shouldn't actually be necessary.
            MXC_SPI0->dma = MXC_F_SPI_DMA_TX_FLUSH | MXC_F_SPI_DMA_RX_FLUSH;
			// prepare for the next write/read QSPI cycle from the host.
            MXC_SPI0->dma = ((DMA_XFER_SIZE - 1) << MXC_F_SPI_DMA_RX_THD_VAL_POS) |
            		         MXC_F_SPI_DMA_RX_FIFO_EN;
            in_count = 0;
            DMA_OUT->status = ~0;
            DMA_OUT->ctrl = 0;
        }
		else
		{
			// If no data was transferred, then the host is signaling that the cbuf's should be flushed and the protocol reinitialized.
			// This is handled by main thread logic.  The host will perform a reset before setting an operational mode and
			// when the host is closed.
			MXC_SPI0->ctrl0 = DMA_IN->ctrl = DMA_OUT->ctrl = 0;
			DMA_IN->status = DMA_OUT->status = ~0;
			MXC_SPI0->intfl = MXC_SPI0->intfl; //clearing interrupt flag.
			s_ready = false;
		}
	}
	// The following assert is a catch-all for unusual fatal error conditions.
	// This can be triggered if inappropriate breakpoints are set in critical routines in this module.
	//ASSERT( !(fl & (MXC_F_SPI_INTFL_ABORT | MXC_F_SPI_INTFL_RX_OV | MXC_F_SPI_INTFL_TX_OV | MXC_F_SPI_INTFL_FAULT)) )
	if (fl & (MXC_F_SPI_INTFL_ABORT | MXC_F_SPI_INTFL_RX_OV | MXC_F_SPI_INTFL_TX_OV | MXC_F_SPI_INTFL_FAULT))
	{
		printf("error SPI fatal: %08x\n\n",fl);
		while(1);
	}
//	MXC_SPI0->intfl = fl;
	MXC_SPI0->intfl = MXC_SPI0->intfl;
//	printf("SPI_handler: OUT!\n");
//	print_SPI();
//	LED_Off(LED1);
}

void qspis_ss0( void )
{
	// This is and ISR which called by board_ logic when the master asserts SS_QPSI (see schematic).

	if( !s_ready )
		return;	// should never occur

	// This is phase 1 of 2 configuration phases.  The second phase occurs in SPI0_IRQHandler() when the first 32-bit word has been received
	// from the host.

	// The time between the assertion of SS_QSPI and the point at which the bus must be turned around is relatively long so
	// this is the best time to perform as much of the DMA configuration as possible.

	// Two DMA channels are not strictly necessary as only one DMA transfer is occurring at any given time (the QSPI bus is half-duplex).
	// The reason that two channels are used is to minimize the instruction execution time during BOARD_DMA_CHANNEL_QSPI_IN_ISR().
	// In effect, we're using DMA_OUT channel as a caching mechanism.

	// Setup the DMA channels according to the current state of the respective cbufs.
	//
	// "dbuf's" are double-buffers.  They facilitate accessing a cbuf (circular buffer) such that potentially two pointers/sizes are made available.
	// Two pointers/sizes are usually necessary to describe memory in a cbuf due to the fact that circular buffers can "wrap" back-to-front.
	// Fortunately, the MAX78000's DMA controller accommodates the concept of dbuf's very well via the reload mechanism.
//	LED_On(LED1);

//    board_test1_set();
	cbuf_dbuf_t dbuf;

	cbuf_write_aquire_dbuf( &s_cbuf_in, &dbuf );
	DMA_IN->dst = (uint32_t)dbuf.buffer[0];
	DMA_IN->cnt = (uint32_t)dbuf.size[0];
	// Notice that we don't use dbuf.size[1].  This is calculated later during the read in SPI0_IRQHandler()
	// and relies on the host to honor the flow control protocol.
	DMA_IN->dstrld = (uint32_t)dbuf.buffer[1];

	cbuf_read_aquire_dbuf( &s_cbuf_out, &dbuf );
	DMA_OUT->src = (uint32_t)dbuf.buffer[0];
	DMA_OUT->cnt = s_out_cnt = (uint32_t)dbuf.size[0];
	DMA_OUT->cntrld = (uint32_t)dbuf.size[1];
	DMA_OUT->srcrld = (uint32_t)dbuf.buffer[1];

	if( dbuf.size[1] )
	{
		// The DMA reload feature is required.  This is the usual case.
        DMA_OUT->ctrl = MXC_F_DMA_CTRL_EN | MXC_S_DMA_CTRL_SRCWD_WORD |
        				MXC_S_DMA_CTRL_DSTWD_WORD | ((DMA_XFER_SIZE - 1) << MXC_F_DMA_CTRL_BURST_SIZE_POS) |
						MXC_F_DMA_CTRL_DIS_IE | MXC_S_DMA_CTRL_REQUEST_SPI0TX | MXC_F_DMA_CTRL_SRCINC | MXC_F_DMA_CTRL_RLDEN;
	}
    else
	{
		// The DMA reload feature is not required.
        DMA_OUT->ctrl = MXC_F_DMA_CTRL_EN | MXC_S_DMA_CTRL_SRCWD_WORD |
        				MXC_S_DMA_CTRL_DSTWD_WORD | ((DMA_XFER_SIZE - 1) << MXC_F_DMA_CTRL_BURST_SIZE_POS) |
						MXC_F_DMA_CTRL_DIS_IE| MXC_S_DMA_CTRL_REQUEST_SPI0TX | MXC_F_DMA_CTRL_SRCINC;
	}

	// configure the slave to host flow control protocol state
	s_flow.writeable = cbuf_free( &s_cbuf_in );
	s_flow.readable = cbuf_used( &s_cbuf_out );
//	printf("SS0init: \n");
//	print_SPI();
//	MXC_Delay (1);
//	LED_Off(LED1);
//     board_test1_clear();
}


cbuf_size_t qspis_read( void *dst, cbuf_size_t size )
{
	// called by the main thread to copy data that the master has written to the slave
	size = cbuf_read( &s_cbuf_in, dst, size );
	return size;
}

cbuf_size_t qspis_read_aquire( void **dst )
{
	// called by the main thread to access data (in place) that the master has written to the slave
	cbuf_size_t size = cbuf_read_aquire( &s_cbuf_in, dst );
    return size;
}

cbuf_size_t qspis_write_aquire( void **dst )
{
	// called by the main thread to allocate space for data that will be read out by the master
    cbuf_size_t size = cbuf_write_aquire( &s_cbuf_out, dst );
    return size;
}

void qspis_read_release( cbuf_size_t size )
{
	// called by the main thread to indicate that data previously allocated by qspis_write_aquire() is 
	// no longer needed.
	if( size )
	{
		ASSERT( cbuf_used( &s_cbuf_in ) >= size );
		cbuf_read_release( &s_cbuf_in, size );
	}
}

void qspis_write_release( uint32_t size )
{
	// called by the main thread to commit data previously allocated by qspis_write_aquire() to be read out by the master.
	if( size )
	{
		ASSERT( cbuf_free( &s_cbuf_out ) >= size );
		cbuf_write_release( &s_cbuf_out, size );
	}
}

bool qspis_reset( void )
{
	// returns true if the QSPIS logic is in a non-ready state and should be initialized by calling qspis_init()
	return !s_ready;
}

