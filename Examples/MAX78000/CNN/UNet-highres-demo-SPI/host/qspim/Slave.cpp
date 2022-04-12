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

#include "global.h"
#include "LibFT4222.h"
#include "qp.h"
#include "Slave.h"
#include <stdio.h>
#include <conio.h>

uint32_t Slave::Search( id_t** p_slave_id )
{
	DWORD dev_num = 0;
	FT_STATUS status = FT_CreateDeviceInfoList(&dev_num);
	if (FT_OK != status)
		return 0;
	FT_DEVICE_LIST_INFO_NODE* dev_node = (FT_DEVICE_LIST_INFO_NODE*)malloc(dev_num * sizeof(FT_DEVICE_LIST_INFO_NODE));
	if (!dev_node)
		return 0;
	status = FT_GetDeviceInfoList(dev_node, &dev_num);
	if (FT_OK != status)
		return 0;
	size_t length = (size_t)dev_num * SLAVE_SERIAL_NUMBER_SIZE;
	id_t* p = (id_t*)new id_t[dev_num];
	if (!p)
		return 0;
	memset(p, 0, dev_num*sizeof(id_t));
	uint32_t i = 0, j = 0;
	printf("dev no = %d\n", dev_num);
	while (dev_num--)
	{
		printf("connected to %s\n", dev_node[i].Description);
		if ((!memcmp("MAX32520FTHR", dev_node[i].Description, 12)) || !memcmp("FT4222", dev_node[i].Description, 6))
		//if (!memcmp("FT4222", dev_node[i].Description, 6))
		{
			memcpy(p[j++].serial_number, dev_node[i].SerialNumber, SLAVE_SERIAL_NUMBER_SIZE);
		}
		i++;
	}
	free(dev_node);
	*p_slave_id = p;
	return j;
}

bool Slave::newSetMode(qp_mode_t qp_mode)
{
	// set firmware operational mode
	// This must be the first transfer after resetting the slave
	uint16_t out_size;
	qp_mode_t* p_out = (qp_mode_t*)AllocOut(&out_size);
	void* p_in = AllocIn();
	out_size = sizeof(qp_mode_t);
	*p_out = qp_mode;
	if (!newXfer(p_in, 0, p_out, out_size)) {
		printf("failed1\n");
		return false;
	}
	printf("in  = %08x\n", *(uint32_t*)p_in);
	printf("out = %08x\n", *(uint32_t*)p_out);
	printf("writable = %08x\n", this->flow.writeable);
	printf("readable = %08x\n", this->flow.readable);
	if (!this->flow.writeable || this->flow.readable) {
		printf("failed2\n");
		return false; // the board firmware is not in an expected state.
	}
	FreeIn(p_in);
	FreeOut(p_out);
	return true;
}

bool Slave::SetMode(qp_mode_t qp_mode)
{
	// set firmware operational mode
	// This must be the first transfer after resetting the slave
	uint16_t out_size;
	qp_mode_t* p_out = (qp_mode_t*)AllocOut(&out_size);
	void* p_in = AllocIn();
	out_size = sizeof(qp_mode_t);
	*p_out = qp_mode;
	if (!Xfer(p_in, 0, p_out, out_size)) {
		printf("failed1\n");
		return false;
	}
	printf("in  = %08x\n", *(uint32_t*)p_in);
	printf("out = %08x\n", *(uint32_t*)p_out);
	printf("writable = %08x\n", this->flow.writeable);
	printf("readable = %08x\n", this->flow.readable);
	if (!this->flow.writeable || this->flow.readable) {
		printf("failed2\n");
		return false; // the board firmware is not in an expected state.
	}
	FreeIn(p_in);
	FreeOut(p_out);
	return true;
}

bool Slave::Open( const char * serial_number )
{
	if (serial_number)
	{
		Close();

		if (FT_OK != FT_OpenEx((PVOID)serial_number, FT_OPEN_BY_SERIAL_NUMBER, &this->hft))
			return false;

		if (FT_OK != FT4222_SetClock(this->hft, SYS_CLK_80))
			return false;

		if (FT_OK != FT4222_SPIMaster_Init(this->hft, SPI_IO_QUAD, CLK_DIV_4, QP_SPI_MODE_POLARITY ? CLK_IDLE_HIGH : CLK_IDLE_LOW, QP_SPI_MODE_PHASE ? CLK_TRAILING : CLK_LEADING, 0x01))
			return false;

		if (FT_OK != FT4222_SPI_SetDrivingStrength(this->hft, DS_4MA,DS_4MA,DS_4MA))
			return false;

		this->flow.readable = 0;
		this->flow.writeable = sizeof(qp_flow_t);
		Sleep(50);	// wait for slave to come out of reset (reset time allowance should be > 100us)
	}
	return true;
}


