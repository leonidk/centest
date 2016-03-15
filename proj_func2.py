from sympy import *
import numpy as np
import matplotlib.pyplot as plt
error_funcs = []
vals = {'B': 0.07, 'f': 1, 'u': 0.01, 'd': 0.01}

xv = np.linspace(0,1,10)
yv = np.zeros(xv.shape)

funcs = []
z, x, f,theta = symbols('z x f theta') # make variables
theta = atan2(x,z) # define theta

rect = f * tan(theta)
#funcs.append((rect,'rectilinear'))
ftheta = f* theta
#funcs.append((ftheta,'ftheta'))
cubed = f* pow(tan(theta),Rational(1,3))
#funcs.append((cubed,'cubed'))
ortho = f* sin(theta)
funcs.append((ortho,'ortho'))
plt.style.use('fivethirtyeight')
for proj, name in funcs:
	z, x, f,theta = symbols('z x f theta') # make variables
	d, B, u = symbols('d B u') # make variables
	theta = atan2(x,z) # define theta

	x = solve(proj - u,x)#,Interval(0,f))# isolate X
	print x
	if len(x) > 1:
		print 'multiple answers!', x
	x = simplify(x[0])
	print 'x', x
	xd = simplify(x.subs(u,u-d)) # shifted by disparity
	print 'disp', xd
	# x(u) = x(u-d) + B
	zf = solveset(xd + B - x,z,Interval(0,f)).condition.lhs # solve for Z
	print 'z',zf
	#if len(zf) > 1:
	#	print 'multiple answers!', zf
	#zf = simplify(zf[0])
	dispf = solveset(zf-z, d,Interval(0,f)).condition.lhs # solve for D
	#dispf = dispf.condition.lhs

	print 'd'
	#if len(dispf) > 1:
	#	print 'multiple answers!', dispf
	#dispf = simplify(dispf[-1])
	print dispf
	zf_diff = diff(zf,d) # dz/d_disparity
	print 'zd'
	zf_z = simplify(zf_diff).subs(d,dispf) # substite d in
	print 'zz'
	print name
	print "\tz(d) :", latex(simplify(zf))
	print "\td(z) :", simplify(dispf)
	print "\tzp(d):", latex(simplify(zf_diff))
	print "\tzp(z)", latex(simplify(zf_z))
	err_func = Lambda((z), zf_z.subs(vals))
	print err_func(0)
	for i, xn in enumerate(xv):
		yv[i] =  err_func(zn)#limit(zf_z.subs(vals),z,xn)
	plt.plot(xv,yv,label=name)

plt.legend()
plt.show()
