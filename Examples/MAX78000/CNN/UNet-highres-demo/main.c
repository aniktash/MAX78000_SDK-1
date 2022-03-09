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

// unet_v7_fake_passthr
// Created using ./ai8xize.py --verbose --test-dir sdk/Examples/MAX78000/CNN --prefix unet_v7_fake_passthr --checkpoint-file unet/qat_ai85unet_v7_352_4_best_fake_passthr_q.pth.tar --config-file unet/unet_v7_fake_passthr.yaml --device MAX78000 --compact-data --mexpress --display-checkpoint --sample-input unet/camvid_sample_352_4.npy --overwrite --overlap-data --mlator --no-unload

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mxc.h"
#include "cnn.h"

#include "led.h"
#include "camera.h"
//#include "sampledata.h"
//#include "sampleoutput.h"


#define USE_CAMERA   // if enabled, it uses the camera specified in the make file, otherwise it uses serial loader


//------------------------------------------------------------
#if 1//def USE_CAMERA
#include "camera_util.h"
#endif
//------------------------------------------------------------


volatile uint32_t cnn_time; // Stopwatch

#define CON_BAUD 115200
#define NUM_PIXELS 7744 // 88x88
#define NUM_IN_CHANNLES 48
#define NUM_OUT_CHANNLES 64

void fail(void)
{
  printf("\n*** FAIL ***\n\n");
  while (1);
}

// 48-channel 88x88 data input (371712 bytes total / 7744 bytes per channel):
// HWC 88x88, channels 0 to 3
/*static const uint32_t input_0[] = SAMPLE_INPUT_0;

// HWC 88x88, channels 4 to 7
static const uint32_t input_4[] = SAMPLE_INPUT_4;

// HWC 88x88, channels 8 to 11
static const uint32_t input_8[] = SAMPLE_INPUT_8;

// HWC 88x88, channels 12 to 15
static const uint32_t input_12[] = SAMPLE_INPUT_12;

// HWC 88x88, channels 16 to 19
static const uint32_t input_16[] = SAMPLE_INPUT_16;

// HWC 88x88, channels 20 to 23
static const uint32_t input_20[] = SAMPLE_INPUT_20;

// HWC 88x88, channels 24 to 27
static const uint32_t input_24[] = SAMPLE_INPUT_24;

// HWC 88x88, channels 28 to 31
static const uint32_t input_28[] = SAMPLE_INPUT_28;

// HWC 88x88, channels 32 to 35
static const uint32_t input_32[] = SAMPLE_INPUT_32;

// HWC 88x88, channels 36 to 39
static const uint32_t input_36[] = SAMPLE_INPUT_36;

// HWC 88x88, channels 40 to 43
static const uint32_t input_40[] = SAMPLE_INPUT_40;

// HWC 88x88, channels 44 to 47
static const uint32_t input_44[] = SAMPLE_INPUT_44;*/


int console_UART_init(uint32_t baud)
{
  mxc_uart_regs_t* ConsoleUart = MXC_UART_GET_UART(CONSOLE_UART);
  int err;
  NVIC_ClearPendingIRQ(MXC_UART_GET_IRQ(CONSOLE_UART));
  NVIC_DisableIRQ(MXC_UART_GET_IRQ(CONSOLE_UART));
  NVIC_SetPriority(MXC_UART_GET_IRQ(CONSOLE_UART), 1);
  NVIC_EnableIRQ(MXC_UART_GET_IRQ(CONSOLE_UART));

  if ((err = MXC_UART_Init(ConsoleUart, baud, MXC_UART_IBRO_CLK)) != E_NO_ERROR) {
    return err;
  }
  
  return 0;
}


uint8_t gen_crc(const void* vptr, int len) {
  const uint8_t *data = vptr;
  unsigned crc = 0;
  int i, j;
  for (j = len; j; j--, data++) {
    crc ^= (*data << 8);
    for(i = 8; i; i--) {
      if (crc & 0x8000)
        crc ^= (0x1070 << 3);
      crc <<= 1;
    }
  }
  return (uint8_t)(crc >> 8);
}

