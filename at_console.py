# at_console.py — 8266 AT 指令交互控制台（走 STM32 透传链路）
# 打字回车即发送（自动补 \r\n），8266 的回复实时打印
# 输入 exit 退出
import threading
import serial

PORT = "COM5"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=0.2)
print(f"=== AT 控制台 [{PORT} @ {BAUD}] ===")
print("打字回车发送（自动加\\r\\n），输入 exit 退出\n")

alive = True

def reader():
    while alive:
        chunk = ser.read(256)
        if chunk:
            print(chunk.decode("utf-8", errors="replace"), end="", flush=True)

t = threading.Thread(target=reader, daemon=True)
t.start()

try:
    while True:
        line = input()
        if line.strip().lower() == "exit":
            break
        ser.write((line + "\r\n").encode())
except (KeyboardInterrupt, EOFError):
    pass

alive = False
ser.close()
print("\n=== 已退出 ===")
