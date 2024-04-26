#/usr/bin/env python3

#get VBAN audio on UDP port 6980
#strip off the VBAN header (28bytes) and convert into two channel,
#48KHz Fs and 16bit audio sample words,
#forward via sounddevice to default output (speaker)

import socket
import sys
import time
import math
import signal
import struct
import numpy as np
import sounddevice as sd
from matplotlib.animation import FuncAnimation
import matplotlib.pyplot as plt
import queue

q = queue.Queue()  # Queue to store audio data
lines = 0
block_length = int(48*5)
plotdata = np.zeros((block_length, 2))

def DecodeUDPPacket(stream, data) :
    # VBAN has a 28byte header - strip it off
    # we assume fix audio rate: 48KHz, 2 channels, 16bit
    global audiobuffer
    ##print(len(data))
    #### audiobuffer = data[28:]
    #### it must be a two-dimensional numpy array, with 16bit in each channel
    audio = np.frombuffer(data[28:], dtype=np.int16)
    audio.shape = (48*5, 2) #48KHz, 5 frames, two channels
    ## we get 960 bytes, as: 48KHz * 2(byte) *2(channels) *5(frames)
    ## it should be converted into two-dimensional numpy array (for stereo),
    ## with 16bit sample size per channel:
    ## so, 48 * 5 and 2 channels (two-dimensional)
    print(audio)
    print('===================')
    ###### IT FAILS HERE! #######
    #stream.write(audio)

    q.put(audio)
    
    return 0

def update_plot(frame):
    """This function is called by Matplotlib for each plot update.

    Typically, audio callbacks happen more frequently than plot updates,
    therefore the queue tends to contain multiple blocks of audio data.

    Parameters:
    - frame (int): The frame number of the animation (not explicitly used).

    This function updates the plot with the most recent audio data from the queue.
    """
    global plotdata
    global lines
    while True:
        try:
            data = q.get_nowait()
        except queue.Empty:
            break
        shift = len(data)
        plotdata = np.roll(plotdata, -shift, axis=0)
        plotdata[-shift:, :] = data
    for column, line in enumerate(lines):
        line.set_ydata(plotdata[:, column])
    return lines

def Main():
    global lines
    global plotdata

    print(sd.query_devices())

    fig, ax = plt.subplots()
    lines = ax.plot(plotdata)
    ax.axis((0, len(plotdata), -1, 1))
    ax.axis('off')  # Turn off axis ticks and labels
    fig.tight_layout(pad=0)

    clatchsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    clatchsock.bind(("0.0.0.0", 6980))

    sd.default.samplerate = 48000
    sd.default.channels = 2

    stream = sd.OutputStream(device=7, samplerate=48000.0, channels=2, dtype='int16')

    # Animation object for updating the plot
    ani = FuncAnimation(fig, update_plot, interval=30, blit=True, cache_frame_data=False)

    do_just_once = 1

    while True :
        data, addr = clatchsock.recvfrom(8192) # buffer size larger as packet!
        ##print("UDP: Len: %d" % (len(data)))
        ##print(''.join('{:02x} '.format(x) for x in data))
        DecodeUDPPacket(stream, data)

        ##looks wrong and stops - we try to plot two channels!
        if do_just_once == 0 :
            plt.show()
            do_just_once = 1
        
if __name__ == '__main__':
    Main()

