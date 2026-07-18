import serial
ser = serial.Serial('COM5', 115200, timeout=1)
for _ in range(10):
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    if line:
        print(line)
ser.close()
