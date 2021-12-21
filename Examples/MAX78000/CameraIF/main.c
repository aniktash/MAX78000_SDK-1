/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All Rights Reserved.
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
******************************************************************************/

/**
 * @file    main.c
 * @brief   Parallel camera example with the OV7692/OV5642/HM01B0/HM0360 camera sensors as defined in the makefile.
 *
 * @details This example uses the UART to stream out the image captured from the camera.
 *          Alternatively, it can display the captured image on TFT is it is enabled in the make file.
 *          The image is prepended with a header that is interpreted by the grab_image.py
 *          python script.  The data from this example through the UART is in a binary format.
 *          Instructions: 1) Load and execute this example. The example will initialize the camera
 *                        and start the repeating binary output of camera frame data.
 *                        2) Run 'sudo grab_image.py /dev/ttyUSB0 115200'
 *                           Substitute the /dev/ttyUSB0 string for the serial port on your system.
 *                           The python program will read in the binary data from this example and
 *                           output a png image.
 */

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
#include "utils.h"
#include "dma.h"

#ifdef ENABLE_TFT  // defined in make file
#include "tft.h"
#endif

#define CAMERA_FREQ (10 * 1000 * 1000)

#if defined(CAMERA_HM01B0)
#define IMAGE_XRES  324/2
#define IMAGE_YRES  244/2
#define CAMERA_MONO
//#define STREAM_ENABLE
#endif

#if defined(CAMERA_HM0360)
#define IMAGE_XRES  320
#define IMAGE_YRES  240
#define CAMERA_MONO
//#define STREAM_ENABLE
#endif

#if defined(CAMERA_OV7692) || defined(CAMERA_OV5642)
#define IMAGE_XRES  80// 120//352//352
#define IMAGE_YRES  80// 120//241
#define STREAM_ENABLE  // If enabled, camera is setup in streaming mode to send the image line by line
// to TFT, or serial port as they are captured. Otherwise, it buffers the entire image first and
// then sends to TFT or serial port.
// With serial port set at 900kbps, it can stream for up to 80x80 with OV5642 camera in stream mode.
// or 176x144 when stream mode is disabled.
// It can display on TFT up to 176x144 if stream mode is disabled, or 320x240 if enabled.
#endif

#define CON_BAUD 115200*8   //UART baudrate used for sending data to PC, use max 921600 for serial stream
#define X_START     0
#define Y_START     0

void process_img(void)
{
    uint8_t*   raw;
    uint32_t  imgLen;
    uint32_t  w, h;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);

#ifndef STREAM_ENABLE
    // buffers the entire image and send to serial or TFT
#ifndef ENABLE_TFT
    // Send the image through the UART to the console if no TFT
    // A python program will read from the console and write
    // to an image file
    utils_send_img_to_pc(raw, imgLen, w, h, camera_get_pixel_format());
#else
#ifndef CAMERA_MONO
    // Send the image to TFT
    MXC_TFT_ShowImageCameraRGB565(X_START, Y_START, raw, h, w);
#else
    MXC_TFT_ShowImageCameraMono(X_START, Y_START, raw, h, w);
#endif // #ifndef CAMERA_MONO
#endif // ##ifndef ENABLE_TFT

#else // #ifndef STREAM_ENABLE
    // STREAM mode
    uint8_t* data = NULL;
    // send image line by line to PC, or TFT
#ifndef ENABLE_TFT
    // initialize the communication by providing image format and size
    utils_stream_img_to_pc_init(raw, imgLen, w, h, camera_get_pixel_format());
#endif

    if (w>320)
    	w=320;
    // Get image line by line
    for (int i = 0; i < h; i++) {

        // Wait until camera streaming buffer is full
        while ((data = get_camera_stream_buffer()) == NULL) {
            if (camera_is_image_rcv()) {
                break;
            }
        };

#ifndef ENABLE_TFT
        // Send one line to PC
        utils_stream_image_row_to_pc(data, w * 2);

#else
#ifndef CAMERA_MONO
        // Send one line to TFT
        if (i<240)
        	MXC_TFT_ShowImageCameraRGB565(X_START, Y_START + i, data, w, 1);

#else
        MXC_TFT_ShowImageCameraMono(X_START, Y_START + i, data, w, 1);

#endif
#endif
        // Release stream buffer
        release_camera_stream_buffer();
    }

    stream_stat_t* stat = get_camera_stream_statistic();

    //printf("DMA transfer count = %d\n", stat->dma_transfer_count);
    //printf("OVERFLOW = %d\n", stat->overflow_count);
    if (stat->overflow_count > 0) {
        LED_On(LED2); // Turn on red LED if overflow detected

        while (1);
    }

