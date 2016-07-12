import numpy as np
import tensorflow as tf
from sklearn.metrics import accuracy_score

import sys
import os
import random

class Neural_Network_2D:
    def __init__(self, maxiter=10, eta=1e-3, print_every=1):
        self.maxiter = maxiter
        self.eta = eta
        self.print_every = print_every
#train_X should be ~400x70
    def fit(self, train_X,train_y, test_X,test_y):
        self.session = tf.InteractiveSession()
        self.x = tf.placeholder(tf.float32, [train_X.shape[1],train_X.shape[2]])

# output
        self.y = tf.placeholder(tf.int64, [train_X.shape[1]])

# weights
        conv1_weights = tf.Variable(
          tf.truncated_normal([5, 5, 1, 32],  # 5x5 filter, depth 32.
                      stddev=0.1))
        conv1_biases = tf.Variable(tf.constant(0.1, shape=[32]))
        conv2_weights = tf.Variable(
          tf.truncated_normal([5, 5, 32, 1],
                      stddev=0.1))
        conv2_biases = tf.Variable(tf.constant(0.0, shape=[1]))


# model
        self.img = tf.reshape(self.x,[1,train_X.shape[1],train_X.shape[2],1])

        conv = tf.nn.conv2d(self.img,
                    conv1_weights,
                    strides=[1, 1, 1, 1],
                    padding='SAME')
        relu = tf.nn.relu(tf.nn.bias_add(conv, conv1_biases))
        pool = tf.nn.max_pool(conv1, ksize=[1, 3, 3, 1], strides=[1, 2, 2, 1],
                             padding='SAME', name='pool1')
        conv = tf.nn.conv2d(pool,
                    conv2_weights,
                    strides=[1, 1, 1, 1],
                    padding='SAME')
        relu = tf.nn.relu(tf.nn.bias_add(conv, conv2_biases))
        conv = tf.nn.conv2d(relu,
                    conv3_weights,
                    strides=[1, 1, 1, 1],
                    padding='SAME')
        relu = tf.nn.relu(tf.nn.bias_add(conv, conv3_biases))
        self.pred = tf.nn.bias_add(conv, conv2_biases)
        self.pred = tf.reshape(self.pred,[train_X.shape[1],train_X.shape[2]])

# loss
        cost = tf.nn.sparse_softmax_cross_entropy_with_logits(self.pred,self.y)

# optimizer

        self.optimizer = tf.train.AdamOptimizer(1e-4).minimize(cost)

        correct_prediction = tf.equal(tf.argmax(self.pred,1), self.y)
        accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))

        init = tf.initialize_all_variables()
        loss_value = self.session.run(init)
        test_loss = 0

        #print self.pred.eval(
        #            feed_dict={self.x: train_X[25,:,:], self.y: train_y[25,:]})[0,:]

        for epoch in xrange(self.maxiter):
            train_accuracy = accuracy.eval(
                feed_dict={self.x: train_X[0,:,:], self.y: train_y[0,:]})
                #print("step %d, training accuracy %f, loss %f"%(epoch, train_accuracy,test_loss))
            print epoch, train_accuracy
            #idxs = np.random.permutation(train1.shape[0])
            for i in xrange(0, train_X.shape[0]):
                    _,test_loss = self.session.run([self.optimizer,cost],
                        feed_dict={self.x: train_X[i,:,:], self.y : train_y[i,:]})
        test_accuracy = 0.0
        test_count = 0.0
        for i in xrange(0, test_X.shape[0]):
                test_accuracy += accuracy.eval(
                    feed_dict={self.x: test_X[i,:,:], self.y : test_y[i,:]})
                test_count += 1
        print 'test err', test_accuracy/test_count

if len(sys.argv) > 1:
    mod = Neural_Network_2D(maxiter=int(sys.argv[1]))
else:
    mod = Neural_Network_2D()

if not os.path.isfile('gt.npy'):
    folders = ['moto2d/']
    model_name = 'moto2d_model'
    gt = []
    sgbm = []
    raw = []
    for f in folders:
        gt.append(np.loadtxt(f + 'gt.csv',delimiter=','))
        sgbm.append(np.loadtxt(f + 'sgbm.csv',delimiter=','))
        raw.append(np.loadtxt(f + 'raw.csv',delimiter=','))
    gt = np.hstack(gt).astype(np.int32)
    sgbm = np.vstack(sgbm).astype(np.float32)
    raw = np.vstack(raw).astype(np.float32)
    sgbm = sgbm.reshape(gt.shape[0],gt.shape[1],-1)
    raw = raw.reshape(gt.shape[0],gt.shape[1],-1)
    np.save('gt.npy',gt)
    np.save('raw.npy',raw)
    np.save('sgbm.npy',sgbm)
else:
    gt = np.load('gt.npy').astype(np.float32)
    raw = np.load('raw.npy').astype(np.float32)
    sgbm = np.load('sgbm.npy').astype(np.float32)

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
train_raw, test_raw = split_data(-raw+4000,RAND_FRAC)
train_sgbm, test_sgbm = split_data(-sgbm+7500000,RAND_FRAC)

mod.fit(train_raw,train_gt,test_raw,test_gt)