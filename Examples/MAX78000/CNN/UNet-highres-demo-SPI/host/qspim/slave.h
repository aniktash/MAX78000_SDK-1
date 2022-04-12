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

class Slave
{
	
private:

	void *			hft;

	bool read(void* pv_in, uint16_t in_size);
	bool write(const void* pv_out, uint16_t out_size);
	bool write_read(void* pv_in, uint16_t in_size, void* pv_out, uint16_t out_size);

public:

	qp_flow_t		flow;

	#define SLAVE_SERIAL_NUMBER_SIZE	16
	typedef struct
	{
		char serial_number[SLAVE_SERIAL_NUMBER_SIZE];
	}
	id_t;

	static uint32_t Search( id_t** p_slave_id );

	bool Open( const char* serial_number );
	void Close(void);
	bool Xfer(void* pv_in, uint16_t in_size, const void* pv_out, uint16_t out_size); 
	bool newXfer(void* pv_in, uint16_t in_size, const void* pv_out, uint16_t out_size);
	bool SetMode( qp_mode_t p_qp_mode );
	bool newSetMode(qp_mode_t p_qp_mode);

	void* AllocIn( uint16_t * size = NULL );
	void FreeIn(void* pv);
	void* AllocOut(uint16_t * size = NULL );
	void FreeOut(void* pv);
};


