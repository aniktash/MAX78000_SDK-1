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
    parser = argparse.ArgumentParser(description = "verifies a binary tring.out file",
                                     formatter_class = argparse.RawTextHelpFormatter)
    parser.add_argument("-i", "--input",
                        required = False, default = "trng.out",
                        help = "input binary file")

    ar = parser.parse_args()
    data = Path(ar.input).read_bytes()
    print (f'Number of bytes to verify: {len(data)}')

    cnt = 0
    last = -1
    err=0
    for k in range(len(data)//4):
        cnt += 1
        i = int.from_bytes(data[4*k:4*k+4], byteorder='little', signed=False)
        if i != last+1:
            print(f'different at {cnt}: trng:{i} expected:{last+1} ')
            err += 1
            break
        last = i

    if err == 0:
        print("No error")


if __name__ == "__main__":
    main()
