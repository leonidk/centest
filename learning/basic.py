import numpy as np

from sklearn.metrics import accuracy_score

# load data
folders =  ['moto/','piano/','pipes/']
gt = []
sgbm = []
raw = []
for f in folders:
    gt.append(np.loadtxt(f + 'gt.csv',delimiter=','))
    sgbm.append(np.loadtxt(f + 'sgbm.csv',delimiter=','))
    raw.append(np.loadtxt(f + 'raw.csv',delimiter=','))
gt = np.hstack(gt)
sgbm = np.vstack(sgbm)
raw = np.vstack(raw)

# data shape
disp_dim = raw.shape[1]
N = gt.shape[0]

# split things
def split_data(data,pt):
    return data[:pt], data[pt:]
rand_perm = np.random.permutation(N)
RAND_FRAC = int(round(0.8 * N))
gt = gt[rand_perm]
raw_orig = np.copy(raw)
raw = raw[rand_perm,:]
sgbm = sgbm[rand_perm,:]
train_gt, test_gt = split_data(gt,RAND_FRAC)
train_raw, test_raw = split_data(raw,RAND_FRAC)
train_sgbm, test_sgbm = split_data(sgbm,RAND_FRAC)

naive_raw = np.argmin(test_raw,1)
naive_sgbm = np.argmin(test_sgbm,1)

print 'raw accuracy ', accuracy_score(test_gt,naive_raw)
print 'sgbm accuracy ', accuracy_score(test_gt,naive_sgbm)

from sklearn.linear_model import SGDClassifier, LogisticRegression
from sklearn.ensemble import RandomForestClassifier

from keras.utils.np_utils import to_categorical

one_hot_train = to_categorical(train_gt,disp_dim)
one_hot_test = to_categorical(test_gt,disp_dim)

from keras.models import Sequential
from keras.optimizers import SGD,Adam
from keras.regularizers import *
from keras.layers import Dense,Activation,Convolution1D,Flatten,Dropout,AveragePooling1D

model = Sequential()
model.add(Convolution1D(8,9,border_mode='same',input_dim=1,input_length=70))
model.add(Activation('relu'))
model.add(Convolution1D(16,9,border_mode='same',subsample_length=2))
model.add(Activation('relu'))
model.add(Convolution1D(32,9,border_mode='same',subsample_length=5))
model.add(Activation('relu'))
model.add(Convolution1D(64,5,border_mode='same'))
model.add(Activation('relu'))
model.add(AveragePooling1D(7))
model.add(Flatten())
model.add(Dense(disp_dim))
model.add(Activation('softmax'))
model.compile(loss='categorical_crossentropy', optimizer=Adam(lr=0.0001),metrics=['accuracy'])

X = -train_raw + 3000
X = X.reshape((-1,70,1))
model.fit(X,one_hot_train,nb_epoch=10,batch_size=128,verbose=2)
X = -test_raw + 3000
X = X.reshape((-1,70,1))
pred = model.predict_classes(X,128)
print '2lyer nn accuracy ', accuracy_score(test_gt,pred)

json_string = model.to_json()
open('newest_model.json', 'w').write(json_string)
model.save_weights('newest_model.h5')

X = -raw_orig + 3000
X = X.reshape((-1,70,1))
pred = model.predict_classes(X,128)
with open('1dcnn-nin.txt','w') as otf:
    for p in pred:
        otf.write(str(int(p)) + '\n')

