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
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "qp.h"
#include "slave.h"
#include <queue>
#include <conio.h>
#include <io.h>

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "user32.lib")
#define CRYPTO_BLOCK_SIZE 16	// slave firmware is using AES-256 CBC

static 	Slave slave;
static const char* arg_file1;
static const char* arg_file2;
static uint64_t in_total =0, out_total=0;

static ULONGLONG last_time;

#define FRAME_SIZE 88*88*16*4 // size of one image frame

char unfolded_inp[4][352][352];
//unsigned char colors[352][352][3];
unsigned char buffer[2*FRAME_SIZE];  //2 frames
unsigned int buffer_cnt = 0;

COLORREF *arr; // color bitmap

void display(void)
{
	HWND window; 
	HDC dc; 
	window = GetActiveWindow(); //GetForegroundWindow();
	dc = GetDC(window);

	// Creating temp bitmap
	HBITMAP map = CreateBitmap(352, // width. 
		352, // height
		1, // Color Planes, unfortanutelly don't know what is it actually. Let it be 1
		8 * 4, // Size of memory for one pixel in bits (in win32 4 bytes = 4*8 bits)
		(void*)arr); // pointer to array
    
	// Temp HDC to copy picture
	HDC src = CreateCompatibleDC(dc); 
	SelectObject(src, map); // Inserting picture into our temp HDC
	// Copy image from temp HDC to window
	BitBlt(dc, // Destination
		10,  // x and
		10,  // y - upper-left corner of place, where we'd like to copy
		352, // width of the region
		352, // height
		src, // source
		0,   // x and
		0,   // y of upper left corner  of part of the source, from where we'd like to copy
		SRCCOPY); // Defined DWORD to juct copy pixels. Watch more on msdn;

	DeleteDC(src); // Deleting temp HDC
}

void unfold_display(unsigned char * buf)
{
	char inp[64][88][88];

	arr = (COLORREF*)calloc(352 * 352, sizeof(COLORREF));
	
	int index = 0;
	int offset = 0;

	for (int r = 0; r < 88; r++)
		for (int c = 0; c < 88; c++)
		{
			for (int d = 0; d < 64; d += 4)
			{
				offset = index + 88 * 88 * 4 * (int)(d / 4);
				inp[d + 0][r][c] = buf[offset + 0];
				inp[d + 1][r][c] = buf[offset + 1];
				inp[d + 2][r][c] = buf[offset + 2];
				inp[d + 3][r][c] = buf[offset + 3];
			}
			index += 4;
		}

	index = 0;;

	for (int s1 = 0; s1 < 4; s1++)
		for (int s2 = 0; s2 < 4; s2++)
		{
			for (int d = 0; d < 4; d++)
				for (int r = s1; r < 352; r += 4)
					for (int c = s2; c < 352; c += 4)
						unfolded_inp[d][r][c] = inp[index + d][(int)(r / 4)][(int)(c / 4)];
			index += 4;
		}

	char mx = -128;
	int val = 3;
	int cnt = 0;
	unsigned char color[3];

	for (int r = 0; r < 352; r++)
	{
		for (int c = 0; c < 352; c++)
		{
			mx = -128;
			val = 3;
			color[0] = 0;
			color[1] = 0;
			color[2] = 0;
			for (int d = 0; d < 4; d++)
			{
				if (unfolded_inp[d][r][c] > mx)
				{
					mx = unfolded_inp[d][r][c];
					val = d;
				}
			}

			if (val != 3)
			{
				if (val == 1)
					val = 2;
				else if (val == 2)
					val = 1;
				color[val] = 255;
			}

			arr[cnt++] = color[0] << 16 | color[1] << 8 | color[2];// (COLORREF)0xFF0000;// 0x00RRGGBB
		}
	}

	display();
	free(arr);
}

