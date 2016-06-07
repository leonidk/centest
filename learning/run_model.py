import numpy as np

from keras.utils.np_utils import to_categorical
from keras.models import Sequential
from keras.optimizers import SGD,Adam
from keras.regularizers import *
from keras.layers import Dense,Activation,Convolution1D,Flatten,Dropout,AveragePooling1D
from keras.models import model_from_json
import os, sys
from sklearn.metrics import accuracy_score

# load data
f = 'moto/'
model_name = 'moto_piano_model'
if len(sys.argv) > 2:
    f = sys.argv[1]
    model_name = sys.argv[2]
gt = []
sgbm = []
raw = []
gt.append(np.loadtxt(f + 'gt.csv',delimiter=','))
sgbm.append(np.loadtxt(f + 'sgbm.csv',delimiter=','))
raw.append(np.loadtxt(f + 'raw.csv',delimiter=','))
gt = np.hstack(gt)
sgbm = np.vstack(sgbm)
raw = np.vstack(raw)

# data shape
disp_dim = raw.shape[1]
N = gt.shape[0]

model = model_from_json(open(model_name + '.json').read())
model.load_weights(model_name + '.h5')
model.compile(loss='categorical_crossentropy', optimizer='sgd')

naive_raw = np.argmin(raw,1)
naive_sgbm = np.argmin(sgbm,1)

print 'raw accuracy ', accuracy_score(gt,naive_raw)
print 'sgbm accuracy ', accuracy_score(gt,naive_sgbm)

X = -raw + raw.mean()
X = X.reshape((-1,70,1))
pred = model.predict_classes(X,128)
print '2lyer nn accuracy ', accuracy_score(gt,pred)
with open(f + '1dcnn-nin.txt','w') as otf:
    for p in pred:
        otf.write(str(int(p)) + '\n')
