/*******************************************************************************
* Copyright (C) 2021 Maxim Integrated Products, Inc., All rights Reserved.
*
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
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
*******************************************************************************/

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "uart.h"
#include "led.h"
#include "board.h"

#include "camera.h"
#include "dma.h"
#include "camera_util.h"

#ifdef ENABLE_TFT  // defined in make file
    #ifdef EvKit_V1
#include "tft.h"
    #endif

    #ifdef FTHR_RevA
#include "tft_fthr.h"
    #endif
#endif


//#define FIXED_PATTERN

#ifdef PATTERN_GEN
#define FIXED_PATTERN
#endif

int slaveAddress;
int id;
int dma_channel;
uint32_t RED_VALUE = 175;

static uint32_t * data_addr0 = (uint32_t *) 0x50400700;
//static uint32_t * data_addr1 = (uint32_t *) 0x50408700;
//static uint32_t * data_addr2 = (uint32_t *) 0x50410700;
static uint32_t * data_addr3 = (uint32_t *) 0x50418700;

//static uint32_t * data_addr4 = (uint32_t *) 0x50800700;
//static uint32_t * data_addr5 = (uint32_t *) 0x50808700;
static uint32_t * data_addr6 = (uint32_t *) 0x50810700;
//static uint32_t * data_addr7 = (uint32_t *) 0x50818700;

//static uint32_t * data_addr8 = (uint32_t *) 0x50c00700;
static uint32_t * data_addr9 = (uint32_t *) 0x50c08700;
//static uint32_t * data_addr10 = (uint32_t *) 0x50c10700;
//static uint32_t * data_addr11 = (uint32_t *) 0x50c18700;
uint8_t ur, ug, ub;

static uint32_t  *addr, offset0, offset1, subtract;

uint8_t data565[IMAGE_XRES*2];
// *****************************************************************************

int initialize_camera(void)
{
    int ret = 0;



     // Initialize DMA for camera interface
    MXC_DMA_Init();
    dma_channel = MXC_DMA_AcquireChannel();
    printf("dma_channel for camera: %d", dma_channel);


    // Initialize the camera driver.
    camera_init(CAMERA_FREQ);
    printf("\n\nCamera Example\n");

    slaveAddress = camera_get_slave_address();
    printf("Camera I2C slave address: %02x\n", slaveAddress);

    // Obtain the manufacturer ID of the camera.
    ret = camera_get_manufacture_id(&id);

    if (ret != STATUS_OK) {
        printf("Error returned from reading camera id. Error %d\n", ret);
        return -1;
    }

    printf("Camera ID detected: %04x\n", id);

#if defined(CAMERA_HM01B0) || defined(CAMERA_HM0360) || defined(CAMERA_OV5642)
    camera_set_hmirror(0);
    camera_set_vflip(0);
#endif


   printf("Init Camera\n");

    // Setup the camera image dimensions, pixel format and data acquiring details.
#ifdef RGB565
    ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB565, FIFO_FOUR_BYTE, STREAMING_DMA, dma_channel); // RGB565 stream
#else //RGB888 with 0 at MSB
    ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB888, FIFO_THREE_BYTE, STREAMING_DMA, dma_channel); // RGB888 stream
#endif


    if (ret != STATUS_OK) {
        printf("Error returned from setting up camera. Error %d\n", ret);
        return -1;
    }
   // camera_sleep(1); // sleep
    MXC_Delay(SEC(1));

#if defined(CAMERA_OV7692) && defined(STREAM_ENABLE)
   // camera_write_reg(0x11, 0x3); // set camera clock prescaller to prevent streaming overflow for QVGA
    camera_write_reg(0x11, 0x1); // set camera clock prescaller to prevent streaming overflow for QVGA
#endif

    // make the scale ratio of camera input size the same as output size, make is faster and regular
    camera_write_reg(0xc8, 0x1);
    camera_write_reg(0xc9, 0x60);
    camera_write_reg(0xca, 0x1);
    camera_write_reg(0xcb, 0x60);
