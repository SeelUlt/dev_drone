import network
import espnow
import machine
import time

# --- КОНФИГУРАЦИЯ ---
UART_BAUD = 115200
# Пины UART для C3 Super Mini (Проверьте вашу плату!)
# Вариант А (часто на Super Mini):
UART_RX_PIN = 21  
UART_TX_PIN = 20  
# Вариант Б (если А не работает, попробуйте эти):
# UART_RX_PIN = 20
# UART_TX_PIN = 21

# --- ИНИЦИАЛИЗАЦИЯ ---
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

esp = espnow.ESPNow()
esp.active(True)

# Инициализация UART для связи со STM32
uart = machine.UART(1, baudrate=UART_BAUD, rx=UART_RX_PIN, tx=UART_TX_PIN)

print("ESP32-C3 Bridge (ESP-NOW -> UART) Ready")

while True:
    host, msg = esp.recv()
    if msg:
        # msg - это 5 байт (Pitch, Roll, Yaw, Throttle, Flags)
        # Отправляем их "сырыми" в UART для STM32
        uart.write(msg)
    
    time.sleep_ms(5)