#endif //#ifndef STREAM_ENABLE
}

uint8_t data565[IMAGE_XRES*IMAGE_YRES*2];

void process_img_888(void)
{
    uint8_t*   raw;
    uint32_t  imgLen;
    uint32_t  w, h;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);

#ifndef STREAM_ENABLE
    // buffers the entire image and send to serial or TFT
#ifndef ENABLE_TFT
    // Send the image through the UART to the console if no TFT
    // A python program will read from the console and write
    // to an image file
    utils_send_img_to_pc(raw, imgLen, w, h, camera_get_pixel_format());
#else
#ifndef CAMERA_MONO
    // Send the image to TFT
    uint8_t r,g,b;
    uint16_t rgb;
    int j=0;
	// convert 888 to 565
#if 0 // FIFO_THREE_BYTE - original implementation of camera
	for (int k=0; k<3*w*h; k+=3){
		r =  raw[k];
		g =  raw[k+1];
		b =  raw[k+2];
		rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
		data565[j++] = (rgb >> 8) & 0xFF;
		data565[j++] = rgb & 0xFF;

	}
#else  // FIFO_FOUR_BYTE //  r=raw[k] but k+2 and k+3 are not right
	for (int k=0; k<4*w*h; k+=4){
		r =  raw[k];
		g =  raw[k+1];
		b =  raw[k+2];
		//skip k+3
		rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
		data565[j++] = (rgb >> 8) & 0xFF;
		data565[j++] = rgb & 0xFF;

	}

#endif

	///MXC_TFT_ShowImageCameraRGB565(X_START, Y_START, raw, h, w);
    MXC_TFT_ShowImageCameraRGB565(X_START, Y_START, data565, h, w);
#else
    MXC_TFT_ShowImageCameraMono(X_START, Y_START, raw, h, w);
#endif // #ifndef CAMERA_MONO
#endif // ##ifndef ENABLE_TFT

#else // #ifndef STREAM_ENABLE
    // STREAM mode
    uint8_t* data = NULL;
    // send image line by line to PC, or TFT
#ifndef ENABLE_TFT
    // initialize the communication by providing image format and size
    utils_stream_img_to_pc_init(raw, imgLen, w, h, camera_get_pixel_format());
#endif

    if (w>320)
    	w=320;
    // Get image line by line
    for (int i = 0; i < h; i++) {

        // Wait until camera streaming buffer is full
        while ((data = get_camera_stream_buffer()) == NULL) {
            if (camera_is_image_rcv()) {
                break;
            }
        };

#ifndef ENABLE_TFT
        // Send one line to PC
        utils_stream_image_row_to_pc(data, w * 2);

#else
#ifndef CAMERA_MONO
        // Send one line to TFT
        if (i<240)
        {
            uint8_t r,g,b;
            uint16_t rgb;
            int j=0;

          //  for (int k=0;k<w; k++)
           // 	printf("%02X", data[k]);
           // printf("\n");

#if 0 // we always have 4 bytes in 888
        	// convert 888 to 565
        	for (int k=0; k<3*w; k+=3){

        		//printf("%02X%02X%02X%02X    ", data[k], data[k+1], data[k+2], data[k+3]);

        		r = data[k]; //OK
        		g = data[k+1]; //OK
        		b = data[k+2];


        		/*
        		b =  data[k];
        		g =  data[k+1];
        		r =  data[k+2];
*/
        		rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        		data565[j++] = (rgb >> 8) & 0xFF;
        		data565[j++] = rgb & 0xFF;

        	}
        	printf("\n");
#else
        	// convert 888 to 565
        	for (int k=0; k<4*w; k+=4){

        		//printf("%02X%02X%02X%02X    ", data[k], data[k+1], data[k+2], data[k+3]);

        		r = data[k];
        		g = data[k+1];
        		b = data[k+2];

        		//skip k+3
        		rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        		data565[j++] = (rgb >> 8) & 0xFF;
        		data565[j++] = rgb & 0xFF;

        		/*
        		b =  data[k];
        		g =  data[k+1];
        		r =  data[k+2];
*/
        		rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        		data565[j++] = (rgb >> 8) & 0xFF;
        		data565[j++] = rgb & 0xFF;

        	}
        	printf("\n");

#endif

        	MXC_TFT_ShowImageCameraRGB565(X_START, Y_START + i, data565, w, 1);
        }

#else
        MXC_TFT_ShowImageCameraMono(X_START, Y_START + i, data, w, 1);

#endif
#endif
        // Release stream buffer
        release_camera_stream_buffer();
    }

    stream_stat_t* stat = get_camera_stream_statistic();

    //printf("DMA transfer count = %d\n", stat->dma_transfer_count);
    //printf("OVERFLOW = %d\n", stat->overflow_count);
    if (stat->overflow_count > 0) {
        LED_On(LED2); // Turn on red LED if overflow detected

        while (1);
    }

