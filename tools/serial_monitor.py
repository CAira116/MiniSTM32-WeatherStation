"""
Mini Serial Monitor - 自动检测 COM 口
默认波特率 115200
按 Ctrl+C 退出
"""
import serial
import serial.tools.list_ports
import sys

BAUD = 115200

# 自动查找 CH340 或 STM32 串口
ports = list(serial.tools.list_ports.comports())
target = None

for p in ports:
    desc = p.description.lower()
    if 'ch340' in desc or 'stm' in desc or 'usb-serial' in desc:
        target = p.device
        break

# 如果没找到，尝试常用 COM 口
if not target:
    for p in ports:
        if p.device in ['COM3', 'COM4', 'COM5', 'COM6', 'COM7']:
            target = p.device
            break

if not target and ports:
    target = ports[0].device  # 最后一个选择

if not target:
    print("No COM port found! Is the board connected?")
    sys.exit(1)

print(f"Serial Monitor - {target} {BAUD}bps")
print("Waiting for data... Press Ctrl+C to exit")
print("=" * 50)

try:
    ser = serial.Serial(target, BAUD, timeout=0.5)
    while True:
        data = ser.readline()
        if data:
            sys.stdout.write(data.decode('utf-8', errors='replace'))
            sys.stdout.flush()
except KeyboardInterrupt:
    print("\n\nClosed.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in dir() and ser.is_open:
        ser.close()
