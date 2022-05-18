"""
file: test.py
author: zoanana990
description:
This file is used to test kecho.ko
"""
import os
import string
import random
import socket
import threading
import argparse
import time
import numpy as np

## Configuration
LENGTH_OF_STRING = 5000

HOST = '127.0.0.1'
PORT = 12345

MAX_THREAD = 1000

LOCK = threading.Lock()
CONDITION = threading.Condition() 

TIMECOST = np.zeros(MAX_THREAD)
RECV_CORR = np.zeros(MAX_THREAD)

READY = False
WAIT = False

SLEEP = 1000

def wait():
    time.sleep(SLEEP)
    
## Socket Connection
def worker(thread_num):

    CORR = False
    ## Connection
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))

    start = time.monotonic_ns()  
    if WAIT:
        wait()
    else:
        outdata = '[' + str(thread_num) + ']' + ''.join(random.choice(string.ascii_letters + string.digits) \
            for _ in range(LENGTH_OF_STRING)) + '[' + str(thread_num) + ']' 
        s.send(outdata.encode())
        indata = s.recv(LENGTH_OF_STRING + 20)
        
        if outdata[:-1] == indata[:-1]:
            CORR = True
            print(outdata)
            print(indata)


    end = time.monotonic_ns()

    ## Get time by lock
    LOCK.acquire()
    TIMECOST[thread_num] = end - start
    RECV_CORR[thread_num] = CORR
    LOCK.release()
    s.close()

if __name__ == "__main__":

    ## get command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', action="store_true", help="waiting for long time")
    args = parser.parse_args()

    if args.w:
        WAIT = True
    
    ## Multi-thread Programming
    print("============================ Test Start ============================")

    # threads = []
    # for i in range(MAX_THREAD):
    #     threads.append(threading.Thread(target = worker, args = (i,)))
    #     threads[i].start()

    worker(0)

    # for i in range(MAX_THREAD):
    #     threads[i].join()
    
    print("============================ Test Finish ============================")
    
    print(TIMECOST)
    print(RECV_CORR)