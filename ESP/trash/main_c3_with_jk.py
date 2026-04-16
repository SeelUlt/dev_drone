import network
import espnow
import struct
from machine import Pin

# Настройка Wi-Fi в режиме Station
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=1)

e = espnow.ESPNow()
e.active(True)

# Светодиод на C3 Super Mini обычно на GPIO 8 (синий)
led = Pin(8, Pin.OUT) 

print("C3 Receiver is waiting for data...")

while True:
    host, msg = e.recv()
    if msg:
        # Проверка структуры пакета
        if len(msg) == 9 and msg[0] == 0xAA:
            # Проверка CRC (XOR всех байт кроме последнего)
            calc_crc = 0
            for b in msg[:-1]:
                calc_crc ^= b
            
            if calc_crc == msg[-1]:
                # Распаковываем (ID, 5 осей, кнопки)
                data = struct.unpack("BbbbbbB", msg[1:-1])
                
                # Мигаем светодиодом для индикации связи
                led.value(not led.value())
                
                # Вывод в терминал
                print("ID:{:3} | Axes:{} | BTN:{:08b}".format(data[0], data[1:6], data[6]))
            else:
                print("Bad CRC over air")