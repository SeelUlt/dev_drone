import network
import espnow
from machine import UART, Pin
import time

# Настройка Wi-Fi и ESP-NOW
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=1) # Обязательно один канал с приемником

e = espnow.ESPNow()
e.active(True)

# Твой MAC-адрес C3 Super Mini в байтовом виде
PEER_MAC = b'\x1c\xdb\xd4\xc3\xc9\x30' 

try:
    e.add_peer(PEER_MAC)
    print("Peer C3 added!")
except:
    print("Peer already exists or error")

# Настройка UART2 (от ПК)
uart = UART(2, baudrate=115200, rx=16, tx=17, timeout=5)
HEADER = 0xAA
PACKET_SIZE = 9

print("Transmitter is running...")

while True:
    if uart.any() >= PACKET_SIZE:
        # Ищем заголовок, чтобы не слать мусор
        byte = uart.read(1)
        if byte and byte[0] == HEADER:
            payload = uart.read(PACKET_SIZE - 1)
            if len(payload) == PACKET_SIZE - 1:
                # Собираем полный пакет для отправки
                full_packet = byte + payload
                
                # Отправляем по воздуху
                try:
                    e.send(PEER_MAC, full_packet, False) # False - не ждать подтверждения для скорости
                except Exception as err:
                    print("Send error:", err)
    
    time.sleep_ms(1)