#endif //#ifndef STREAM_ENABLE
}



// *****************************************************************************
int main(void)
{
    int ret = 0;
    int slaveAddress;
    int id;
    int dma_channel;

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC0);

    /* Set system clock to 100 MHz */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    // Initialize DMA for camera interface
    MXC_DMA_Init();
    dma_channel = MXC_DMA_AcquireChannel();

    mxc_uart_regs_t* ConsoleUart = MXC_UART_GET_UART(CONSOLE_UART);

    if ((ret = MXC_UART_Init(ConsoleUart, CON_BAUD, MXC_UART_IBRO_CLK)) != E_NO_ERROR) {
        return ret;
    }


#ifdef ENABLE_TFT
    printf("Init TFT\n");
    /* Initialize TFT display */
    MXC_TFT_Init(MXC_SPI0, 1, NULL, NULL);
    MXC_TFT_SetBackGroundColor(4);
#endif


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


    //MXC_Delay(SEC(2));
   LED_Toggle(LED1);
   printf("Init Camera\n");

    // Setup the camera image dimensions, pixel format and data acquiring details.
#ifndef STREAM_ENABLE
#ifndef CAMERA_MONO
    ///ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB565, FIFO_FOUR_BYTE, USE_DMA, dma_channel); // RGB565

   ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB888, FIFO_THREE_BYTE, USE_DMA, dma_channel); //OK
	//ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB888, FIFO_FOUR_BYTE, USE_DMA, dma_channel);   // data is 3 bytes, invalid case
#else
    ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_BAYER, FIFO_FOUR_BYTE, USE_DMA, dma_channel); // Mono
#endif

#ifdef ENABLE_TFT
    /* Set the screen rotation */
    MXC_TFT_SetRotation(SCREEN_ROTATE);
    /* Change entry mode settings */
    MXC_TFT_WriteReg(0x0011, 0x6858);
#endif
#else
#ifndef CAMERA_MONO
    ///ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB565, FIFO_FOUR_BYTE, STREAMING_DMA, dma_channel); // RGB565 stream
	ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB888, FIFO_THREE_BYTE, STREAMING_DMA, dma_channel); // working

#else
    ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_BAYER, FIFO_FOUR_BYTE, STREAMING_DMA, dma_channel); // Mono stream
#endif

#ifdef ENABLE_TFT
    /* Set the screen rotation */
    MXC_TFT_SetRotation(SCREEN_FLIP);
#endif
#endif //#ifndef STREAM_ENABLE

    if (ret != STATUS_OK) {
        printf("Error returned from setting up camera. Error %d\n", ret);
        return -1;
    }

    MXC_Delay(SEC(1));

#if defined(CAMERA_OV7692) && defined(STREAM_ENABLE)
    ///camera_write_reg(0x11, 0x6); // set camera clock prescaller to prevent streaming overflow for QVGA
    camera_write_reg(0x11, 0xF); // set camera clock prescaller to prevent streaming overflow for QVGA
#endif

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
            ///process_img();
        	process_img_888();

            // Prepare for another frame capture.
            LED_Toggle(LED1);
            camera_start_capture_image();
        }
    }

    return ret;
}
