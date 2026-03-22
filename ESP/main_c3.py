from machine import Pin
import time
import network
import espnow

#mac of esp classic 78:1c:3c:2c:33:20
#mac of esp c3 1c:db:d4:c3:c9:30

joy_x = 0
joy_x = 0

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=3)

e = espnow.ESPNow()
e.active(True)

def on_recv(status):
    if status is not None:
        try:
            mac, msg = e.recv()
            joy_x = msg[0] if msg[0] < 128 else msg[0] - 256
            joy_y = msg[1] if msg[1] < 128 else msg[1] - 256
            print(f"X:{joy_x:4d}  Y:{joy_y:4d}")
            time.sleep(0.2)
        except:
            pass
    
e.irq(on_recv)

print("Recieve is start")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    e.active(False)
    wlan.active(False)
    print("Stop")