"""
    expected_frame_time.py

    Calculates the expected frame time given some
    parameters

    python3 expected_frame_time.py
    <n_bits>        1 <= N <= 24
    <time_per_bit>  in us
    <mpt_time>      in us
    <mode>          frame_sync or frame_sync_z mode (0/1)
"""
import numpy as np
from matplotlib import pyplot as plt

def cycle_2(n_bits, tpb, mpt, mode):
    if n_bits <= 0:
        return np.array([])

    time = 0
    next_time = 0
    c = 0
    timestamps = []

    for i in range(n_bits):
        if mode == 1 and i == 0:
            time += tpb
        else:
            if next_time <= time:
                c += 1
                timestamps.append(time)
                next_time = time + mpt
            time += tpb

        if time >= 16666:
            break

    if c < n_bits:
        rest = cycle_2(n_bits - c, tpb, mpt, 0)
        if rest.size > 0:
            timestamps.extend((roundup(time) + rest).tolist())

    return np.array(timestamps)

#
# Assumes the number of bits to capture is
# the same as the number of bits sent
def cycle_1(n_bits, tpb, mpt, mode)->int:
    if n_bits <= 0:
        0
    
    # time for a single cycle
    time = 0
    next_time = 0
    c = 0

    x = []
    for i in range(n_bits):
        if mode == 1 and i == 0:    # skip the first
            time += tpb
        else:
            if next_time <= time:
                c += 1
                next_time = time + mpt

            time += tpb

            if time >= 16666:
                break
        
    if (c < n_bits):
        # ignore for other cycles as we don't consider
        # the frame start
        return 16666 + cycle_1(n_bits - c, tpb, mpt, 0)
    else:
        return time

def roundup(time_us):
    return ((time_us + 15999) // 16666) * 16666

if __name__ == '__main__':
    time_us = cycle_1(24, 693, 1000, 0)
    ft = roundup(time_us)
    print(f'Expected frametime: {ft} us')
    print(f'Expected frame rate: {1/(ft*1e-6):.2f} fps')

    timestamps = cycle_2(24, 693, 1000, 0)
    print(timestamps)
    #plt.figure()
    #plt.vlines(timestamps, ymin=0, ymax=1)
    #plt.title('Time points when c is incremented')
    #plt.xlabel('Time (us)')
    #plt.yticks([])
    #plt.grid(True)
    #plt.show()
