#!/usr/bin/env python
import json
import os, sys
from PIL import Image
import numpy as np
from subprocess import call
import re, shutil
import platform
def load_pfm(fname):
    color = None
    width = None
    height = None
    scale = None
    endian = None

    file = open(fname,'rU')
    header = file.readline().rstrip()
    if header == 'PF':
        color = True
    elif header == 'Pf':
        color = False
    else:
        raise Exception('Not a PFM file.')

    dim_match = re.match(r'^(\d+)\s(\d+)\s$', file.readline())
    if dim_match:
        width, height = map(int, dim_match.groups())
    else:
        raise Exception('Malformed PFM header.')

    scale = float(file.readline().rstrip())
    if scale < 0: # little-endian
        endian = '<'
        scale = -scale
    else:
        endian = '>' # big-endian

    data = np.fromfile(file, endian + 'f')
    shape = (height, width, 3) if color else (height, width)
    return np.flipud(np.reshape(data, shape)), scale

def save_pfm(fname, image, scale=1):
    file = open(fname, 'wb')
    color = None

    if image.dtype.name != 'float32':
        raise Exception('Image dtype must be float32.')

    if len(image.shape) == 3 and image.shape[2] == 3: # color image
        color = True
    elif len(image.shape) == 2 or len(image.shape) == 3 and image.shape[2] == 1: # greyscale
        color = False
    else:
        raise Exception('Image must have H x W x 3, H x W x 1 or H x W dimensions.')

    file.write('PF\n' if color else 'Pf\n')
    file.write('%d %d\n' % (image.shape[1], image.shape[0]))

    endian = image.dtype.byteorder

    if endian == '<' or endian == '=' and sys.byteorder == 'little':
        scale = -scale

    file.write('%f\n' % scale)

    np.flipud(image).tofile(file)
config = {}

config['description'] = "a standard collection of quarter sized middlebury images"
config['names'] = []
config['data'] = {}
config['maxdisp'] = {}
config['fx'] = {}
config['fy'] = {}
config['px'] = {}
config['py'] = {}
config['dpx'] = {}
config['gt'] = {}
config['gt_mask'] = {}
config['baseline'] = {}
config['width'] = {}
config['height'] = {}
config['maxint'] = {}
config['minint'] = {}

basePath = './MiddEval3/trainingQ/'
def check_and_make_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)
check_and_make_dir('./middlebury')
for folder in os.listdir(basePath):
    lcfolder = folder.lower()
    nfolder = './middlebury/' + lcfolder
    check_and_make_dir(nfolder)
    check_and_make_dir(nfolder +'/left')
    check_and_make_dir(nfolder + '/right')
    check_and_make_dir(nfolder + '/gt')

    lft_rgb = nfolder + '/left/rgb.png'
    lft_mono = nfolder + '/left/mono.png'
    rgt_rgb = nfolder + '/right/rgb.png'
    rgt_mono = nfolder + '/right/mono.png'
    gt_mask = nfolder + '/gt/mask.pfm'
    gt = nfolder + '/gt/gt.pfm'

    if platform.system() == 'Windows':
        flags=0x08000000
        script='magick'
    else:
        flags = 0
        script = 'convert'
    call([script,'-define','png:bit-depth=16',basePath + folder + '/im0.png',lft_rgb],creationflags=flags)
    call([script,'-define','png:bit-depth=16',basePath + folder + '/im1.png',rgt_rgb],creationflags=flags)
    call([script,'-define','png:bit-depth=16',basePath + folder + '/im0.png','-colorspace','Gray',lft_mono],creationflags=flags)
    call([script,'-define','png:bit-depth=16',basePath + folder + '/im1.png','-colorspace','Gray',rgt_mono],creationflags=flags)

    data = {'left' : {'mono' : lft_mono, 'rgb': lft_rgb},'right' : {'mono' : rgt_mono, 'rgb': rgt_rgb}}

    with open(basePath + folder + '/calib.txt','r') as myfile:
        calib=myfile.read().split('\n')
    calib = {x[0]:x[1] for x in [x.split('=') for x in calib[:-1]]} # trust leo
    cam0 = calib['cam0'].strip('[]').replace(';','').replace(' ',',').split(',') #see above

    o_gt = load_pfm(basePath + folder + '/disp0GT.pfm')
    save_pfm(gt_mask,(o_gt[0] != np.inf).astype(np.float32))
    shutil.copy(basePath + folder + '/disp0GT.pfm',gt)
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
