import serial
import time
import sys

port = '/dev/ttyACM0'
baudrate = 115200

click_rate = 5 # clicks per second

mode = 'auto'
if len(sys.argv) > 1:
    arg = sys.argv[1]

    if arg == 'auto':
        mode = 'auto'
    elif arg == 'manual':
        mode = 'manual'
    else:
        print("Usage: python clicker.py [auto|manual]")
        sys.exit(1)

ser = serial.Serial(port, baudrate)
while True:
    # Sending Enter triggers a click
    if mode == 'manual':
        input("Press Enter to click...")
        ser.write(b'\n')
    else:  # auto mode
        ser.write(b'\n')
        time.sleep(1 / click_rate)