static bool loopback( void )
{
	static uint32_t last_good;
	static uint32_t last_v_in;
	static uint32_t packet_ndx;
	static uint32_t v_in = 0;
	static uint32_t v_out = 0;

	ULONGLONG this_time = last_time;
	bool ret = true;
	this_time = GetTickCount64();
	ULONGLONG dt = this_time - last_time;
	if (dt > 1000)
	{
		ULONGLONG db = (ULONGLONG)4 * ( v_in - last_v_in );
		printf("%0.1fMB compared!", (double)v_in * 4.0 / (1024.0*1024.0) );
		double bytes_per_second = 1000.0 * (double)db / (double)dt;
		printf(", %0.0f kB/sec\n", bytes_per_second / (1024.0));
		last_v_in = v_in;
		last_time = this_time;
	}
	uint16_t i, writeable, rand_writeable;
	uint32_t* p_out = (uint32_t*)slave.AllocOut(&writeable);

	//rand_writeable = (8 % (1 + writeable)) & ~3;
	rand_writeable = (rand() % (1 + writeable)) & ~3;

	for (i = 0; i < rand_writeable >> 2; i++)
	{
		p_out[i] = v_out++;
	}

	uint16_t in_size;
	uint16_t out_size = rand_writeable;
	uint32_t* p_in = (uint32_t*)slave.AllocIn(&in_size);
	packet_ndx++;
	//Sleep(200);
//	if (!slave.Xfer(p_in, in_size, p_out, out_size) )

	if (!slave.newXfer(p_in, in_size, p_out, out_size) )
	{
		printf("QSPI transfer failure.\n");
		ret = false;
		goto done;
	}
//	printf("out=%04x, in=%04x, Osize=%x \n", *p_out, *p_in, out_size);
	for (i = 0; i < in_size/4; i++)
	{
	//	if (p_in[i] != v_in++)
	//		printf("Not Good!\n");
		if (p_in[i] != v_in++)
		{	
			printf("Loopback data compare failure, packet #%d, out_size=%d, in_size=%d, offset=%d.\n", packet_ndx, out_size, in_size, i );
			printf("%8.8X = last good in\n", last_good);
			uint32_t dc = in_size / 4 - i;
			if (dc > 8)
				dc = 8;
			printf("failed in:\n");
			for (uint32_t j = 0; j < dc; j++)
			{
				printf("%8.8X\n", p_in[i + j]);
			}
			ret = false;
			goto done;
		}
		last_good = p_in[i];
	}
	if (i)
		last_good = p_in[i - 1];

done:
	slave.FreeIn(p_in);
	slave.FreeOut(p_out);
	return ret;
}

static bool trng(FILE* fout)
{
	// stores data from the MAX32520's TRNG into a file
	bool ret = true;
	static size_t in_last = 0;
	uint16_t in_size;

	void* p_out = slave.AllocOut(0);
	void* p_in = slave.AllocIn(&in_size);

	ULONGLONG this_time = GetTickCount64();
	ULONGLONG dt = this_time - last_time;

	if (dt > 1000)
	{
		printf("%0.1fMbits generated", (double)in_total * 8.0 /  (1024.0*1024.0) );
		uint32_t db = 8 * in_last;
		double bits_per_second = 1000.0 * (double)db / (double)dt;
		printf(", %0.2f Mbits/sec\n", bits_per_second / (1024.0 * 1024.0));
		last_time = this_time;
		in_last = 0;
	}

	in_total += in_size;
	in_last += in_size;

	if (!slave.newXfer(p_in, in_size, p_out, 0))
	{
		printf("QSPI transfer failure.\n");
		ret = false;
		goto done;
	}
	if (in_size && fwrite(p_in, 1, in_size, fout) != in_size)
	{
		printf("Error writing to output file, '%s'\n", arg_file1 );
		ret = false;
		goto done;
	}
done:
	slave.FreeIn(p_in);
	slave.FreeOut(p_out);
	return ret;
}

