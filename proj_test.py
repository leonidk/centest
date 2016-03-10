import numpy as np

xv = xrange(10)
shiftv = xrange(5)
halfx = (len(xv)-1)/2.0

for x in xv:
	xf = (x-halfx)/halfx
	for shift in shiftv:
		if x-shift < 0:
			continue
		df = (x-shift-halfx)/halfx
		if abs(df) > abs(xf):
			#df is further off axis, samples are smaller
			ds = 1
			xs = df**2/xf**2
		else:
			#df is smaller
			xs = 1
			ds = xf**2/df**2
		print x,shift, xf,df, xs,ds