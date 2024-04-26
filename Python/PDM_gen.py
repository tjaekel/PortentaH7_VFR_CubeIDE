import numpy as np
import matplotlib.pyplot as plt
import random

def pdm(x):
    n = len(x)
    y = np.zeros(n)
    error = np.zeros(n+1)    
    for i in range(n):
        y[i] = 1 if x[i] >= error[i] else 0
        error[i+1] = y[i] - x[i] + error[i]
    return y, error[0:n]

decimation = 64 
periods = 2
amplitude = 0.4                     #0.5 is max. (full amplitude)
dcOffset = 0.5
LE = True                           #bit order for STM32 MCU
DIT = True                          #add dithering or not
n = (periods*48*decimation) + 1     #two periods
fclk = 48e3*decimation              #sample frequency (Hz)
t = np.arange(n) / fclk
f_sin = 1e3                         # sine frequency (Hz)

x = dcOffset + amplitude * np.sin(2*np.pi*f_sin*t)
#dithering
random.seed()

#print(x)
if DIT :
    #do the dithering: round float values to 5 significant numbers,
    #assuming we translate it to a 16bit signed integer (+/-32767)
    for i in range(len(x)) :
        r = random.random() - 0.5       #0.0 <= N < 1.0, both directions
        r = r * 0.00005                #0.000005 or 0.00005
        x[i] = x[i] + r
    #print(x)

y, error = pdm(x)

##print(len(y))
##print(y)
b = bytearray((n-1)//8)
if LE :
    ind  = 0
    j = 0
    for i in range(n-1) :
        v = int(y[i]) << ind
        b[j] = b[j] | ((int(y[i])) << ind)
        ind += 1
        if ind == 8 :
            j += 1
            ind = 0
else :
    ind  = 7
    j = 0
    for i in range(n-1) :
        v = int(y[i]) << ind
        b[j] = b[j] | ((int(y[i])) << ind)
        ind -= 1
        if ind == -1 :
            j += 1
            ind = 7

##print(' '.join(f'0x{A:02x},' for A in b))
print('PDM bytes:')
i = 0
sBytes = 0
for A in b :
    print(f'0x{A:02x},'.format(A), end='')
    print(f'0x{A:02x},'.format(A), end='')
    i += 1
    sBytes += 2
    if i == 16 :
        print()
        i = 0;

print('samples bytes: {:d}'.format(sBytes))

plt.plot(1e3*t, x, label='input signal', linewidth=2.0)
plt.step(1e3*t, y, label='pdm signal',  linewidth=0.1)
#plt.step(1e3*t, error, label='error')
plt.xlabel('Time (ms)')
plt.ylim(-0.05,1.05)
plt.legend()
plt.show()

