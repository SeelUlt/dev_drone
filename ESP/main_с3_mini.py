import network
import espnow
import machine
import time

PEER_MAC = b'\xaa\xbb\xcc\xdd\xee\xff'  # MAC Classic

UART_BAUD = 115200
UART_RX_PIN = 21
UART_TX_PIN = 20

REQ_PERIOD_MS = 10  # можно увеличить пока

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=1)

esp = espnow.ESPNow()
esp.active(True)
esp.add_peer(PEER_MAC)

uart = machine.UART(1, baudrate=UART_BAUD, rx=UART_RX_PIN, tx=UART_TX_PIN)

last_cmd = 0x01
last_req = time.ticks_ms()

#print("ESP32-C3 bridge ready")

while True:
    # читаем команду от STM32
    if uart.any():
        d = uart.read(1)
        if d:
            last_cmd = d[0]

    # периодический запрос
    now = time.ticks_ms()
    if time.ticks_diff(now, last_req) >= REQ_PERIOD_MS:
        esp.send(PEER_MAC, bytes([last_cmd]))
        last_req = now

    # принимаем ответ
    if esp.any():
        mac, msg = esp.recv(0)
        if msg and len(msg) == 5:
            uart.write(msg)

    time.sleep_ms(1)