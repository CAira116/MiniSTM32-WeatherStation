import serial, time

# USB-TTL 占的 COM 口，COM5 是 ST-Link，USB-TTL 会多一个
ports = ["COM3","COM4","COM5","COM6","COM7","COM8"]

for port in ports:
    try:
        s = serial.Serial(port, 74880, timeout=0.5)
        time.sleep(0.2)
        data = s.read(200)
        s.close()
        if data:
            print(f"{port} @ 74880: {data[:100]}")
        # try AT
        for baud in [115200, 9600, 57600, 38400, 19200, 230400, 74880]:
            s = serial.Serial(port, baud, timeout=1)
            s.write(b"AT\r\n")
            time.sleep(0.5)
            resp = s.read(100)
            s.close()
            if b'OK' in resp or b'ok' in resp.lower():
                print(f"{port} @ {baud} → OK! {resp}")
                break
            if resp:
                print(f"{port} @ {baud}: {resp[:60]}")
    except Exception as e:
        pass
print("done")
