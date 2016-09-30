import re, shutil, sys, os
import numpy as np
import json
import hashlib
from os.path import normpath, walk, isdir, isfile, dirname, basename, \
    exists as path_exists, join as path_join
from contextlib import contextmanager
import os

@contextmanager
def cd(newdir):
    prevdir = os.getcwd()
    os.chdir(os.path.expanduser(newdir))
    try:
        yield
    finally:
        os.chdir(prevdir)
def path_checksum(paths):
    """
    Recursively calculates a checksum representing the contents of all files
    found with a sequence of file and/or directory paths.

    """
    if not hasattr(paths, '__iter__'):
        raise TypeError('sequence or iterable expected not %r!' % type(paths))

    def _update_checksum(checksum, dirname, filenames):
        for filename in sorted(filenames):
            path = path_join(dirname, filename)
            if isfile(path):
                #print path
                fh = open(path, 'rb')
                while 1:
                    buf = fh.read(4096)
                    if not buf : break
                    checksum.update(buf)
                fh.close()

    chksum = hashlib.sha1()

    for path in sorted([normpath(f) for f in paths]):
        if path_exists(path):
            if isdir(path):
                walk(path, _update_checksum, chksum)
            elif isfile(path):
                _update_checksum(chksum, dirname(path), basename(path))

    return chksum.hexdigest()


def loadFiles(folder,t):
    res = []
    for f in os.listdir(folder):
        jd = os.path.join(folder,f)
        try:
            with open(jd) as fp:
                d = json.load(fp)
                if d['type'] == t:
                    base, ext = os.path.splitext(f)
                    d['name'] = base
                    res.append(d)
        except:
            pass
    return res

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

def check_and_make_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def get_default_dataset_config():
    config = {}

    config['description'] = ''
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
    config['type'] = 'dataset'
    return config