/*
	camera_write_reg(0x14, 0x60); //gain ceiling max 128

	camera_write_reg(0x13, 0xee); //autoexposure off
	camera_write_reg(0x0f, 0xf0); //msb of exposure
	camera_write_reg(0x10, 0xff); //lsb of exposure
*/

 //  camera_write_reg(0x61, 0x60); //pattern bar

 //   camera_write_reg(0x80, 0x7E);
 //  camera_write_reg(0x82, 0x0F); //pattern bar
//    camera_write_reg(0x82, 0x0F); //pattern bar
    return ret;
}





void load_row_cnn_init(void)
{
		data_addr0 = (uint32_t *) 0x50400700;
		data_addr3 = (uint32_t *) 0x50418700;
		data_addr6 = (uint32_t *) 0x50810700;
		data_addr9 = (uint32_t *) 0x50c08700;
		addr = data_addr9;
}


uint32_t m;


void load_row_cnn(uint8_t *data, int row)
{
    union {
        uint32_t w;
        uint8_t b[4];
    } m;

#ifdef RGB565
    uint16_t *dataptr = (uint16_t *)data;
#else
    uint8_t *dataptr = data;
#endif

    offset0 = 0x00002000;
    offset1 = 0x00002000;
    subtract = 0x00004000-1;

    switch (row & 3) {
        case 0:
            data_addr9 = addr;
            addr = data_addr0;
            break;
        case 1:
            data_addr0 = addr;
            addr = data_addr3;
            offset0 += 0x000FA000-0x00002000;
            subtract += 0x000FC000-0x00004000;
            break;
        case 2:
            data_addr3 = addr;
            addr = data_addr6;
            offset1 += 0x000FA000-0x00002000;
            subtract += 0x000FC000-0x00004000;
            break;
        default:
            data_addr6 = addr;
            addr = data_addr9;
            break;
    }

	// indexes of 352x352 image (row,j)
	for (int j = 0; j < IMAGE_XRES; j += 4)
	{
#if defined(FIXED_PATTERN)
        // static test pattern
        m.b[0] = RED_VALUE;           // r0
        m.b[1] = j >> 1;        // g0
        m.b[2] = row >> 1;      // b0
        m.b[3] = RED_VALUE;           // r1

        *addr = m.w ^ 0x80808080U;
        addr += offset0;

        m.b[0] = (j+1) >> 1;    // g1
        m.b[1] = row >> 1;      // b1
        m.b[2] = RED_VALUE;           // r2
        m.b[3] = (j+2) >> 1;    // g2

        *addr = m.w ^ 0x80808080U;
        addr += offset1;

        m.b[0] = row >> 1;      // b2
        m.b[1] = RED_VALUE;           // r3
        m.b[2] = (j+3) >> 1;    // g3
        m.b[3] = row >> 1;      // b3

        *addr = m.w ^ 0x80808080U;
        addr -= subtract;
#elif defined(RGB565)
        // RGB565 to packed 24-bit RGB
        m.b[0] = (*dataptr & 0xF800) >> 11;
        m.b[1] = (*dataptr & 0x07E0) >> 5;
        m.b[2] = (*dataptr & 0x001F) << 3;
        dataptr++;
        m.b[3] = (*dataptr & 0xF800) >> 11;

        *addr = m.w ^ 0x80808080U;
        addr += offset0;

        m.b[0] = (*dataptr & 0x07E0) >> 5;
        m.b[1] = (*dataptr & 0x001F) << 3;
        dataptr++;
        m.b[2] = (*dataptr & 0xF800) >> 11;
        m.b[3] = (*dataptr & 0x07E0) >> 5;

        *addr = m.w ^ 0x80808080U;
        addr += offset1;

        m.b[0] = (*dataptr & 0x001F) << 3;
        dataptr++;
        m.b[1] = (*dataptr & 0xF800) >> 11;
        m.b[2] = (*dataptr & 0x07E0) >> 5;
        m.b[3] = (*dataptr & 0x001F) << 3;
        dataptr++;

        *addr = m.w ^ 0x80808080U;
        addr -= subtract;
#else
        // unpacked 24-bit RGB to packed 24-bit RGB
        m.b[0] = *dataptr++; // r0
        m.b[1] = *dataptr++; // g0
        m.b[2] = *dataptr++; // b0
        dataptr++;           // skip MSB
        m.b[3] = *dataptr++; // r1

        *addr = m.w ^ 0x80808080U;
        addr += offset0;

        m.b[0] = *dataptr++; // g1
        m.b[1] = *dataptr++; // b1
        dataptr++;           // skip MSB
        m.b[2] = *dataptr++; // r2
        m.b[3] = *dataptr++; // g2

        *addr = m.w ^ 0x80808080U;
        addr += offset1;

        m.b[0] = *dataptr++; // b2
        dataptr++;           // skip MSB
        m.b[1] = *dataptr++; // r3
        m.b[2] = *dataptr++; // g3
        m.b[3] = *dataptr++; // b3
        dataptr++;           // skip MSB

        *addr = m.w ^ 0x80808080U;
        addr -= subtract;
#endif
    }

}



