#!/usr/bin/env python

import numpy as np
from skimage import filter,exposure,io
import skimage
import os
import re, shutil

orig_dir = 'MiddEval3'
target_dir = 'MiddEval3_noise'

input_gamma = 2.2
output_gamma = 1.0  #1.0 means do nothing
gaussian_sigma = 0.8
well_capacity = 1600.0
read_noise = 2.0
color_correction = np.array([[1.6013  ,-0.4631, -0.1382 ],[-0.2511, 1.6393,  -0.3882 ],[0.0362,  -0.5823, 1.5461 ]])
#color_correction = np.array([[1,0,0],[0,1,0],[0,0,1]])
cc_inv = np.linalg.pinv(color_correction)

def check_and_make_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)


def add_noise(img):
    img = skimage.img_as_float(img)
    img = img.dot(cc_inv)
    img = exposure.adjust_gamma(img,input_gamma)
    img = filter.gaussian_filter(img,gaussian_sigma,multichannel=True)

    img *= well_capacity
    img = np.random.poisson(img).astype(np.float64)

    #img += np.random.poisson(np.ones(img.shape)*read_noise).astype(np.float32)
    img += (np.random.standard_normal(img.shape)*read_noise).astype(np.float64)

    img /= well_capacity

    img = np.clip(img,0.0,1.0)
    img = exposure.adjust_gamma(img,output_gamma)
    return img


for f1 in os.listdir(orig_dir):
    check_and_make_dir(target_dir)
    for f2 in os.listdir(os.path.join(orig_dir,f1)):
        check_and_make_dir(os.path.join(target_dir,f1,f2))
        for file in os.listdir(os.path.join(orig_dir,f1,f2)):
            if file == 'im0.png' or file == 'im1.png':
                img = io.imread(os.path.join(orig_dir,f1,f2,file))
                img = add_noise(img)
                io.imsave(os.path.join(target_dir,f1,f2,file),img)
            else:
                shutil.copy(os.path.join(orig_dir,f1,f2,file),os.path.join(target_dir,f1,f2,file))

