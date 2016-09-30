#!/usr/bin/env python

import json
import os, sys
from PIL import Image
import numpy as np
from subprocess import call
import platform
import os
from util import *

config = get_default_dataset_config()
config['description'] = "a standard collection of quarter sized middlebury images"

basePath = './MiddEval3/trainingQ/'
targetPath = 'middlebury'

check_and_make_dir(targetPath)
for folder in os.listdir(basePath):
    lcfolder = folder.lower()
    nfolder = os.path.join(targetPath,lcfolder)
    check_and_make_dir(nfolder)
    check_and_make_dir(os.path.join(nfolder,'left'))
    check_and_make_dir(os.path.join(nfolder,'right'))
    check_and_make_dir(os.path.join(nfolder,'gt'))

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
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,folder,'im0.png'),lft_rgb],creationflags=flags)
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,folder,'im1.png'),rgt_rgb],creationflags=flags)
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,folder,'im0.png'),'-colorspace','Gray',lft_mono],creationflags=flags)
    call([script,'-define','png:bit-depth=16',os.path.join(basePath,folder,'im1.png'),'-colorspace','Gray',rgt_mono],creationflags=flags)

    data = {'left' : {'mono' : lft_mono, 'rgb': lft_rgb},'right' : {'mono' : rgt_mono, 'rgb': rgt_rgb}}

    with open(os.path.join(basePath,folder,'calib.txt'),'r') as myfile:
        calib=myfile.read().split('\n')
    calib = {x[0]:x[1] for x in [x.split('=') for x in calib[:-1]]} # trust leo
    cam0 = calib['cam0'].strip('[]').replace(';','').replace(' ',',').split(',') #see above

    o_gt = load_pfm(os.path.join(basePath,folder,'disp0GT.pfm'))
    save_pfm(gt_mask,(o_gt[0] != np.inf).astype(np.float32))
    shutil.copy(os.path.join(basePath,folder,'disp0GT.pfm'),gt)
    config['names'].append(lcfolder)
    config['data'][lcfolder] = data
    config['maxdisp'][lcfolder] = int(calib['ndisp'])
    config['dpx'][lcfolder] = float(calib['doffs'])
    config['baseline'][lcfolder] = float(calib['baseline'])/1000.0
    config['fx'][lcfolder] = float(cam0[0])
    config['fy'][lcfolder] = float(cam0[4])
    config['px'][lcfolder] = float(cam0[2])
    config['py'][lcfolder] = float(cam0[5])
    config['width'][lcfolder] = int(calib['width'])
    config['height'][lcfolder] =int(calib['height'])
    config['gt'][lcfolder] =gt
    config['gt_mask'][lcfolder] =gt_mask
    config['minint'][lcfolder] = 0x00FF
    config['maxint'][lcfolder] = 0xFFFF

with open('middlebury.json','w') as fp:
    json.dump(config,fp, sort_keys=True,indent=4, separators=(',', ': '))
