import network
import espnow
from machine import UART, Pin
import time

# 1. Настройка Wi-Fi и ESP-NOW
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=1)

e = espnow.ESPNow()
e.active(True)

# 2. Настройка UART для связи с STM32
# Используем UART1, пины: TX=21, RX=20 (свободные на C3 Super Mini)
uart_stm = UART(1, baudrate=115200, tx=21, rx=20)

led = Pin(8, Pin.OUT) # Синий светодиод на C3

print("C3 Bridge (ESP-NOW -> STM32) Ready...")

while True:
    host, msg = e.recv()
    if msg:
        # Проверяем структуру (Header 0xAA и длина 9 байт)
        if len(msg) == 9 and msg[0] == 0xAA:
            # Считаем CRC для проверки качества приема по воздуху
            calc_crc = 0
            for b in msg[:-1]:
                calc_crc ^= b
            
            if calc_crc == msg[-1]:
                # Данные валидны! Передаем их в STM32 "как есть" (все 9 байт)
                uart_stm.write(msg)
                
                # Индикация: быстрое мерцание при потоке данных
                led.value(not led.value())
            else:
                # Если пакет битый по воздуху — не шлем его в STM32
                # чтобы дрон не дернулся от мусора
                print("Air CRC Bad")