void * Slave::AllocIn( uint16_t * size )
{
	qp_flow_t * p_in = (qp_flow_t*)malloc( this->flow.readable + sizeof(qp_flow_t));
	if (!p_in)
		return NULL;
	p_in++;
	if(size)
		*size = this->flow.readable;
	return p_in;
}

void Slave::FreeIn(void* pv)
{
	qp_flow_t* p_in = (qp_flow_t*)pv;
	p_in--;
	free(p_in);
}

void* Slave::AllocOut(uint16_t * size )
{
	uint32_t* p_out = (uint32_t*)malloc(this->flow.writeable+sizeof(uint32_t));
	if (!p_out)
		return NULL;
	if( size )
		*size = this->flow.writeable;
	return ++p_out;
}

void Slave::FreeOut(void* pv)
{
	uint32_t* p_out = (uint32_t*)pv;
	free(--p_out);
}

void Slave::Close(void)
{
	if (this->hft)
	{
		FT4222_UnInitialize(this->hft);
		FT_Close(this->hft);
	}
}

bool Slave::newXfer(void* pv_in, uint16_t in_size, const void* pv_out, uint16_t out_size)
{
	// performs a write transaction followed by a read transaction
	// data transfer amounts are limited by the flow control mechanism (qp_flow_t)

	qp_flow_t* p_in = (qp_flow_t*)pv_in;
	uint32_t* p_out = (uint32_t*)pv_out;

	p_in--;
	p_out--;

	*p_out = out_size;

	if (!write(p_out, out_size + sizeof(uint32_t)))
		return false;
	//while (!_kbhit());
	//Sleep(.1);
	if (!read(p_in, in_size + sizeof(qp_flow_t)))
		return false;



	this->flow = *p_in;
	this->flow.readable -= in_size;

	return true;
}

bool Slave::Xfer( void * pv_in, uint16_t in_size, const void * pv_out, uint16_t out_size )
{
	// performs a write transaction followed by a read transaction
	// data transfer amounts are limited by the flow control mechanism (qp_flow_t)

	qp_flow_t* p_in = (qp_flow_t*)pv_in;
	uint32_t * p_out = (uint32_t*)pv_out;

	p_in--;
	p_out--;

	*p_out = out_size;
	if (!write_read( p_in, in_size+sizeof(qp_flow_t), p_out, out_size+sizeof(uint32_t) ) )
		return false;

	this->flow = *p_in;
	this->flow.readable -= in_size;
	
	return true;
}

bool Slave::write_read(void* pv_in, uint16_t in_size, void* pv_out, uint16_t out_size)
{
	uint32 read;
	uint32 err_value;
	//if ((FT_OK != FT4222_SPIMaster_MultiReadWrite(this->hft, (uint8*)pv_in, (uint8*)pv_out, 0, out_size, in_size, &read) || read != in_size))
	err_value = FT4222_SPIMaster_MultiReadWrite(this->hft, (uint8*)pv_in, (uint8*)pv_out, 0, out_size, in_size, &read) ;
	if ((FT_OK != err_value || read != in_size)) {
		printf("ERROR Code: %d", err_value);
		return false;
	}
	return true;

}

bool Slave::read( void * pv_in, uint16 in_size )
{
	uint32 read;
	if ((FT_OK != FT4222_SPIMaster_MultiReadWrite(this->hft, (uint8*)pv_in, NULL, 0, 0, in_size, &read) || read != in_size ) )
		return false;
	return true;
}

bool Slave::write( const void * pv_out, uint16 out_size )
{
	uint32 read;
	if ((FT_OK != FT4222_SPIMaster_MultiReadWrite( this->hft, NULL, (uint8*)pv_out, 0, out_size, 0, &read)) )
		return false;
	return true;
}

