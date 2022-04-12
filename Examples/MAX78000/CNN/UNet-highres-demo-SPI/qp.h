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

// This defines the protocol used by the qspim host and the qspis firmware

// low-level SPI bus configuration
#define QP_SPI_MODE_POLARITY		0	// 0/1 = idle clock low/high
#define QP_SPI_MODE_PHASE			0	// 0/1 = data clocked in on rising/falling edge

typedef enum
{
	// command supported by the qspim host software and qspis slave firmware
	qp_mode_null,
	qp_mode_loopback,
	qp_mode_encrypt,
	qp_mode_trng,
	qp_mode_decrypt,
	qp_mode_streaming,
	qp_mode_cnn_in
}
qp_mode_t;

#pragma pack(1)

typedef struct
{
	// This prepends all QSPI frames read out from the qspis firmware

	// QSPI transfer sizes are always multiples of 32-bits
	// therefore the lower two bits of these members must be zero.
	// The slave sends this data at the begining of the QSPI read accesses
	uint16_t writeable;	// number of bytes that the slave can accept from the host (including this transfer)
	uint16_t readable;	// number of bytes that can be read from the slave (including this transfer)
}
qp_flow_t;

// Not defined explicity is a single 32-bit value that prepends all QSPI frames written to the qspis firmware.
// This value is the number of bytes in the write phase of each QSPI write/read cycle.

#pragma pack()
