/* *****************************************************************************
 * Copyright (C) 2020 Maxim Integrated Products, Inc., All Rights Reserved.
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
#ifndef __CBUF_H__
#define __CBUF_H__

// cbufs are circular buffers.  Data is written by a "writer", and subsequently read
// by a "reader" on a first-in-first-out basis.

// cbufs can be used to marshal data between execution contexts such as between the main
// thread and ISR-based device drivers. Another example would be between different tasks in an RTOS.

// cbuf_init() is called to define a circular buffer.  The memory used to describe the buffer
// must remain valid throughout the lifetime of the cbuf.

// Access to the cbuf is bi-directional;  there is a writer and a reader each of which can be
// in a different execution context.  cbuf's manage synchronization via CPU supported atomic operations and
// therefore do not require external synchronization like temporarily disabling interrupts.  Only one
// execution context is allowed in each direction--one for writing, and one for reading.  If
// your design requires multiple readers/writers, then you must implment appropriate synchronization.

// cbuf_read_aquire() is used to acquire continuous space from a cbuf for writing dat  Data is subsequently
// written by the caller which then calls cbuf_read_release() to indicate that the data is written.

// cbuf_write_aquire() and cbuf_write_release() operate similarly in the converse direction.

// cbufs access functions are mostly implemented as inline functions due to the environment were they
// are usually used--interrupt service routines where latency must be minimized.

typedef uint8_t  cbuf_data_t;
typedef uint32_t cbuf_size_t;

typedef struct
{
    cbuf_data_t *           	buffer;     	// pointer to buffer
    cbuf_size_t             	size;       	// size of buffer in bytes.  A size of zero indicates that the
												// buffer is 2^N bytes long, where N = sizeof(cbuf_size_t)*8
	cbuf_size_t					write_ndx;
	cbuf_size_t					read_ndx;
	volatile cbuf_size_t		free;

}
cbuf_t;

typedef struct
{
   // 'dbuf' is a double-buffer.
   // used to describe a circular buffer as a pair of data
   // pointers with corresponding sizes
    void *      	buffer[2];
    cbuf_size_t   	size[2];
}
cbuf_dbuf_t;


#ifndef CBUF_MEMCPY
#define CBUF_MEMCPY cbuf_memcpy
#endif

#ifndef CBUF_INLINE
#define CBUF_INLINE static inline
#endif

#ifndef CBUF_ASSERT
#define CBUF_ASSERT(x)
#endif

#ifndef CBUF_READ_ATOMIC
// use GCC built-in to handle read-modify-write on cbuf_t.free
// define if you need a different sychronization mechanism
#define CBUF_READ_ATOMIC(p,a) __atomic_add_fetch( p, a, __ATOMIC_RELAXED );
#endif


#ifndef CBUF_WRITE_ATOMIC
#define CBUF_WRITE_ATOMIC(p,a) __atomic_sub_fetch( p, a, __ATOMIC_RELAXED );
#endif

cbuf_size_t cbuf_read_to_reg8( cbuf_t *p_cbuf, volatile void *pv_fifo, cbuf_size_t size );
cbuf_size_t cbuf_write_from_reg8( cbuf_t *p_cbuf, volatile void *pv_fifo, cbuf_size_t size );
cbuf_size_t cbuf_read( cbuf_t *p_cbuf, void *pv_dst, cbuf_size_t size );
cbuf_size_t cbuf_write( cbuf_t *p_cbuf, const void *pv_src, cbuf_size_t size );

CBUF_INLINE cbuf_size_t _cbuf_delta( cbuf_size_t a, cbuf_size_t b, cbuf_size_t size )
{
	// cbuf private
	// returns the number of data bytes available for reading from the cbuf
	cbuf_size_t d;
	if( b >= a )
	{
		d = size + a - b;
	}
	else
	{
		d = a - b;
	}
	return d;
}

CBUF_INLINE cbuf_size_t _cbuf_min( cbuf_size_t a, cbuf_size_t b )
{
	// cbuf private 
	// minimum function
	
	cbuf_size_t min;
	if( a > b )
	{
		min = b;
	}
	else
	{
		min = a;
	}
	return min;
}

CBUF_INLINE cbuf_size_t _cbuf_add( cbuf_size_t offset, cbuf_size_t add, cbuf_size_t size )
{
	// cbuf private 
	// cbuf index addition including wrap-around
	cbuf_size_t result = offset + add;
	if(  result >= size )
	{
		result -= size;
	}
	return result;
}

CBUF_INLINE cbuf_size_t cbuf_used( cbuf_t *p_cbuf )
{
    // returns the number of bytes available for reading
    return p_cbuf->size - p_cbuf->free;
}

CBUF_INLINE bool cbuf_empty( cbuf_t *p_cbuf )
{
    // return's true if the cbuf is empty
    return cbuf_used(p_cbuf) ? false : true;
}

CBUF_INLINE cbuf_size_t cbuf_free( cbuf_t *p_cbuf )
{
    // returns the number of bytes available for writing
    return p_cbuf->free;
}

CBUF_INLINE bool cbuf_full( cbuf_t *p_cbuf )
{
    // return's true if the cbuf is full
    return cbuf_free(p_cbuf) ? false : true;
}

CBUF_INLINE cbuf_size_t cbuf_write_aquire( cbuf_t *p_cbuf, void **ppv )
{
    // aquire a pointer, *ppv, to writable space in the cbuf.  returns the size of space.
    // will not necessarily return all of the space avaiable--only returns the next
    // contiguous space.  Returns zero if no space available.

	cbuf_size_t size = p_cbuf->size;

    cbuf_size_t write_ndx = p_cbuf->write_ndx;
	cbuf_size_t free = cbuf_free( p_cbuf );

	cbuf_size_t linear = size - write_ndx;

    *ppv = (void *)&p_cbuf->buffer[write_ndx];
    return _cbuf_min( free, linear );
}

CBUF_INLINE void cbuf_write_release( cbuf_t *p_cbuf, cbuf_size_t release_size )
{
    // release 'size' bytes to the cbuf which become available for subsequent cbuf_read_aquire() calls.

	cbuf_size_t size = p_cbuf->size;
  
    CBUF_ASSERT( cbuf_free( p_cbuf ) >= release_size );

	CBUF_WRITE_ATOMIC( &p_cbuf->free, release_size );

	p_cbuf->write_ndx = _cbuf_add( p_cbuf->write_ndx, release_size, size );
}

CBUF_INLINE void cbuf_write_release_async( cbuf_t *p_cbuf, cbuf_size_t release_size )
{
    // release 'size' bytes to the cbuf which become available for subsequent cbuf_read_aquire() calls.
    // this function does not sychronize p_cbuf->free access
    
	cbuf_size_t size = p_cbuf->size;
  
    CBUF_ASSERT( cbuf_free( p_cbuf ) >= release_size );

	p_cbuf->free -= release_size;

	p_cbuf->write_ndx = _cbuf_add( p_cbuf->write_ndx, release_size, size );
}

CBUF_INLINE cbuf_size_t cbuf_read_aquire( cbuf_t *p_cbuf, void **ppv )
{
    // acquire a pointer, *ppv, to readable data in the cbuf.  returns the size of data block.
    // will not necessarily return all of the data available--only returns the next
    // contiguous data block.  Returns zero if no data is available.

	cbuf_size_t size = p_cbuf->size;

    cbuf_size_t read_ndx = p_cbuf->read_ndx;

	cbuf_size_t used = cbuf_used( p_cbuf );
	cbuf_size_t linear = size - read_ndx;

	*ppv = (void *)&p_cbuf->buffer[read_ndx];
    return _cbuf_min( used, linear );
}

CBUF_INLINE void cbuf_reset( cbuf_t *p_cbuf )
{
    // used to reset an empty cbuf to the beginning of its buffer.
    // useful for using cbuf's to 'defragment' streams from other cbufs.
    p_cbuf->read_ndx = p_cbuf->write_ndx = 0;
	p_cbuf->free = p_cbuf->size;
}


CBUF_INLINE void cbuf_read_release( cbuf_t *p_cbuf, cbuf_size_t release_size )
{
    // return 'size' bytes back to the cbuf which become available for subsequent cbuf_write_aquire() calls.
    
	cbuf_size_t size = p_cbuf->size;
	    
    CBUF_ASSERT( cbuf_used( p_cbuf ) >= release_size );

	CBUF_READ_ATOMIC( &p_cbuf->free, release_size );

	p_cbuf->read_ndx = _cbuf_add( p_cbuf->read_ndx, release_size, size );
}

CBUF_INLINE void cbuf_read_release_async( cbuf_t *p_cbuf, cbuf_size_t release_size )
{
    // return 'size' bytes back to the cbuf which become available for subsequent cbuf_write_aquire() calls.
    // this function does not sychronize p_cbuf->free access

	cbuf_size_t size = p_cbuf->size;
	    
    CBUF_ASSERT( cbuf_used( p_cbuf ) >= release_size );

	p_cbuf->free += release_size;
	p_cbuf->read_ndx = _cbuf_add( p_cbuf->read_ndx, release_size, size );
}

CBUF_INLINE cbuf_size_t cbuf_read_aquire_dbuf( cbuf_t *p_cbuf, cbuf_dbuf_t *p_dbuf )
{
	// returns a double buffer which describes the cbuf for the purpose of reading.
	// p_dbuf->size[..] an be zero.
	// return value is the total number of bytes acquired.
	// useful for logic that can accept multiple buffers of data at once.

	cbuf_size_t size = p_cbuf->size;

	cbuf_size_t read_ndx = p_cbuf->read_ndx;

	cbuf_size_t used = cbuf_used( p_cbuf );

	cbuf_size_t linear = size - read_ndx;

	p_dbuf->size[0] = _cbuf_min( used, linear );
	p_dbuf->buffer[0] = (void *)&p_cbuf->buffer[read_ndx];
	p_dbuf->buffer[1] = p_cbuf->buffer;
	p_dbuf->size[1] = used - p_dbuf->size[0];
	return p_dbuf->size[0] + p_dbuf->size[1];
}

CBUF_INLINE cbuf_size_t cbuf_write_aquire_dbuf( cbuf_t *p_cbuf, cbuf_dbuf_t *p_dbuf )
{
	// returns a double buffer which describes the cbuf for the purpose of writing
	// p_dbuf->size[..] an be zero.
	// return value is the total number of bytes acquired.
	// useful for logic that can accept multiple buffers of data at once.

	cbuf_size_t size = p_cbuf->size;

	cbuf_size_t write_ndx = p_cbuf->write_ndx;

	cbuf_size_t free = cbuf_free( p_cbuf );
	cbuf_size_t linear = size - write_ndx;

	p_dbuf->size[0] = _cbuf_min( free, linear );
    p_dbuf->buffer[0] = (void *)&p_cbuf->buffer[write_ndx];
	p_dbuf->buffer[1] = p_cbuf->buffer;
	p_dbuf->size[1] = free - p_dbuf->size[0];
    return p_dbuf->size[0] + p_dbuf->size[1];
}

CBUF_INLINE void cbuf_init( cbuf_t *p_cbuf, void *pv_buffer, cbuf_size_t size )
{
    // initialize a cbuf_t struct with the given buffer and buffer size
    p_cbuf->size = size;
    p_cbuf->buffer = (uint8_t *)pv_buffer;
	cbuf_reset(p_cbuf);
}

CBUF_INLINE cbuf_size_t cbuf_write_string( cbuf_t *p_cbuf, const char *p_str )
{
    // writes an ASCIIZ to the cbuf
    cbuf_size_t len = 0;
    if( p_str )
    {
        while( p_str[len] ) len++;
        len = cbuf_write( p_cbuf, p_str, len );
    }
    return len;
}

CBUF_INLINE cbuf_size_t cbuf_write_byte( cbuf_t *p_cbuf, uint8_t byte )
{
    // writes a single byte to a cbuf
    return cbuf_write( p_cbuf, &byte, sizeof(byte) );
}

CBUF_INLINE void cbuf_memcpy( void *pv_dst, const void *pv_src, cbuf_size_t size )
{
    // 32-bit oriented version of memcpy() which can be inlined for enhanced performance
    // for smaller sizes typical of cbuf implementations on 32-bit cortex MCU's

    cbuf_size_t i = 0;
    cbuf_data_t * p_dst = (cbuf_data_t*)pv_dst;
    cbuf_data_t * p_src = (cbuf_data_t*)pv_src;
    uint32_t * p_wdst = (uint32_t*)pv_dst;
    uint32_t * p_wsrc = (uint32_t*)pv_src;

    cbuf_size_t word_count = size / sizeof(cbuf_size_t);
    for(i=0;i<word_count;i++)
    {
        p_wdst[i] = p_wsrc[i];
    }
    i*=sizeof(cbuf_size_t);
    for( ; i < size; i++ )
    {
        p_dst[i] = p_src[i];
    }
}

#endif // __CBUF_H__
