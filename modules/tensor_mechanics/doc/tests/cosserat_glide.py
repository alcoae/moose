#!/usr/bin/env python

import os
import sys
import numpy as np
import matplotlib.pyplot as plt

def expected(y):
    mu = 2.0
    mu_c = 3.0
    be = 0.6
    bbb = 1.0
    we = np.sqrt(2 * mu * mu_c / be / (mu + mu_c))
    phi = bbb * np.sinh(we * y)
    u = 2 * mu_c * bbb * (1 - np.cosh(we * y)) / we / (mu + mu_c)
    m32 = 2 * bbb * be * we * np.cosh(we * y)
    si21 = -4 * mu * mu_c / (mu + mu_c) * bbb * np.sinh(we * y)
    return (phi, u, m32, si21)
    return -bulk * np.log(np.exp(-p0 / bulk) + grav * dens0 * x / bulk)

def equal_to_tol(a, b):
    return (np.abs(a) < 1.0E-10 and np.abs(b) < 1.0E-10) or ((a - b)/(a + b) < 1.0E-5)

def glide():
    f = open("../../tests/static_deformations/gold/cosserat_glide_out_soln_0001.csv")
    data = [map(float, line.strip().split(",")) for line in f.readlines()[1:12]]
    f.close()
    return data



ypoints = np.arange(0, 1.05, 0.01)
moosex = [0.1 * i for i in range(11)]
moose = glide()

plt.figure()
plt.plot(ypoints, expected(ypoints)[0], 'k-', linewidth = 1.0, label = 'expected Cosserat rot')
plt.plot(ypoints, expected(ypoints)[1], 'r-', linewidth = 1.0, label = 'expected displacement')
plt.plot(moosex, [d[4] for d in moose], 'ks', markersize = 10.0, label = 'MOOSE Cosserat rot')
plt.plot(moosex, [d[1] for d in moose], 'r^', markersize = 10.0, label = 'MOOSE displacement')
plt.legend(loc = 'center right')
plt.xlabel("y (m)")
plt.ylabel("displacement (m)")
plt.title("Cosserat glide")
plt.savefig("cosserat_glide_disp.pdf")

plt.figure()
plt.plot(ypoints, expected(ypoints)[2], 'k-', linewidth = 1.0, label = 'expected couple stress')
plt.plot(ypoints, expected(ypoints)[3], 'r-', linewidth = 1.0, label = 'expected shear stress')
plt.plot(moosex, [d[0] for d in moose], 'ks', markersize = 10.0, label = 'MOOSE couple stress')
plt.plot(moosex, [d[3] for d in moose], 'r^', markersize = 10.0, label = 'MOOSE shear stress')
plt.legend(loc = 'center right')
plt.xlabel("y (m)")
plt.ylabel("stress (Pa)")
plt.title("Cosserat glide")
plt.savefig("cosserat_glide_stress.pdf")

sys.exit(0)
