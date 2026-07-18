import serial
import sys

print("=== STM32 串口监视 ===")
print("COM5, 115200 baud")
print("按 Ctrl+C 退出\n")

ser = serial.Serial('COM5', 115200, timeout=1)

try:
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(f">>> {line}", flush=True)
except KeyboardInterrupt:
    print("\n关闭...")
    ser.close()
