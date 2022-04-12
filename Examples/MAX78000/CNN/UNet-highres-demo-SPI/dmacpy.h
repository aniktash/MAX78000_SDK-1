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

#ifndef __DMACPY_H_INCL__
#define __DMACPY_H_INCL__

#include "dma_regs.h"

#ifndef DMACPY_CHANNEL
#error DMACPY_CHANNEL must be defined
#endif

static inline void dmacpy( void * dst, const void * src, size_t size )
{
	// uses the DMA controller to do memcpy's.  faster and doesn't disrupt CPU cache.
	// assumes a single execution context.  If multiple, you must add synchronization externally.
	// The DMA controller can't be used to transfer memory to or from the flash.
	__IO mxc_dma_ch_regs_t * const r = &MXC_DMA->ch[DMACPY_CHANNEL];
    if( size )
    {
        r->cnt = size;
        r->dst = (uint32_t)dst;
        r->src = (uint32_t)src;
        r->ctrl = MXC_F_DMA_CTRL_EN |
        		  MXC_S_DMA_CTRL_SRCWD_WORD | MXC_S_DMA_CTRL_DSTWD_WORD |
				  ((4-1) << MXC_F_DMA_CTRL_BURST_SIZE_POS ) |  MXC_S_DMA_CTRL_REQUEST_MEMTOMEM 	|
				  MXC_F_DMA_CTRL_SRCINC | MXC_F_DMA_CTRL_DSTINC;
        while( !(r->status & MXC_F_DMA_STATUS_CTZ_IF) ); // this can spin indefinitely if the DMA emits an error.
        										 // this is usually the result of an invalid dst or src.
        r->status = ~0;
        r->ctrl = 0;
    }
}


#endif
