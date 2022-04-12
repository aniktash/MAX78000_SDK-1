#!/bin/sh
PATH=${PATH}:../LibFT4222/imports/LibFT4222/lib/i386
echo Performing AES-256 CBC encrypt/decrypt test
echo Clear input is:  clear.in
echo Encypted output is:  crypto.bin
echo Decrypted output is:  clear.out

qspim.exe encrypt clear.in crypto.bin
qspim.exe decrypt crypto.bin clear.out

