import network
import espnow
import time
import random
import struct

PEER_MAC = b'\x1c\xdb\xd4\xc3\xc9\x30'  # MAC C3

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=1)

esp = espnow.ESPNow()
esp.active(True)
esp.add_peer(PEER_MAC)

print("ESP32 Classic generator ready")

BASE = [10, 10, 10, 50, 1]

def make_packet():
    roll = BASE[0] + random.randint(-5, 5)
    pitch = BASE[1] + random.randint(-5, 5)
    yaw = BASE[2] + random.randint(-5, 5)
    throttle = BASE[3] + random.randint(-5, 5)
    flags = BASE[4]

    return struct.pack('bbbbb', roll, pitch, yaw, throttle, flags)

while True:
    if esp.any():
        mac, msg = esp.recv(0)
        if msg:
            pkt = struct.pack(10, 10, 10, 10, 10) #make_packet()
            esp.send(PEER_MAC, pkt)

    time.sleep_ms(1)