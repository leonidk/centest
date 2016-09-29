#!/usr/bin/env python

import json
import os, sys
from PIL import Image
import numpy as np
from subprocess import call
import re, shutil, os
from util import *

config = get_default_config()
config['description'] = "all of KITTI 2012"

basePath = './data_stereo_flow/training/'
targetPath = 'kitti2012'
def check_and_make_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)
check_and_make_dir(targetPath)
for filename in os.listdir(os.path.join(basePath,'disp_noc')):
    lcfolder =  filename[:-7]

    nfolder = os.path.join(targetPath,lcfolder)
    check_and_make_dir(nfolder)
    check_and_make_dir(os.path.join(nfolder,'left'))
    check_and_make_dir(os.path.join(nfolder, 'right'))
    check_and_make_dir(os.path.join(nfolder, 'gt'))

    lft_rgb = os.path.join(nfolder,'left','rgb.png')
    lft_mono = os.path.join(nfolder,'left','mono.png')
    rgt_rgb = os.path.join(nfolder,'right','rgb.png')
    rgt_mono = os.path.join(nfolder,'right','mono.png')
    gt_mask = os.path.join(nfolder,'gt','mask.pfm')
    gt = os.path.join(nfolder,'gt','gt.pfm')


    if platform.system() == 'Windows':
        flags=0x08000000
        script='magick'
    else:
        flags = 0
        script = 'convert'

    call([script,'-define','png:bit-depth=16',os.path.join(basePath,'image_0',filename),'-define','png:format=png48',lft_rgb],creationflags=flags)
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,'image_1',filename),'-define','png:format=png48',rgt_rgb],creationflags=flags)
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,'image_0',filename),lft_mono],creationflags=flags)
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,'image_1',filename),rgt_mono],creationflags=flags)

    data = {'left' : {'mono' : lft_mono, 'rgb': lft_rgb},'right' : {'mono' : rgt_mono, 'rgb': rgt_rgb}}
    with open(os.path.join(basePath,'calib',lcfolder + '.txt'),'r') as myfile:
        calib=myfile.read().split('\n')
    calib = {x[0]:x[1] for x in [x.split(':') for x in calib[:-1]]} # trust leo
    calibP0 = [float(x) for x in calib['P0'][1:].split(' ')]
    calibP1 = [float(x) for x in calib['P1'][1:].split(' ')]

    gtimg = Image.open(os.path.join(basePath,'disp_noc',filename))
    gtarr = np.array(gtimg).astype(np.float32)/256.0
    save_pfm(gt_mask,(gtarr !=0).astype(np.float32))
    save_pfm(gt,gtarr)
    config['names'].append(lcfolder)
    config['data'][lcfolder] = data
    config['maxdisp'][lcfolder] = int(256)
    config['dpx'][lcfolder] = float(calibP0[2])-float(calibP1[2])
    config['baseline'][lcfolder] = -float(calibP1[3])/float(calibP1[0])
    config['fx'][lcfolder] = float(calibP0[0])
    config['fy'][lcfolder] = float(calibP0[5])
    config['px'][lcfolder] = float(calibP0[2])
    config['py'][lcfolder] = float(calibP0[6])
    config['width'][lcfolder] = int(gtarr.shape[1])
    config['height'][lcfolder] = int(gtarr.shape[0])
    config['gt'][lcfolder] = gt
    config['gt_mask'][lcfolder] = gt_mask
    config['minint'][lcfolder] = 0x00FF
    config['maxint'][lcfolder] = 0xFFFF

with open('kitti2012.json','w') as fp:
    json.dump(config,fp, sort_keys=True,indent=4, separators=(',', ': '))