/*
  Data Order:
  camera data: (352,352,3), following indexes are based on camera pixel index

  0x50400700:
 	  (0,1,0)|(0,0,2)|(0,0,1)|(0,0,0)              // 0
 	  (0,5,0)|(0,4,2)|(0,4,1)|(0,4,0)              // 1
 	  ...
 	  (0,349,0)|(0,348,2)|(0,348,1)|(0,348,0)      // 87

 	  (4,1,0)|(4,0,2)|(4,0,1)|(4,0,0)              // 88
 	  (4,5,0)|(4,4,2)|(4,4,1)|(4,4,0)
 	  ...
 	  (4,349,0)|(4,348,2)|(4,348,1)|(4,348,0)		// 175
 	  ...
 	  ...
 	  ...
 	  (348,1,0)|(348,0,2)|(348,0,1)|(348,0,0)              //
 	  (348,5,0)|(348,4,2)|(348,4,1)|(348,4,0)
 	  ...
 	  (348,349,0)|(348,348,2)|(348,348,1)|(348,348,0)		// 7743

  0x50408700:
   	  (0,2,1)|(0,2,0)|(0,1,2)|(0,1,1)              // 0
 	  (0,6,1)|(0,6,0)|(0,5,2)|(0,5,1)              // 1
 	  ...
 	  (0,350,1)|(0,350,0)|(0,349,2)|(0,349,1)      // 87

 	  (4,2,1)|(4,2,0)|(4,1,2)|(4,1,1)              // 88
 	  (4,6,1)|(4,6,0)|(4,5,2)|(4,5,1)
 	  ...
 	  (4,350,1)|(4,350,0)|(4,349,2)|(4,349,1)      // 175
 	  ...
 	  ...
 	  ...
 	  (348,2,1)|(348,2,0)|(348,1,2)|(348,1,1)              //
 	  (348,6,1)|(348,6,0)|(348,5,2)|(348,5,1)
 	  ...
 	  (348,350,1)|(348,350,0)|(348,349,2)|(348,349,1)      // 7743

  0x50410700:
   	  (0,3,2)|(0,3,1)|(0,3,0)|(0,2,2)              // 0
 	  (0,7,2)|(0,7,1)|(0,7,0)|(0,6,2)              // 1
 	  ...
 	  (0,351,2)|(0,351,1)|(0,351,0)|(0,350,2)      // 87

 	  ...
 	  ...
 	  ...
 	  (348,3,2)|(348,3,1)|(348,3,0)|(348,2,2)              //
 	  (348,7,2)|(348,7,1)|(348,7,0)|(348,6,2)
 	  ...
 	  (348,351,2)|(348,351,1)|(348,351,0)|(348,350,2)      // 7743


 The same pattern of 3x7744 words repeats another 3 times, with starting row index changed from 0 to 1, then 2 and then 3
 resulting in 4x3x7744 words:
 ....
 	0x50c18700: last bank
 	...
 	  (351,351,2)|(351,351,1)|(351,351,0)|(351,350,2)      // 7743


   */


  // This function loads the sample data input -- replace with actual data

  /*
  memcpy32((uint32_t *) 0x50400700, input_0, 7744);
  memcpy32((uint32_t *) 0x50408700, input_4, 7744);
  memcpy32((uint32_t *) 0x50410700, input_8, 7744);
  memcpy32((uint32_t *) 0x50418700, input_12, 7744);
  memcpy32((uint32_t *) 0x50800700, input_16, 7744);
  memcpy32((uint32_t *) 0x50808700, input_20, 7744);
  memcpy32((uint32_t *) 0x50810700, input_24, 7744);
  memcpy32((uint32_t *) 0x50818700, input_28, 7744);
  memcpy32((uint32_t *) 0x50c00700, input_32, 7744);
  memcpy32((uint32_t *) 0x50c08700, input_36, 7744);
  memcpy32((uint32_t *) 0x50c10700, input_40, 7744);
  memcpy32((uint32_t *) 0x50c18700, input_44, 7744);
  */

