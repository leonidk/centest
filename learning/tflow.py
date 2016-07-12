import numpy as np
import tensorflow as tf

s = tf.Session()

x = np.zeros((1,2,4)).astype(np.float32)
x[0,:,1] = 1
y = 0*np.ones((2,)).astype(np.int32)
#y[0] = -1
a = tf.constant(x)
aa = tf.squeeze(a)
b = tf.constant(y)
print s.run(tf.nn.sparse_softmax_cross_entropy_with_logits(aa,b))

#a = tf.constant(np.array([[.1, .3, .5, .9]]))
#b = tf.constant(np.array([1.0]))
#d = tf.nn.softmax(a)
#cost = tf.reduce_mean(-tf.reduce_sum(c * tf.log(d), reduction_indices=[1]))
#
#b = tf.constant(np.array([[0.0,-1.0,0.0,0.0]]))
#x = tf.constant(2)
#y= tf.constant(5)
#c2 = tf.select(tf.less(x,y),c,d)
#s.run(d)

