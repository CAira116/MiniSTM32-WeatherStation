"""气象站 TCP 接收端 —— 开 8899 端口,15 分钟窗口,打印收到的报文"""
import socket, time

PORT = 8899
TIMEOUT = 15 * 60

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("0.0.0.0", PORT))
s.listen(1)
s.settimeout(TIMEOUT)
print(f"[server] listening on {PORT}, window {TIMEOUT // 60} min")

try:
    conn, addr = s.accept()
    print(f"[server] connected from {addr[0]}:{addr[1]}")
    while True:
        data = conn.recv(1024)
        if not data:
            break
        print(f"[server] recv: {data.decode('utf-8', errors='replace').strip()}")
except socket.timeout:
    print("[server] timeout, exiting")
except KeyboardInterrupt:
    print("\n[server] stopped")
finally:
    s.close()
