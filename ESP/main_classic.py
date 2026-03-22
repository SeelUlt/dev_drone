import network
import espnow
import time
from machine import ADC, Pin

DEADZONE = 10

PIN_X = 34
PIN_Y = 35

adc_x = ADC(Pin(PIN_X))
adc_y = ADC(Pin(PIN_Y))

adc_x.atten(ADC.ATTN_11DB)
adc_y.atten(ADC.ATTN_11DB)

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=3)

mac_address = b'\x1c\xdb\xd4\xc3\xc9\x30'

e = espnow.ESPNow()
e.active(True)

def to_bytes(x, y):
    # Преобразуем signed int (-128...127) в unsigned байт (0...255)
    x_byte = x & 0xFF
    y_byte = y & 0xFF
    return bytes([x_byte, y_byte])

def normalize_axis(val):
    val = (val - 2048) // 16
    if abs(val) < DEADZONE:
        return 0
    else: return val

try:
    e.add_peer(mac_address, b'')
    print(f"Peer was added: {mac_address}")
except Exception as err:
    print(f"Error of adding peer {err}")
    
counter = 0

while True:
    text = f"Hello, counter is {counter}"
    x_val = normalize_axis(adc_x.read())
    y_val = normalize_axis(adc_y.read())
    print(f"X: {x_val}, Y: {y_val}")
    msg = to_bytes(x_val, y_val)
    try:
        try:
            e.send(mac_address, msg, False)
        except TypeError:
            e.send(mac_address, msg)
        
    except OSError as err:
        print(f"Error {err}")
        
    counter += 1
    print(f"counter: {counter}")
    time.sleep(0.5)
    

