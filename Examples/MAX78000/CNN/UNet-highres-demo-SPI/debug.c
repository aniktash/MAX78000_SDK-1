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

#include "debug.h"
#ifdef DEBUG_ENABLE

#include "cbuf.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

// The following def's can be overridden by defining them in debug_config.h

#ifndef DEBUG_FORMAT_BUFFER_SIZE
#define DEBUG_FORMAT_BUFFER_SIZE 128	// size of the buffer that receives a printf result
#endif

#ifndef DEBUG_VARG_SIZE
#define DEBUG_VARG_SIZE (4*sizeof(uint32_t))    // can handle up to 4 32-bit vargs (printf ... params)
#endif

#ifndef DEBUG_QUEUE_SIZE
#define DEBUG_QUEUE_SIZE (DEBUG_VARG_SIZE*32)	// maximum number of printf's that can be queued
#endif

#ifndef DEBUG_LOCK
#define DEBUG_LOCK(x)
#warning DEBUG_* facility is asynchrounous
// This is OK if DEBUG_PRINTF() is not called from multiple execution contexts (like from main() and and ISR).
// If you want to use DEBUG_PRINTF() from an multiple execution contexts, then you must provide syncrhonization
// functions.  For example, to support DEBUG_PRINTF() from main() and from an ISR on an CMSIS supported processor: 
// #define DEBUG_LOCK(x) __disable_irq()
// #define DEBUG_LOCK(x) __enable_irq()
// Note that disabling interrupts will likely cause the qspim to fail.
#endif

#ifndef DEBUG_UNLOCK
#define DEBUG_UNLOCK(x)
#endif

#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT(x) { DEBUG_LOCK(); if(!(x)) { while(1); } }
#endif

static cbuf_t s_cbuf_queue;
static cbuf_t s_indexer;

void debug_init( void )
{
    static char fmtbuf[DEBUG_FORMAT_BUFFER_SIZE];
    static char queuebuf[DEBUG_QUEUE_SIZE];

    cbuf_init( &s_cbuf_queue, queuebuf, sizeof(queuebuf) );
    cbuf_init( &s_indexer, fmtbuf, sizeof(fmtbuf) );
}

void debug_printf( const char * p_format, ... )
{
	// Should not be called from an ISR unless DEBUG_LOCK() and DEBUG_UNLOCK() are defined to provide synchronization
    uint32_t i;
    uint32_t * p_dst;
    uint32_t * p_src = (uint32_t*)&p_format;
    DEBUG_LOCK();
    if( cbuf_write_aquire( &s_cbuf_queue, (void**)&p_dst ) )
    {
        for(i=0;i<DEBUG_VARG_SIZE/sizeof(uint32_t);i++)
            p_dst[i] = p_src[i];
        cbuf_write_release(&s_cbuf_queue, DEBUG_VARG_SIZE);
    }
    else
    {
		DEBUG_ASSERT(0); // cbuf out of space.  increase DEBUG_CBUF_BUFFER_SIZE
    }
    DEBUG_UNLOCK();

}

void debug_process( void )
{
    if( cbuf_empty(&s_indexer) )
    {
        void *p;
        cbuf_reset(&s_indexer);
        if( cbuf_read_aquire( &s_cbuf_queue, &p ) )
        {
            const char * p_format = *(const char **)p;
            p += sizeof(const char*);
            va_list l;
            l.__ap = p;
            void * fmtbuf;
            uint32_t aquired = cbuf_write_aquire( &s_indexer, &fmtbuf );
            int size = vsnprintf( fmtbuf, aquired, p_format, l );
            DEBUG_ASSERT( size > 0 && aquired > size ); // increase DEBUG_FORMAT_BUFFER_SIZE or fix format string
            cbuf_write_release_async( &s_indexer, size+1 );
            cbuf_read_release( &s_cbuf_queue, DEBUG_VARG_SIZE );
        }
    }
    if( !cbuf_empty(&s_indexer) )
    {
		void *p;
		uint32_t size = cbuf_read_aquire( &s_indexer, &p );
        size = DEBUG_OUTPUT( p, size );
		cbuf_read_release_async( &s_indexer, size );
    }
}

#endif
