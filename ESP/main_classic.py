import network
import espnow
import sys
import time
import struct

# --- КОНФИГУРАЦИЯ ---
PEER_MAC = b'\x1c\xdb\xd4\xc3\xc9\x30'  # MAC-адрес C3 Mini

# --- ИНИЦИАЛИЗАЦИЯ ---
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

esp = espnow.ESPNow()
esp.active(True)
esp.add_peer(PEER_MAC)

print("ESP32 Classic Bridge (USB -> ESP-NOW) Ready")

buffer = bytearray()
EXPECTED_LEN = 5

while True:
    # Читаем 1 байт из USB (от Raspberry Pi)
    data = sys.stdin.buffer.read(1)
    
    if data:
        byte = data[0]
        buffer.append(byte)
        
        # Если накопили 5 байт — отправляем пакет
        if len(buffer) == EXPECTED_LEN:
            try:
                esp.send(PEER_MAC, bytes(buffer))
                # Раскомментируйте для отладки:
                # print(f"Sent: {list(buffer)}") 
            except Exception as e:
                print(f"ESP-NOW Error: {e}")
            buffer = bytearray()
    
    time.sleep_ms(2)