uint8_t ddata[352];
// STREAM mode
uint8_t* data = NULL;
stream_stat_t* stat;
void load_input_camera(void)
{

    uint8_t*   raw;
    uint32_t  imgLen;
    uint32_t  w, h;

#ifdef FIXED_PATTERN
    RED_VALUE -= 0;
    RED_VALUE &= 0xFF;
#endif
    // Initialize loading rows to CNN
    load_row_cnn_init();

    // Get the details of the image from the camera driver.
#ifndef PATTERN_GEN
    camera_get_image(&raw, &imgLen, &w, &h);
#else
    w = 352;
    h = 352;
#endif

    // Get image line by line
    for (int row = 0; row < h; row++)
    {
#ifndef PATTERN_GEN
			// Wait until camera streaming buffer is full
			while ((data = get_camera_stream_buffer()) == NULL) {
				if (camera_is_image_rcv()) {
					break;
				}
			};
#endif

     	  LED_Toggle(LED2);
		   load_row_cnn(data, row);
		  LED_Toggle(LED2);
		   // Release stream buffer
#ifndef PATTERN_GEN
		   release_camera_stream_buffer();
#endif
    }
#ifndef PATTERN_GEN
  //  camera_sleep(1);
    stat = get_camera_stream_statistic();

    //printf("DMA transfer count = %d\n", stat->dma_transfer_count);
    //printf("OVERFLOW = %d\n", stat->overflow_count);
    if (stat->overflow_count > 0) {
    	printf("OVERFLOW = %d\n", stat->overflow_count);
        LED_On(LED2); // Turn on red LED if overflow detected

        while (1);
    }
#endif
}

#define DIV 32   // Note 320%DIV should be 0 or you will see vertical lines between slices on TFT

