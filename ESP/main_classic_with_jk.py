import network
import espnow
import sys
import uselect
import time

# 1. Настройка Wi-Fi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=1)

# 2. Настройка ESP-NOW
e = espnow.ESPNow()
e.active(True)
PEER_MAC = b'\x1c\xdb\xd4\xc3\xc9\x30' # Проверь MAC своей C3!
try:
    e.add_peer(PEER_MAC)
except:
    pass

# 3. Настройка поллинга для USB-порта
poll = uselect.poll()
poll.register(sys.stdin, uselect.POLLIN)

HEADER = 0xAA
PACKET_SIZE = 9
buffer = bytearray()

print("USB MODE ACTIVE: Send packets from PC now...")

while True:
    # Проверяем, есть ли данные в USB-порту
    if poll.poll(0): 
        char = sys.stdin.buffer.read(1)
        if char:
            byte = char[0]
            
            if len(buffer) == 0 and byte == HEADER:
                buffer.append(byte)
            elif len(buffer) > 0:
                buffer.append(byte)
                
            if len(buffer) == PACKET_SIZE:
                # Пакет собран — пуляем в ESP-NOW
                try:
                    e.send(PEER_MAC, buffer, False)
                except:
                    pass
                buffer = bytearray() # Очистка буфера

    # Если буфер застрял (мусор), чистим его по таймауту
    if len(buffer) > 0 and time.ticks_diff(time.ticks_ms(), time.ticks_ms()) > 100:
        buffer = bytearray()

    time.sleep_ms(1)