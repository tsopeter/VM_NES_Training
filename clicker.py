import serial
import time

port = '/dev/ttyACM0'
baudrate = 115200

click_rate = 5 # clicks per second

ser = serial.Serial(port, baudrate)
while True:
    # Sending Enter triggers a click
    ser.write(b'\n')
    time.sleep(1 / click_rate)