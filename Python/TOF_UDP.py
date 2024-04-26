#/usr/bin/env python3

#Test the UDP port 8081 receptions with MCU command "udptest"
#it needs a command on MCU first:
#    udpip <hostip>
#where host IP is the address on ETH or WiFi adapter, it depends how
#the host PC is connected to the network

#in order to kill:
# press CTRL-C but send anything on this UDP port

import socket
import sys
import time
import math
import signal
import struct
import numpy as np
import cv2

#function to normalize an array
def normalized_array(arr):
    # # Check if all elements are the same i.e. beyond 255
    if np.all(arr == arr[0]):
            # Return an array of the same shape with all elements set to 1
            return np.ones_like(arr)
    else:
        # Normalize array between 0 and 1
        return (arr - np.min(arr)) / (np.max(arr) - np.min(arr))

#function to take in depth values and spit out uint (0-255) to be processed as images
def process_depth_data(sensor_array, zones, MIN_DEPTH_CUTOFF = None, MAX_DEPTH_CUTOFF = None):
    """
    process lidar data for depth map:
    -clip it
    -scale it to 0-255 for grayscale image
    -normalize it to 0-1
    """
    sensor_array = np.clip(sensor_array, 0, 200)
    # print(sensor_array_clipped.reshape(8,8))
    sensor_array_normalized = normalized_array(sensor_array)
    sensor_array_normalized *= 255
    image_array=sensor_array_normalized.astype(np.uint8).reshape(zones, zones)

    return image_array

def DecodeUDPPacket(data) :
    global currentUDPseq

    RANGING_SENSOR_MAX_NB_ZONES = 64
    RANGING_SENSOR_NB_ARGET_PER_ZONE = 1

    NumberOfZones = struct.unpack('<I', data[0:4])
    #print("\nZ: %d" % (NumberOfZones))

    if NumberOfZones[0] == 16 :
        zones_per_line = 4
    else :
        zones_per_line = 8

    sensorValues = list()
    for i in range(NumberOfZones[0]) :
        NumberOfTargets, Distance, Status, Ambient, Signal = struct.unpack('<IIIII', data[4 + i * (5*4):4 + (i + 1) * (5*4)])
        #if (i % zones_per_line) == 0 :
        #    print()
        ##print("N: %d D: %d S: %d A: %f S: %f" % (NumberOfTargets, Distance, Status, Ambient, Signal))
        #print(">%4d | %2d <" % (Distance, Status), end = "")

        #####
        sensorValues.append(Distance)

    #print(sensorValues)

    sensorArray=np.array(sensorValues,dtype=float)
    #print(sensorArray)

    imageArray=process_depth_data(sensorArray, zones_per_line) #clip-normalize-scale-uint8-reshape
    print(imageArray)

    #resizing image to make it visible
    imageArray_resized = cv2.resize(imageArray, (400, 400), interpolation=0) # interpolation flags:    INTER_NEAREST=0    CUBIC=2     INTER_LANCZOS4=4  

    # segment image by clipping
    ##ret,imageArray_binary=cv2.threshold(imageArray_resized,50,255,cv2.THRESH_BINARY_INV)
    ##cv2.imshow('test image',imageArray_binary)
    cv2.imshow('test image',imageArray_resized)

    if cv2.waitKey(1) == 27:
        return 1
    else:
        return 0


def Main():
    clatchsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    clatchsock.bind(("0.0.0.0", 8080))

    while True :
        data, addr = clatchsock.recvfrom(8192) # buffer size larger as packet!
        ##print("UDP: Len: %d" % (len(data)))
        ##print(''.join('{:02x} '.format(x) for x in data))
        if DecodeUDPPacket(data) == 1:
            break
        
if __name__ == '__main__':
    Main()

