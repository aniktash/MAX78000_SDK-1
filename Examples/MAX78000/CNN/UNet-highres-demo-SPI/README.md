# UNet Demo - High resolution



## Description

This demo shows a U-Net network, trained to segment images into four categories and color them as follows:

- Building: Red
- Tree: Green
- Sky: Blue
- Unknown: Black

The model is trained on 352x352 RGB images that were folded into 12 channel 88x88 images as elaborated in https://arxiv.org/pdf/2203.16528.pdf

The firmware captures the 352x352 RGB888 images from MAX78000 evkit camera and load to the cnn. The once the inference is complete, the result is sent through SPI (Slave) to the host (SPI Master).



## Building Firmware

Navigate directory where UNet-highres-demo-SPI software is located and build the project:

```bash
$ unset MAXHIM_PATH   # to make sure that the SDK accompanied by this demo is used, not the installed SDK
$ cd /Examples/MAX78000/CNN/UNet-highres-demo-SPI
$ make -r
```

If this is the first time after installing tools, or peripheral files have been updated, first clean drivers before rebuilding the project: 

```bash
$ make -r distclean
```



**Note:** To use a predefined fixed pattern as the image instead of camera, enable **#define PATTERN_GEN** in camera_util.h before building the firmware.



## Modes of operation

The firmware (SPI Slave)  supports different modes of operation, as controlled by the host application through SPI commands. 

The host application is located in `/host/qspim`:

```bash
$ cd host/qspim
```



Please reset the evkit and use following scripts to start the desired mode:

1-  **streaming**: Images are captured from camera, folded and stored in CNN, inference is performed and the result (folded inference output) is streamed out. The host can invoke this mode by  `./streaming-live.sh` to start the steaming and display the segmentation result in real-time. 

Alternatively, `./streaming.sh` can be used to store the inference result in binary format in `streaming.out` and `./display.sh` to display the segmentation result from stored binary file. 

```bash
$ ./streaming-live.sh
Display camera inference in realtime. Press any key to end.
dev no = 2
connected to FT230X Basic UART
connected to FT4222
S/N =
qp_mode_t = qp_mode_streaming_t
mode=5
in  = 00000000
out = 00000005
writable = 00007ffc
readable = 00000000
0.00 fps
3.8Mbits generated, 3.73 Mbits/sec
1.08 fps
7.6Mbits generated, 3.72 Mbits/sec
11.3Mbits generated, 3.61 Mbits/sec
0.84 fps
1.10 fps
15.1Mbits generated, 3.73 Mbits/sec
process interrupted by keypress.
```



2- **loopback (for testing)**:  The evkit sends out random data and reads back to compare. There is no inference in this mode. This mode is used to verify the basic SPI communication. The host can invoke this mode by `./loopback.sh`. 

```bash
$ ./loopback.sh
Performing QSPI loopback test. Press any key to end.
dev no = 2
connected to FT230X Basic UART
connected to FT4222
S/N =
qp_mode_t = qp_mode_loopback_t
mode=1
in  = 65726867
out = 00000001
writable = 00007ffc
readable = 00000000
0.3MB compared!, 340 kB/sec
0.7MB compared!, 354 kB/sec
1.0MB compared!, 352 kB/sec
1.4MB compared!, 337 kB/sec
1.8MB compared!, 375 kB/sec
```



3- **trng (for testing)**:  The evkit streams out an incremental 32bit counter. There is no inference in this mode. The host can invoke this mode by `./trng.sh`.  The captured binary data is stored in `trng.out` and can be verified using  ` ./trng_verify.sh`:

```bash
$ ./trng.sh
Storing random numbers to trng.out. Press any key to end.
dev no = 2
connected to FT230X Basic UART
connected to FT4222
S/N =
qp_mode_t = qp_mode_trng_t
mode=3
in  = 00000043
out = 00000003
writable = 00007ffc
readable = 00000000
4.5Mbits generated, 4.43 Mbits/sec
9.4Mbits generated, 4.81 Mbits/sec
13.4Mbits generated, 3.82 Mbits/sec
17.1Mbits generated, 3.69 Mbits/sec
20.9Mbits generated, 3.69 Mbits/sec
process interrupted by keypress.

$ ./trng_verify.sh
Number of bytes to verify: 3078912
No error
```



4- **cnn_in (for testing)**: The folded stored images in the CNN memory are streamed out. The host can invoke this mode by `./cnn_in.sh`.  The captured binary data is stored in `cnn_in.out`. This mode can be used to verify that the images are folded and stored correctly in the CNN memory. No inference is performed in this mode. To unfold and display the stored data, use  `./display-cnn_in.out`:

```bash
$ ./cnn_in.sh
Storing cnn input to a file. Press any key to end.
dev no = 2
connected to FT230X Basic UART
connected to FT4222
S/N =
qp_mode_t = qp_mode_cnn_in_t
mode=6
in  = 014afa38
out = 00000006
writable = 00007ffc
readable = 00000000
4.0Mbits generated, 3.84 Mbits/sec
8.2Mbits generated, 4.04 Mbits/sec
12.7Mbits generated, 4.43 Mbits/sec
process interrupted by keypress.

$ ./display-cnn_in.sh
cnn_in.out
start from frame 1 of 4
start from frame 2 of 4
start from frame 3 of 4
start from frame 4 of 4
```

