import numpy as np

from sklearn.metrics import accuracy_score

# load data
gt = np.loadtxt('gt.csv',delimiter=',')
sgbm = np.loadtxt('sgbm.csv',delimiter=',')
raw = np.loadtxt('raw.csv',delimiter=',')

# data shape
disp_dim = raw.shape[1]
N = gt.shape[0]

# split things
def split_data(data,pt):
    return data[:pt], data[pt:]
rand_perm = np.random.permutation(N)
RAND_FRAC = int(round(0.66 * N))
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

#clf = SGDClassifier(n_jobs=-1)
#clf.fit(train_raw,train_gt)
#pred = clf.predict(test_raw)
#print 'linearsvm accuracy ', accuracy_score(test_gt,pred)

#clf = LogisticRegression(n_jobs=-1)
#clf.fit(train_raw,train_gt)
#pred = clf.predict(test_raw)
#print 'logistic accuracy ', accuracy_score(test_gt,pred)

#clf = RandomForestClassifier(min_samples_leaf=20,n_jobs=-1)
#clf.fit(train_raw,train_gt)
#pred = clf.predict(test_raw)
#print 'rfc accuracy ', accuracy_score(test_gt,pred)
#pred = clf.predict(raw_orig)
#with open('rfc.txt','w') as otf:
#    for p in pred:
#        otf.write(str(int(p)) + '\n')
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
X = -train_raw + train_raw.mean()
X = X.reshape((-1,70,1))
model.fit(X,one_hot_train,nb_epoch=24,batch_size=128,verbose=2)
X = -test_raw + test_raw.mean()
X = X.reshape((-1,70,1))
pred = model.predict_classes(X)
print '2lyer nn accuracy ', accuracy_score(test_gt,pred)
X = -raw_orig + train_raw.mean()
X = X.reshape((-1,70,1))
pred = model.predict_classes(X)
with open('1dcnn-nin.txt','w') as otf:
    for p in pred:
        otf.write(str(int(p)) + '\n')

