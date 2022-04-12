#!/usr/bin/env python
import argparse
import numpy as np
from matplotlib import pyplot as plt
from PIL import Image
from pathlib import Path


def get_indata(data_path):
    with open(data_path, 'r') as f:
        data = f.readlines()
    buf = []
    cnt = 0
    for line in data:
        splitted_line = line.split(',')
        for el in splitted_line:
            el = el.replace(" ", "")
            if el.startswith('0x'):
                buf[-1].append(el[:10])
            elif el.startswith('#define'):
                buf.append([])
    return buf


def get_outdata(data_path):
    segment_len_idx = 3
    segment_len = 0
    segment = []
    segment_list = []

    with open(data_path) as f:
        while True:
            line = f.readline()
            if not line:
                segment_list.append(segment)
                segment = []
                break
            if line[0] != "#" and line[0] != "}":
                tmp = line[0: len(line) - 1].rstrip()  # remove EOL
                #                 tmp = tmp.replace("0x", "").replace(" ", "")  # remove 0x
                tmp = tmp.replace(" ", "")
                tmp = tmp.rstrip(", \\")  # remove \
                tmp_list = tmp.rstrip(" \\").split(",")  # remove , at the end of line

                while tmp_list:
                    if segment_len == 0:
                        if len(tmp_list) >= segment_len_idx:
                            segment_len = int(tmp_list[segment_len_idx - 1], 16)
                            tmp_list = tmp_list[segment_len_idx:]
                            segment_len_idx = 3
                            segment_list.append(segment)
                            segment = []
                        else:
                            segment_len_idx -= len(tmp_list)
                            tmp_list = []

                    if segment_len >= len(tmp_list):
                        segment.extend(tmp_list)
                        segment_len -= len(tmp_list)
                        tmp_list = []
                    else:
                        segment.extend(tmp_list[:segment_len])
                        tmp_list = tmp_list[segment_len:]
                        segment_len = 0

    out_data = segment_list[1:]
    return out_data


def read_from_hex_list(buf, num_blocks, image_dim):
    inp = -1 * np.ones((4 * num_blocks, image_dim, image_dim))
    idx = 0
    for x in range(image_dim):
        for y in range(image_dim):
            ind = image_dim * x + y
            for i in range(num_blocks):
                for j in range(4):
                    jj = 3 - j
                    if jj != 3:
                        inp[i * 4 + j][x][y] = int(buf[i][ind][-8 + (2 * jj):-6 + (2 * jj)],
                                                   16)
                    else:
                        inp[i * 4 + j][x][y] = int(buf[i][ind][-2:], 16)
                    # print(f'{i*4+j} {x} {y}  = {inp[i * 4 + j][x][y]}')
    return inp


def read_from_bin(buf, num_blocks, image_dim):
    inp = -1 * np.ones((4 * num_blocks, image_dim, image_dim))
    idx = 0
    for x in range(image_dim):
        for y in range(image_dim):
            ind = image_dim * x + y
            for i in range(num_blocks):
                for j in range(4):
                    inp[i * 4 + j][x][y] = buf[i * image_dim * image_dim * 4 + ind*4 + j]
                    # print(f'{i * 4 + j} {x} {y}  = {inp[i * 4 + j][x][y]}')
    return inp


def create_color_map(unfolded_inp, im_size):
    out_vals = np.argmax(unfolded_inp[:, :, :], axis = 0)
    colors = np.zeros((im_size, im_size, 3), dtype = np.uint8)

    for ii, row in enumerate(out_vals):
        for j, val in enumerate(row):
            if val != 3:
                if val == 1:
                    val = 2
                elif val == 2:
                    val = 1
                colors[ii, j, val] = 255

    return colors


def fold_image(inp, fold_ratio):
    img_folded = None
    for i in range(fold_ratio):
        for j in range(fold_ratio):
            if img_folded is not None:
                img_folded = np.concatenate((img_folded, inp[i::fold_ratio, j::fold_ratio, :]),
                                            axis = 2)
            else:
                img_folded = inp[i::fold_ratio, j::fold_ratio, :]

    return img_folded


def unfold_image(inp, fold_ratio, num_channels):
    if fold_ratio == 1:
        return inp
    dec0_u = np.zeros((num_channels, inp.shape[1] * fold_ratio, inp.shape[2] * fold_ratio))
    for i in range(fold_ratio):
        for j in range(fold_ratio):
            dec0_u[:, i::fold_ratio, j::fold_ratio] = inp[num_channels * (
                        i * fold_ratio + j):num_channels * (i * fold_ratio + j + 1), :, :]
    return dec0_u


def read_output_from_txtfile(file_name, index):
    with open(file_name, 'r') as f:
        out = f.readlines()
    print(len(out))
    out= out[index*16:index*16+16]
    print(len(out))
    buf = []
    for i in range(len(out)):
        buf.append([])
        buf[-1].extend(out[i].split(',')[:-1])

    output = buf
    im_size = 88
    output = read_from_hex_list(output, 16, im_size)
    output = output.astype(np.int8)
    unfolded_inp = unfold_image(output, 4, 4)
    im_size = 352
    colors = create_color_map(unfolded_inp, im_size)
    return colors


def read_output_from_binfile(file_name, index):
    out = Path(file_name).read_bytes()
    im_size = 88

    offset = index*im_size*im_size*16*4
    print(f'start from frame {index + 1} of {len(out) //(im_size*im_size*16*4)}')

    output = read_from_bin(out[offset:], 16, im_size)
    output = output.astype(np.int8)
    unfolded_inp = unfold_image(output, 4, 4)
    im_size = 352
    colors = create_color_map(unfolded_inp, im_size)
    return colors


def main():
    """ main function """
    parser = argparse.ArgumentParser(description =
                                     "displays inference result from bin or hex text file ",
                                     formatter_class = argparse.RawTextHelpFormatter)
    parser.add_argument("-i", "--input",
                        required = False, default = "streaming.out",
                        help = "input binaray or hex text file")
    parser.add_argument("-b", "--binary", action = 'store_true',
                        default = True,
                        required = False,
                        help = "if the inputfile is binary")
    parser.add_argument("-c", "--count", type = int,
                        required = False,
                        help = "index of the frame to display")

    ar = parser.parse_args()

    if ar.count is not None:
        if ar.binary:
            colors = read_output_from_binfile(ar.input, ar.count)
        else:
            colors = read_output_from_txtfile(ar.input, ar.count)
        plt.imshow(colors)
        plt.show(block = False)
        # plt.pause(1)
        plt.pause(10)
      
    else:
        for i in range(99999): # large number, will break at the end
            try:
                colors = read_output_from_binfile(ar.input, i)
            except:
                break
            plt.imshow(colors)
            plt.title(str(i))
            plt.show(block = False)
            plt.pause(.3)


if __name__ == "__main__":
    main()
