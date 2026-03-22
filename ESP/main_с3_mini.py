import network
import espnow
import machine
import time

# ------------------------------------------------------------
# Укажите реальный MAC-адрес ESP32 Classic
# ------------------------------------------------------------
PEER_MAC = b'\x78\x1c\x3c\x2c\x33\x20'  # MAC Classic

# ------------------------------------------------------------
# UART настройки (соединение со STM32)
# ------------------------------------------------------------
UART_BAUD = 115200
UART_RX_PIN = 21    # RX на C3 (подключается к TX STM32)
UART_TX_PIN = 20    # TX на C3 (подключается к RX STM32)

REQ_PERIOD_MS = 50  # интервал опроса STM32 (50 мс)

# ------------------------------------------------------------
# Инициализация Wi-Fi в режиме STA
# ------------------------------------------------------------
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
time.sleep(0.5)
wlan.disconnect()
wlan.config(channel=1)

# ------------------------------------------------------------
# Инициализация ESP-NOW
# ------------------------------------------------------------
esp = espnow.ESPNow()
esp.active(True)
esp.add_peer(PEER_MAC)

print("ESP32-C3 bridge ready")
print("My MAC:", ':'.join('%02x' % b for b in wlan.config('mac')))   # отладка

# ------------------------------------------------------------
# UART инициализация
# ------------------------------------------------------------
uart = machine.UART(1, baudrate=UART_BAUD, rx=UART_RX_PIN, tx=UART_TX_PIN)

last_cmd = 0x01
last_req = time.ticks_ms()

# ------------------------------------------------------------
# Основной цикл
# ------------------------------------------------------------
while True:
    # 1. Чтение команды от STM32 (если есть)
    if uart.any():
        d = uart.read(1)
        if d:
            last_cmd = d[0]
            print("C3: command from STM32:", last_cmd)

    # 2. Периодическая отправка запроса к Classic
    now = time.ticks_ms()
    if time.ticks_diff(now, last_req) >= REQ_PERIOD_MS:
        try:
            esp.send(PEER_MAC, bytes([last_cmd]))
            print("C3: request sent, cmd =", last_cmd)
        except OSError as e:
            print("C3: send error:", e)
        last_req = now

    # 3. Приём ответа от Classic
    try:
        if esp.any():
            mac, msg = esp.recv(0)
            if msg:
                print("C3: received response:", msg)
                # Передаём весь пакет (6 байт) STM32
                uart.write(msg)
    except OSError as e:
        print("C3: receive error:", e)

    time.sleep_ms(1)