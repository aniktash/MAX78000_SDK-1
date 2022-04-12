###################################################################################################
#
# Copyright (C) 2020-2021 Maxim Integrated Products, Inc. All Rights Reserved.
#
# Maxim Integrated Products, Inc. Default Copyright Notice:
# https://www.maximintegrated.com/en/aboutus/legal/copyrights.html
#
###################################################################################################
#

import os
import sys
import argparse
import struct
from pathlib import Path


def main():
    """ main function """
    parser = argparse.ArgumentParser(description = "convert bin 2 hex txt",
                                     formatter_class = argparse.RawTextHelpFormatter)
    parser.add_argument("-i", "--input",
                        required = False, default = "streaming_NEW.out",
                        help = "input binary file")
    parser.add_argument("-o", "--output", default = "streaming_NEW_hex.txt",
                        required = False,
                        help = "output hex file in txt format")

    ar = parser.parse_args()
    data = Path(ar.input).read_bytes()  # Python 3.5+
    print (len(data))
    f = open(ar.output, "w")
    cnt = 0
    for k in range(len(data)//4):
        i = int.from_bytes(data[4*k:4*k+4], byteorder='little', signed=False)
        hex_num = '0x' + ''.join(format(i, '08x'))
        f.write(hex_num + ',')
        #print(hex(i))
       # f.write(hex(i))
       # f.write(',')
        cnt += 1
        if cnt%(88*88) == 0:
            print(cnt)
            cnt=0
            f.write('\n')

    f.close()

if __name__ == "__main__":
    main()
