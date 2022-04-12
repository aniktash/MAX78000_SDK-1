#!/bin/sh
PATH=${PATH}:../LibFT4222/imports/LibFT4222/lib/i386
echo Performing QSPI loopback test.  Press any key to end.
./Release/qspim.exe loopback