void display_camera(void)
{

    uint8_t*   raw;
    uint32_t  imgLen;
    uint32_t  w, h;
    static int cnt = DIV-1;
    int xstart, offset, width;

    uint8_t r,g,b;
    uint16_t rgb;
    int j=0;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);


    // send image line by line to PC, or TFT

    // Only display TFT width
    if (w > TFT_W)
    	w=TFT_W;

    xstart = ((DIV-cnt-1)*w/DIV);
    width = w/DIV;
    offset = cnt*width*2;

    // Get image line by line
    for (int row = 0; row < h; row++)
    {
			// Wait until camera streaming buffer is full
			while ((data = get_camera_stream_buffer()) == NULL) {
				if (camera_is_image_rcv()) {
					break;
				}
			};
			LED_Toggle(LED2);
#ifndef RGB565

			j = 0;
        	// convert 888 to 565
			if (row < TFT_H)
			{
				for (int k= offset*2; k< offset*2 + 4*width; k+=4){

					r = data[k];
					g = data[k+1];
					b = data[k+2];

					//skip k+3
					rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
					data565[j++] = (rgb >> 8) & 0xFF;
					data565[j++] = rgb & 0xFF;


				}
				MXC_TFT_ShowImageCameraRGB565(xstart, Y_START + row, data565, width, 1);
			}


#else

			// Send one line to TFT
			if (row < TFT_H)
				MXC_TFT_ShowImageCameraRGB565(xstart, Y_START + row, data + offset, width, 1);
#endif
			LED_Toggle(LED2);
			// Release stream buffer
			release_camera_stream_buffer();
    }
    cnt--;
    if (cnt == -1)
    		cnt=DIV-1;
   // camera_sleep(1);
    stat = get_camera_stream_statistic();

    //printf("DMA transfer count = %d\n", stat->dma_transfer_count);
    //printf("OVERFLOW = %d\n", stat->overflow_count);
    if (stat->overflow_count > 0) {
    	printf("OVERFLOW DISP = %d\n", stat->overflow_count);
        LED_On(LED2); // Turn on red LED if overflow detected

        while (1);
    }

}


static uint32_t sum=0;
void dump_cnn(void)
{

	uint32_t * data_addr[12] = { (uint32_t *) 0x50400700, (uint32_t *) 0x50408700, (uint32_t *) 0x50410700,  (uint32_t *) 0x50418700,
								 (uint32_t *) 0x50800700, (uint32_t *) 0x50808700, (uint32_t *) 0x50810700,  (uint32_t *) 0x50818700,
								 (uint32_t *) 0x50c00700, (uint32_t *) 0x50c08700, (uint32_t *) 0x50c10700,  (uint32_t *) 0x50c18700 };

	sum=0;
	printf("\nDUMPING CNN, press PB0 \n");
	while (!PB_Get(0));
	for (int i=0; i<12; i++)
	{
		for (int j=0; j<7744; j+= 16)
		{
			//printf("\n%08X: ",data_addr[i]);
			for(int k=0; k<16; k++)
			{
				//printf("%08X ", *data_addr[i]);
				sum += *data_addr[i];
				data_addr[i]++;
			}
		}
		//printf("\n");
	}

	printf("SUM: %08X \n", sum);
//	while(1);

}

void dump_inference(void)
{

	uint32_t * data_addr[16] = {(uint32_t *) 0x50400000, (uint32_t *) 0x50408000, (uint32_t *) 0x50410000, (uint32_t *) 0x50418000,
			                    (uint32_t *) 0x50800000, (uint32_t *) 0x50808000, (uint32_t *) 0x50810000, (uint32_t *) 0x50818000,
								(uint32_t *) 0x50c00000, (uint32_t *) 0x50c08000, (uint32_t *) 0x50c10000, (uint32_t *) 0x50c18000,
								(uint32_t *) 0x51000000, (uint32_t *) 0x51008000, (uint32_t *) 0x51010000, (uint32_t *) 0x51018000,
								};


	printf("\nDUMPING INFERENCE, press PB0 \n");
	while (!PB_Get(0));
	for (int i=0; i<16; i++)
	{
		for (int j=0; j<7744; j+= 16)
		{
			printf("\n%08X: ",data_addr[i]);
			for(int k=0; k<16; k++)
			{
				printf("%08X ", *data_addr[i]);
				sum += *data_addr[i];
				data_addr[i]++;
			}
		}
		printf("\n");
	}

	printf("SUM: %08X \n", sum);
	while(1);

}

void run_camera(void)
{
    // Start capturing a first camera image frame.
    printf("Starting\n");
    camera_start_capture_image();

    while (1) {
        // Check if image is aquired.
#ifndef STREAM_ENABLE
        if (camera_is_image_rcv())
#endif
        {
            // Process the image, send it through the UART console.
            process_img();

            // Prepare for another frame capture.
            LED_Toggle(LED1);
            camera_start_capture_image();
        }
    }
}