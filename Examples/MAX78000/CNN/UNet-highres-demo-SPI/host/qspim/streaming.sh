#!/bin/sh
PATH=${PATH}:../LibFT4222/imports/LibFT4222/lib/i386
echo Storing camera inference to a file.  Press any key to end.
./Release/qspim.exe streaming streaming.out
