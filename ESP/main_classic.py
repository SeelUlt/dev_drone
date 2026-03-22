import network
import espnow
import time
import random
import struct

# Укажите реальный MAC-адрес ESP32-C3
PEER_MAC = b'\x1c\xdb\xd4\xc3\xc90'   # замените на реальный

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
time.sleep(0.5)
wlan.disconnect()
wlan.config(channel=1)

esp = espnow.ESPNow()
esp.active(True)
esp.add_peer(PEER_MAC)

print("ESP32 Classic generator ready")
print("My MAC:", ':'.join('%02x' % b for b in wlan.config('mac')))

BASE = [10, 0, 0, 50, 1]

def make_packet():
    roll    = BASE[0] + random.randint(-5, 5)
    pitch   = BASE[1] + random.randint(-5, 5)
    yaw     = BASE[2] + random.randint(-5, 5)
    throttle= BASE[3] + random.randint(-5, 5)
    flags   = BASE[4]
    # 0xAA + 5 байт данных (всего 6 байт)
    return b'\xAA' + struct.pack('bbbbb', roll, pitch, yaw, throttle, flags)

while True:
    try:
        if esp.any():
            mac, msg = esp.recv(0)
            if msg:
                cmd = msg[0] if len(msg) > 0 else 0
                print(f"Classic: received command: {cmd} (0x{cmd:02X})")

                pkt = make_packet()
                roll, pitch, yaw, throttle, flags = struct.unpack('bbbbb', pkt[1:])
                print(f"Classic: sending -> roll={roll}, pitch={pitch}, yaw={yaw}, throttle={throttle}, flags={flags}")

                try:
                    esp.send(PEER_MAC, pkt)
                except OSError as e:
                    print("Classic: send error:", e)
    except OSError as e:
        print("Classic: loop error:", e)

    time.sleep_ms(1)