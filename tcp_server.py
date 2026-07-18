"""气象站 TCP 接收端 —— 开 8899 端口,断线自动等重连"""
import socket

PORT = 8899
TIMEOUT = 15 * 60

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("0.0.0.0", PORT))
s.listen(1)
print(f"[server] listening on {PORT}, window {TIMEOUT // 60} min")

try:
    while True:
        conn, addr = s.accept()
        conn.settimeout(45)              # 45 秒没数据 = 认为断了
        print(f"[server] connected from {addr[0]}:{addr[1]}")
        while True:
            try:
                data = conn.recv(1024)
                if not data:
                    break
                print(f"[server] recv: {data.decode('utf-8', errors='replace').strip()}")
            except (socket.timeout, ConnectionResetError, ConnectionAbortedError, OSError):
                print("[server] connection lost, waiting for reconnect...")
                break
except KeyboardInterrupt:
    print("\n[server] stopped")
finally:
    s.close()
