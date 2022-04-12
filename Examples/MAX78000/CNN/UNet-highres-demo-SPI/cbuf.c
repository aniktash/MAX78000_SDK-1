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
#include "cbuf.h"

cbuf_size_t cbuf_write( cbuf_t *p_cbuf, const void *pv_src, cbuf_size_t size )
{
	// writes to a cbuf from an external buffer using a copy operation
	cbuf_size_t avail, write_count = 0;
	void *p_dst;
	const cbuf_data_t *p_src = (const cbuf_data_t *)pv_src;
	while( size )
	{
		if( !(avail = cbuf_write_aquire( p_cbuf, &p_dst )) )
			return write_count;
		if( avail > size )
			avail = size;
		CBUF_MEMCPY( p_dst, &p_src[write_count], avail );
		write_count += avail;
		size -= avail;
		cbuf_write_release( p_cbuf, write_count );
	}
	return write_count;
}

cbuf_size_t cbuf_read( cbuf_t *p_cbuf, void *pv_dst, cbuf_size_t size )
{
	// reads from a cbuf from an external buffer using a copy operation
	cbuf_size_t avail, read_count = 0;
	void *p_src;
	while( size )
	{
		if( !(avail = cbuf_read_aquire( p_cbuf, &p_src )) )
			return read_count;
		if( avail > size )
			avail = size;
		CBUF_MEMCPY( pv_dst, p_src, avail );
		read_count += avail;
		size -= avail;
		cbuf_read_release( p_cbuf, read_count );
	}
	return read_count;
}

cbuf_size_t cbuf_write_from_reg8( cbuf_t *p_cbuf, volatile void *pv_fifo, cbuf_size_t size )
{
	// writes data from a single volatile address to a cbuf
	// This is useful for populating a cbuf with data from a hardware FIFO.

	uint8_t *p_fifo = (uint8_t *)pv_fifo;
	cbuf_size_t avail, write_size = 0;
	while( size )
	{
		uint8_t *p;
		if( !(avail = cbuf_write_aquire( p_cbuf, (void **)&p )) )
			return write_size;
		if( avail > size )
			avail = size;
		uint32_t i = 0, c = avail;
		while( c )
		{
			p[i++] = *p_fifo;
			c--;
		}
		write_size += avail;
		size -= avail;
		cbuf_write_release( p_cbuf, write_size );
	}
	return write_size;
}

cbuf_size_t cbuf_read_to_reg8( cbuf_t *p_cbuf, volatile void *pv_fifo, cbuf_size_t size )
{
	// reads data from a cbuf and writes it to a single volatile address.
	// This is useful for copying data from a cbuf to a hardware FIFO.

	uint8_t *p_fifo = (uint8_t *)pv_fifo;
	uint32_t avail, read_size = 0;
	while( size )
	{
		uint8_t *p;
		if( !(avail = cbuf_read_aquire( p_cbuf, (void **)&p )) )
			return read_size;
		if( avail > size )
			avail = size;
		uint32_t i = 0, c = avail;
		while( c )
		{
			*p_fifo = p[i++];
			c--;
		}
		read_size += avail;
		size -= avail;
		cbuf_read_release( p_cbuf, read_size );
	}
	return read_size;
}