static bool streaming(FILE* fout)
{
	// stores data from the MAX78000 inference into a file
	bool ret = true;
	static size_t in_last = 0;
	static int fcount = 0;
	static ULONGLONG last_frame_time = 0;
	uint16_t in_size;

	void* p_out = slave.AllocOut(0);
	void* p_in = slave.AllocIn(&in_size);

	ULONGLONG this_time = GetTickCount64();
	ULONGLONG dt = this_time - last_time;

	if (dt > 1000)
	{
		printf("%0.1fMbits generated", (double)in_total * 8.0 / (1024.0 * 1024.0));
		uint32_t db = 8 * in_last;
		double bits_per_second = 1000.0 * (double)db / (double)dt;
		printf(", %0.2f Mbits/sec\n", bits_per_second / (1024.0 * 1024.0));
		last_time = this_time;
		in_last = 0;
	}
	
	in_total += in_size;
	in_last += in_size;
	//printf("Total size, '%d'\n", in_total);
	if (!slave.newXfer(p_in, in_size, p_out, 0))
	{
		printf("QSPI transfer failure.\n");
		ret = false;
		goto done;
	}
	if (fout != NULL)
	{
		if (in_size && fwrite(p_in, 1, in_size, fout) != in_size)
		{
			printf("Error writing to output file, '%s'\n", arg_file1);
			ret = false;
			goto done;
		}
	}
	else
	{
		// copy the streaming data to display buffer and show once we have one frame
		//---------------------------------------
		memcpy(&buffer[buffer_cnt], p_in, in_size);
		buffer_cnt += in_size;

		if (buffer_cnt >= FRAME_SIZE)
		{
			unfold_display(buffer);
			//printf("%d \n", buffer_cnt);
			// copy the leftover for the next frame
			memcpy(&buffer[0], &buffer[buffer_cnt], buffer_cnt - FRAME_SIZE);
			buffer_cnt -= FRAME_SIZE;
			ULONGLONG dt_frame = this_time - last_frame_time;
			last_frame_time = this_time;
			printf("%0.2f fps\n", 1000.0 / (double)dt_frame);
		}
		//---------------------------------------
	}

done:
	slave.FreeIn(p_in);
	slave.FreeOut(p_out);
	return ret;
}
static bool cnn_in(FILE* fout)
{
	// stores data from the MAX78000 cnn input into a file
	bool ret = true;
	static size_t in_last = 0;
	uint16_t in_size;

	void* p_out = slave.AllocOut(0);
	void* p_in = slave.AllocIn(&in_size);

	ULONGLONG this_time = GetTickCount64();
	ULONGLONG dt = this_time - last_time;

	if (dt > 1000)
	{
		printf("%0.1fMbits generated", (double)in_total * 8.0 / (1024.0 * 1024.0));
		uint32_t db = 8 * in_last;
		double bits_per_second = 1000.0 * (double)db / (double)dt;
		printf(", %0.2f Mbits/sec\n", bits_per_second / (1024.0 * 1024.0));
		last_time = this_time;
		in_last = 0;
	}

	in_total += in_size;
	in_last += in_size;
	//printf("Total size, '%d'\n", in_total);
	if (!slave.newXfer(p_in, in_size, p_out, 0))
	{
		printf("QSPI transfer failure.\n");
		ret = false;
		goto done;
	}
	if (in_size && fwrite(p_in, 1, in_size, fout) != in_size)
	{
		printf("Error writing to output file, '%s'\n", arg_file1);
		ret = false;
		goto done;
	}
done:
	slave.FreeIn(p_in);
	slave.FreeOut(p_out);
	return ret;
}


static inline uint16_t clamp_size(uint16_t s, uint16_t block_size)
{
	// returns s clamped by block size.  essentially a binary modulous operator.
	return s & ~(block_size - 1);
}

static uint16_t pad_size(uint16_t s, uint16_t block_size)
{
	// returns a value >= size by up to block_size-1.  
	// used to pad crypto blocks
	uint16_t pad = clamp_size(s, block_size);
	if (s != pad)
	{
		pad += block_size;
	}
	return pad;
}