static void console_uart_send_byte(uint8_t value)
{
    while (MXC_UART_WriteCharacter(MXC_UART_GET_UART(CONSOLE_UART), value) == E_OVERFLOW) { }
}

static void console_uart_send_bytes(uint8_t* ptr, int length)
{
    int i;
    
    for (i = 0; i < length; i++) {
      console_uart_send_byte(ptr[i]);
      //printf("%d\n", ptr[i]);
    }
}



void load_input_serial(void)
{
  uint32_t in_data[NUM_PIXELS];
  uint8_t rxdata[4];
  uint32_t tmp;
  uint8_t crc, crc_result;
  uint32_t index = 0;
  LED_Off(LED2);

  printf("READY\n");

  uint32_t * data_addr = (uint32_t *) 0x50400700;
  for (int ch=0; ch < NUM_IN_CHANNLES; ch +=4){
    LED_Toggle(LED1);
    for (int i = 0; i < NUM_PIXELS; i++) {
      index++;
      tmp = 0;

      for (int j=0; j<4; j++)
      {
        rxdata[j] = MXC_UART_ReadCharacter(MXC_UART_GET_UART(CONSOLE_UART));
        tmp = tmp | (rxdata[j] << 8*(3-j));
      }
#if 1
      //read crc
      crc = MXC_UART_ReadCharacter(MXC_UART_GET_UART(CONSOLE_UART));
      crc_result = gen_crc(rxdata, 4);
      if ( crc != crc_result ) {
        printf("E %d",index);
        LED_On(LED2);
        while(1);
      }
#endif
      //fill input buffer
      in_data[i] = tmp;
    }

	// load data to cnn
    memcpy32(data_addr, in_data, NUM_PIXELS);
   // printf("%d- %08X \n",ch,data_addr);
    data_addr += 0x2000;
    if ((data_addr == (uint32_t *)0x50420700) || (data_addr == (uint32_t *)0x50820700) || (data_addr == (uint32_t *)0x50c20700)){
      data_addr += 0x000f8000;
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
}


// Expected output of layer 18 for unet_v7_fake_passthr given the sample input (known-answer test)
// Delete this function for production code
//static const uint32_t sample_output[] = SAMPLE_OUTPUT;
int check_output(void)
{
  int i;
  uint32_t mask, len;
  volatile uint32_t *addr;
  const uint32_t *ptr = 0;//sample_output;

  while ((addr = (volatile uint32_t *) *ptr++) != 0) {
    mask = *ptr++;
    len = *ptr++;
    for (i = 0; i < len; i++)
      if ((*addr++ & mask) != *ptr++) return CNN_FAIL;
  }

  return CNN_OK;
}

void send_output(void)
{
  uint8_t *data_addr = (uint8_t *) 0x50400000;
  
  printf("SENDING_OUTPUT\n");

  for (int ch=0; ch < NUM_OUT_CHANNLES; ch +=4){
    console_uart_send_bytes(data_addr, 4*NUM_PIXELS);
    data_addr += 0x8000;
    if ((data_addr == (uint8_t *)0x50420000) || (data_addr == (uint8_t *)0x50820000) || (data_addr == (uint8_t *)0x50c20000)){
      data_addr += 0x003e0000;
    }
  }
}

int main(void)
{

  MXC_ICC_Enable(MXC_ICC0); // Enable cache

  // Switch to 100 MHz clock
  MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
  SystemCoreClockUpdate();

  // Initialize UART
  console_UART_init(CON_BAUD);

  printf("Waiting...\n");


#ifdef USE_CAMERA
  initialize_camera();
  //run_camera();
  
#else
  printf("Start SerialLoader.py script...\n");
#endif


#if ENABLE_TFT
    printf("Init TFT\n");
    /* Initialize TFT display */
    MXC_TFT_Init(MXC_SPI0, 1, NULL, NULL);
    printf("set TFT back color\n");
    MXC_TFT_SetBackGroundColor(4);

    /* Set the screen rotation */
   // MXC_TFT_SetRotation(SCREEN_FLIP);

#endif


  // DO NOT DELETE THIS LINE:
  MXC_Delay(SEC(2)); // Let debugger interrupt if needed

  // Enable peripheral, enable CNN interrupt, turn on CNN clock
  // CNN clock: 50 MHz div 1
  cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);

  printf("\n*** CNN Inference Test ***\n");

  cnn_init(); // Bring state machine into consistent state
  cnn_load_weights(); // Load kernels
  cnn_load_bias();
  cnn_configure(); // Configure state machine

#ifdef USE_CAMERA
  // Start getting images from camera and processing them
  printf("Start capturing\n");
  camera_start_capture_image();
#endif


  while (1)
  {
	      LED_Toggle(LED1);

		#ifndef USE_CAMERA
		  load_input_serial(); // Load data input from serial port
		#else
		  load_input_camera(); // Load data input from camera
		#endif


#ifdef PATTERN_GEN
		  dump_cnn();
#endif


#if 1
		  camera_sleep(0); // disable sleep
		  camera_start_capture_image(); // next frame


		  cnn_start(); // Start CNN processing


		  display_camera();


		  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // SLEEPDEEP=0
		  while (cnn_time == 0)
			__WFI(); // Wait for CNN

#endif
		  camera_sleep(0); // disable sleep
		  camera_start_capture_image();

		  if (PB_Get(0))
		  {
		  	  #ifdef CNN_INFERENCE_TIMER
			  printf("\n*** Approximate inference time: %u us ***\n\n", cnn_time);
		  	  #endif
		  }

  }
  


}

/*
  SUMMARY OF OPS
  Hardware: 631,176,656 ops (627,697,664 macc; 3,478,992 comp; 0 add; 0 mul; 0 bitwise)
    Layer 0: 24,285,184 ops (23,789,568 macc; 495,616 comp; 0 add; 0 mul; 0 bitwise)
    Layer 1: 32,215,040 ops (31,719,424 macc; 495,616 comp; 0 add; 0 mul; 0 bitwise)
    Layer 2: 16,107,520 ops (15,859,712 macc; 247,808 comp; 0 add; 0 mul; 0 bitwise)
    Layer 3: 17,904,128 ops (17,842,176 macc; 61,952 comp; 0 add; 0 mul; 0 bitwise)
    Layer 4: 4,019,136 ops (3,902,976 macc; 116,160 comp; 0 add; 0 mul; 0 bitwise)
    Layer 5: 6,911,520 ops (6,830,208 macc; 81,312 comp; 0 add; 0 mul; 0 bitwise)
    Layer 6: 6,870,864 ops (6,830,208 macc; 40,656 comp; 0 add; 0 mul; 0 bitwise)
    Layer 7: 1,517,824 ops (1,517,824 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 8: 27,320,832 ops (27,320,832 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 9: 27,347,936 ops (27,320,832 macc; 27,104 comp; 0 add; 0 mul; 0 bitwise)
    Layer 10: 27,320,832 ops (27,320,832 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 11: 27,375,040 ops (27,320,832 macc; 54,208 comp; 0 add; 0 mul; 0 bitwise)
    Layer 12: 15,611,904 ops (15,611,904 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 13: 53,898,240 ops (53,526,528 macc; 371,712 comp; 0 add; 0 mul; 0 bitwise)
    Layer 14: 214,601,728 ops (214,106,112 macc; 495,616 comp; 0 add; 0 mul; 0 bitwise)
    Layer 15: 32,215,040 ops (31,719,424 macc; 495,616 comp; 0 add; 0 mul; 0 bitwise)
    Layer 16: 32,215,040 ops (31,719,424 macc; 495,616 comp; 0 add; 0 mul; 0 bitwise)
    Layer 17: 31,719,424 ops (31,719,424 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 18: 31,719,424 ops (31,719,424 macc; 0 comp; 0 add; 0 mul; 0 bitwise)

  RESOURCE USAGE
  Weight memory: 281,312 bytes out of 442,368 bytes total (64%)
  Bias memory:   908 bytes out of 2,048 bytes total (44%)
*/
