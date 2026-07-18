import serial, time

PORT = 'COM5'
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(0.5)
print(f'=== {PORT} {BAUD} baud ===  Ctrl+C 退出 ===')
try:
    while True:
        data = ser.readline()
        if data:
            print(data.decode('utf-8', errors='replace'), end='', flush=True)
except KeyboardInterrupt:
    print('\n退出')
    ser.close()
