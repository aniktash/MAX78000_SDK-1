#!/usr/bin/env python
# coding: utf-8

# In[9]:


import crc8
import numpy as np
from matplotlib import pyplot as plt
from PIL import Image
import serial
from utils_fast_New import *


# In[10]:


ser = serial.Serial('COM54')
ser.baudrate = 115200
ser.timeout = 0.001


# In[11]:

#image_path = 'sample_86.png'
image_path = 'image1.png'
# image_path = 'sample_data_repo.png'
# image_path = '../118_India_Im.png'

# just a filename to save the output
output_filename = 'CNNout_sampledata3.txt'
# output_filename = 'CNNOut_118_India_Im.txt'


# In[12]:


# read the original image and display it
im = Image.open(image_path)
img = np.asarray(im)
print(img.min(), img.max())
#plt.imshow(img)
#plt.title('Original Image')
#plt.show()

# resize the image
newsize = (352, 352)
im_resize = im.resize((newsize), Image.LANCZOS)
img_resize = np.asarray(im_resize)[:, :, :3]
#plt.imshow(img_resize)
#plt.title('Resized Image')
#plt.show()

if True:
    img_resize1=np.ndarray(shape=(352,352,3), dtype=np.uint8)
    for i in range(352):
        for j in range(352):
            for k in range (3):
                if k== 0:
                    img_resize1[i, j, k] = 133
                if k ==1:
                    img_resize1[i, j, k] = j >> 1
                if k == 2:
                    img_resize1[i, j, k] = i >> 1
                # img_resize1[i,j,k] = 100
else:
    img_resize1 = img_resize

plt.imshow(img_resize1)
plt.title('Resized Image1')
plt.show()

# fold the image
folded_img = fold_image(img_resize1, 4)
folded_img = np.transpose(folded_img, (2, 0, 1))

# send the folded image to device and save the received result to output_filename
print(folded_img.min(), folded_img.max())
send_image_receive_output(ser, folded_img, output_filename)
#send_image_receive_output(ser, None, output_filename)

# read from output_filename and create the predicted map
colors = read_output_from_txtfile(output_filename)



# In[13]:


f, ax = plt.subplots(1, 3, figsize=(12, 4))
ax[0].imshow(img_resize)
ax[1].imshow(colors)
ax[2].imshow(img_resize)
ax[2].imshow(colors, alpha=0.2)
plt.show() 


# In[ ]:




