#!/usr/bin/env python

import numpy as np
import random
import copy
from collections import defaultdict
import matplotlib.pyplot as plt
from scipy.optimize import minimize

well_capacity = 1000.0
read_noise = 120.0

def add_noise(x):
    x *= well_capacity
    x = np.random.poisson(x).astype(np.float64)
    x += np.random.standard_normal(x.shape)*read_noise
    #x = x + np.random.poisson(np.ones(x.shape)*read_noise).astype(np.float64)
    x /= well_capacity
    return x

def cont_to_hist(x,n):
    a = x*n
    d = defaultdict(int)    
    for v in a:
        d[int(round(v))] += 1
    l = [(k,v) for k,v in d.iteritems()]
    return [x[0] for x in l], [x[1] for x in l]

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