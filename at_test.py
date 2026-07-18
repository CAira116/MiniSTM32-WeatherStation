# at_test.py — ESP8266 最简 AT 验证（第 0 步：PC 直连 TTL，不经过 STM32）
# 用法: python at_test.py [指令] [波特率]
#   python at_test.py            -> 发 AT, 115200
#   python at_test.py AT+GMR     -> 查固件版本
#   python at_test.py AT 9600    -> 换 9600 试
import sys
import time
import serial

PORT = "COM5"
cmd = sys.argv[1] if len(sys.argv) > 1 else "AT"
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

ser = serial.Serial(PORT, baud, timeout=0.2)
print(f"[{PORT} @ {baud}] 发送: {cmd!r} + \\r\\n")

ser.reset_input_buffer()
ser.write((cmd + "\r\n").encode())

# 收 2 秒内的所有回复
deadline = time.time() + 2.0
raw = b""
while time.time() < deadline:
    chunk = ser.read(256)
    if chunk:
        raw += chunk

ser.close()

if not raw:
    print("<< 无回复（已读不回）")
    print("排查: CH_PD拉高? TX/RX交叉? 波特率? 试试 python at_test.py AT 9600")
else:
    print(f"<< 收到 {len(raw)} 字节:")
    print(raw.decode("utf-8", errors="replace"))
    if b"OK" in raw:
        print("=== [OK] 模块活着，第 0 步通过 ===")
