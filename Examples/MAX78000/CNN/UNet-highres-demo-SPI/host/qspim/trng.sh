#!/bin/sh
PATH=${PATH}:../LibFT4222/imports/LibFT4222/lib/i386
echo Storing random numbers to trng.out.  Press any key to end.
./Release/qspim.exe trng trng.out
