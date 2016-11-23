#!/usr/bin/env python

import numpy as np
import random
import copy
from collections import defaultdict
import matplotlib.pyplot as plt
from scipy.optimize import minimize
from scipy.optimize import minimize_scalar
import os
import sys
from util import load_psm
from scipy.ndimage.filters import convolve
from scipy.ndimage.filters import gaussian_filter
from scipy.ndimage.filters import sobel
from scipy.ndimage.interpolation import rotate
from scipy.ndimage.interpolation import shift
from scipy.special import erf

well_capacity = 1000.0
read_noise = 120.0
sensor_depth = 1024.0
min_vals = 20

edge_pixels = 5

def fit_ge(xspan,span):
    if span[-1] < span[0]:
        span = span[::-1]
    #print xspan,[x for x in span]
    span -= span.min()
    span /= span.max()
    opt= minimize(lambda p: sum([abs(float(yv)-(erf(float(xv+p[1])/p[0])+1.0)/2.0) for xv,yv in zip(xspan,span)]),[10,0],bounds=[[1e-6,None],[-1,1]])
    #print opt
    return opt.x[0]

def plot_span(xspan,span,sigma):
    if span[-1] < span[0]:
        span = span[::-1]
    span = np.array(span)
    span -= span.min()
    span /= span.max()
    plt.plot(xspan,span,c='b')
    plt.plot(xspan,[(erf(float(xv)/sigma)+1.0)/2.0 for x in xspan],c='r')
    plt.show()

def fit_mtf(good_image):
    if len(good_image.shape) == 3:
        good_image = good_image.mean(axis=2)
    blur_est = []
    img = good_image
    ye = sobel(img,axis=0)
    xe = sobel(img,axis=1)
    #e1 = convolve(img,np.array([[0,-1,0],[0,0,0],[0,1,0]]))
    #e2 = convolve(img,np.array([[0,0,0],[-1,0,1],[0,0,0]]))
    gs = np.sqrt(xe**2 + ye**2)
    largest_edges = np.argsort(-gs.ravel())[:1]
    yi,xi = np.unravel_index(largest_edges,gs.shape)
    for y,x in zip(yi,xi):
        m = gs[y,x]
        yx = ye[y,x]
        xx = xe[y,x]
        a = np.arctan2(yx,xx)
        gr = rotate(img,a*180.0/3.14159,mode='nearest')
        yer = sobel(gr,axis=0)
        xer = sobel(gr,axis=1)
        #e1 = convolve(img,np.array([[0,-1,0],[0,0,0],[0,1,0]]))
        #e2 = convolve(img,np.array([[0,0,0],[-1,0,1],[0,0,0]]))
        gsr = np.sqrt(xer**2 + yer**2)
        ler = np.argsort(-gsr.ravel())[:edge_pixels]
        yir,xir = np.unravel_index(ler,gsr.shape)
        for y2,x2 in zip(yir,xir):
            cur = gr[y2,x2]
            xp = 0.0
            xm = 0.0
            for plus in xrange(1,gr.shape[1]-x2):
                diff = cur - gr[y2,x2+plus]
                if abs(diff) > abs(xp):
                    xp = diff
                else:
                    plus -=1
                    break
            for minus in xrange(1,x2):
                diff = cur - gr[y2,x2-minus]
                if abs(diff) > abs(xm):
                    xm = diff
                else:
                    minus -=1
                    break
            xspan = range(-minus,plus+1)
            span = gr[y2,x2-minus:x2+plus+1]

            res = fit_ge(xspan,span)
            blur_est.append(res)
    return blur_est
        #print m,a,y,x,a*180.0/3.14159,xx,yx
def add_noise(x):
    x *= well_capacity
    x = np.random.poisson(x).astype(np.float64)
    x += np.random.standard_normal(x.shape)*read_noise
    #x = x + np.random.poisson(np.ones(x.shape)*read_noise).astype(np.float64)
    x /= well_capacity
    return x


def tr(x,n=0):
    fft_of_signal = np.fft.fft(x)
    if n > 0:
        fft_of_signal[0:n] = 0
    return np.real(np.fft.ifft(fft_of_signal))

if len(sys.argv) == 1:
    num_pixels = 10000
    num_int = 1024
    num_samples = 1024
    a = np.random.rand(num_pixels)
    r = np.empty(shape=(num_samples,num_pixels))
    for i in xrange(num_samples):
        n = add_noise(copy.copy(a))
        r[i,:] = n
        #x,y = cont_to_hist(n,num_int)
    x = r.mean(axis=0)
    y = r.var(axis=0)
    opt= minimize(lambda p: sum([abs(yv-p[0]*(xv + p[1])) for xv,yv in zip(x,y)]), [0,0])
    res = opt.x
    print 1.0/res[0],np.sqrt(res[1]*(1.0/res[0]))
    plt.scatter(x,y)
    plt.show()
else:
    target_dir = sys.argv[1]
    imgs = []
    for f in os.listdir(target_dir):
        fl = os.path.join(target_dir,f) 
        img = load_psm(fl)
        imgs.append(img[0].astype(np.float64))
    imgs = np.array(imgs)

    img = imgs.mean(axis=0)
    img = convolve(img/sensor_depth,np.array([[1,2,1],[2,4,2],[1,2,1]])/16.0)
    sigmas = fit_mtf(img)
    print sigmas
    print 'Gaussian Sigma: {0:.2f}'.format(sigmas[0])

    r = imgs.reshape((imgs.shape[0],-1))/sensor_depth
    xo = r.mean(axis=0)
    for row in r:
        row = tr(row,5)
    yo = r.var(axis=0)
    d = defaultdict(list)
    for x,y in zip(xo,yo):
        d[round(x*sensor_depth)].append(y)
    d2 = [(k,sum(v)/float(len(v))) for k,v in d.iteritems() if len(v) > min_vals]
    x = np.array([t[0]/sensor_depth for t in d2])
    y = np.array([t[1] for t in d2])
    opt= minimize(lambda p: sum([abs(yv-p[0]*(xv + p[1])) for xv,yv in zip(x,y)]), [0,0])
    res = opt.x
    well_cap = 1.0/res[0]
    sn = np.sqrt(max(res[1],0)*well_cap)
    print 'Well Capacity: {0:.0f} \n Shot Noise: {1:.2f}'.format(well_cap,sn)
    plt.scatter(x,y,s=2,lw=0)
    plt.xlim(x.min(),x.max())
    plt.ylim(y.min(),y.max())
    plt.show()