static bool crypto(FILE* fin, FILE* fout)
{
	bool ret = true;

	uint16_t in_size, out_size; // "in" and "out" are from the perspective of the master-to-slave connection, not the filesystem.
	uint16_t read_size;
	// allocate buffers for the QSPI transaction
	void* p_out = slave.AllocOut(&out_size);
	void* p_in = slave.AllocIn(&in_size);

	// reduce outgoing size to a crypto block size, if necessary
	out_size = (uint16_t)clamp_size(out_size, CRYPTO_BLOCK_SIZE);

	if (!out_size)
		goto done;

	memset(p_out, 0, out_size); // zero-padding for crypto

	read_size = (uint16_t)fread_s(p_out, out_size, 1, out_size, fin);
	if ( !read_size && !feof(fin))
	{
		printf("Error:  failed to read from input file, '%s'.\n", arg_file1);
		goto done;
	}
	// out_size >= read_size, by one crypto block size
	out_size = pad_size(read_size, CRYPTO_BLOCK_SIZE);

	out_total += out_size;
	if (!slave.Xfer(p_in, in_size, p_out, out_size))
	{
		printf("QSPI transfer failure.\n");
		goto done;
	}
	in_total = in_size;

	if (in_size && fwrite(p_in, 1, in_size, fout) != in_size)
	{
		printf("Error:  failed writing to output file, '%s'\n", arg_file2 );
		goto done;
	}

	ret =  out_total != in_total; // returns true if more work to do

done:
	slave.FreeOut(p_out);
	slave.FreeIn(p_in);
	return ret;

}

static void usage(void)
{
	printf("Usage examples:\n");
	printf("\tqspim loopback\n");
	printf("\tqspim trng <output filename>\n");
	printf("\tqspim encrypt <input filename> <output filename>\n");
	printf("\tqspim decrypt <input filename> <output filename>\n");
	printf("\tqspim streaming <output filename>\n");
	printf("\tqspim cnn_in <output filename>\n");
}

qp_mode_t get_mode(const char* mode)
{
	static const char* mode_str[] =
	{
		"loopback",
		"trng",
		"encrypt",
		"decrypt",
		"streaming",
		"cnn_in"
	};
	static const qp_mode_t mode_id[] =
	{
		qp_mode_loopback,
		qp_mode_trng,
		qp_mode_encrypt,
		qp_mode_decrypt,
		qp_mode_streaming,
		qp_mode_cnn_in

	};
	int i;
	for (i = 0; i < ARRAY_COUNT(mode_str); i++)
	{
		if (!_stricmp(mode, mode_str[i]))
			return mode_id[i];
	}
	return qp_mode_null;
}


int main( int argc, char * argv[] )
{

	int ret = -1;
	qp_mode_t mode;
	const char * arg_mode = argv[1];
	
	arg_file1 = argv[2];
	arg_file2 = argv[3];

	FILE* file1 = NULL;
	FILE* file2 = NULL;

	Slave::id_t* p_slave_id = NULL;

	if (argc < 2 || argc > 4 )
	{
		printf("Error:  unsupported number of arguments\n");
		usage();
		goto shutdown;
	}

	mode = get_mode(arg_mode);
	if (mode == qp_mode_null)
	{
		printf("Error:  unsupported mode '%s'.", arg_mode);
		goto shutdown;
	}

	if (!Slave::Search(&p_slave_id))
	{
		//printf("Error:  MAX32520FTHR not found.\n");
		printf("Error:  Not Connected.\n");
		goto shutdown;
	}
	// This software will only connect to the first FT4222H it finds.
	if (!slave.Open(p_slave_id[0].serial_number))
	{
		printf("Error:  Could not connect to the FT4222H.\n");
		goto shutdown;
	}
	printf("S/N = %s\n", p_slave_id[0].serial_number);
	switch (mode)
	{
		case qp_mode_encrypt:
		case qp_mode_decrypt:
		{
			if (argc != 4)
			{
				printf("Error:  unsupported number of arguments\n");
				usage();
				goto shutdown;
			}
			if (fopen_s(&file1, arg_file1, "rb"))
			{
				printf("Error:  failed to open input file '%s'\n", arg_file1);
				usage();
				goto shutdown;
			}
			if (fopen_s(&file2, arg_file2, "wb"))
			{
				printf("Error:  failed to create output file '%s'\n", arg_file2);
				usage();
				goto shutdown;
			}
			break;
		}
		case qp_mode_trng:
		{
			if (argc != 3)
			{
				printf("Error:  unsupported number of arguments\n");
				usage();
				goto shutdown;
			}
			if (fopen_s(&file1, arg_file1, "wb"))
			{
				printf("Error:  failed to create output file '%s'\n", arg_file1);
				usage();
				goto shutdown;
			}
			break;
		}
		case qp_mode_streaming:
		{
			if (argc == 2)
			{
				file1 = NULL;
				break;
			}
			if ((argc != 3) && (argc != 2))
			{
				printf("Error:  unsupported number of arguments\n");
				usage();
				goto shutdown;
			}
			if (fopen_s(&file1, arg_file1, "wb"))
			{
				printf("Error:  failed to create output file '%s'\n", arg_file1);
				usage();
				goto shutdown;
			}
			break;
		}
		case qp_mode_cnn_in:
		{
			if (argc != 3)
			{
				printf("Error:  unsupported number of arguments\n");
				usage();
				goto shutdown;
			}
			if (fopen_s(&file1, arg_file1, "wb"))
			{
				printf("Error:  failed to create output file '%s'\n", arg_file1);
				usage();
				goto shutdown;
			}
			break;
		}
		case qp_mode_loopback:
		{
			if (argc != 2)
			{
				printf("Error:  unsupported number of arguments\n");
				usage();
				goto shutdown;
			}
			break;
		}
	}
	printf("qp_mode_t = qp_mode_%s_t\n", arg_mode);
	printf("mode=%x\n", mode);
//	for (uint32_t i = 5; i < 10; i++) {
//		mode = (qp_mode_t)(i);
//		Sleep(1);
//		if (!slave.SetMode(mode))
		if (!slave.newSetMode(mode))
		{
			printf("Error:  The slave did not respond correctly.  Did the firwmare hit a breakpoint?\n");
			goto shutdown;
		}
//	}
//	return false;
	last_time = GetTickCount64();
	while(1)
	{
		if (_kbhit())
		{
			printf("process interrupted by keypress.\n");
			break;
		}
		switch (mode)
		{
			case qp_mode_decrypt:
			{
				if (crypto(file1, file2))
					continue;
				printf("decrypted %llu bytes from '%s' to '%s'\n", out_total, arg_file1, arg_file2);
				goto shutdown;
			}
			case qp_mode_encrypt:
			{
				if (crypto(file1, file2))
					continue;
				printf("encrypted %llu bytes from '%s' to '%s'\n", out_total, arg_file1, arg_file2);
				goto shutdown;
			}
			case qp_mode_loopback:
			{
				if (loopback())
					continue;
				printf("%llu bytes compared.\n", out_total);
				goto shutdown;
			}
			case qp_mode_trng:
			{
				if (trng(file1))
					continue;
				printf("%llu bits generated.\n", in_total * 8);
				goto shutdown;
			}
			case qp_mode_streaming:
			{
				if (streaming(file1))
					continue;
				printf("%llu bits generated.\n", in_total * 8);
				goto shutdown;
			}
			case qp_mode_cnn_in:
			{
				if (cnn_in(file1))
					continue;
				printf("%llu bits generated.\n", in_total * 8);
				goto shutdown;
			}
		}
	};

shutdown:

	if (file1)
		fclose(file1);
	if (file2)
		fclose(file2);

	slave.Close();
	slave.Open(p_slave_id[0].serial_number);  // the extra open/close signals the firmware to end the operational mode and go idle.
											  // every open asserts the slave select signal, but clocks no data.  The slave firmware
											  // responds to this by returning to the idle state.
	slave.Close();
	if( p_slave_id )
		delete[] p_slave_id;
	return ret;
}
