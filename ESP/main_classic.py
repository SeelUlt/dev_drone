import network
import espnow
import time

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(channel=3)

mac_address = b'\x1c\xdb\xd4\xc3\xc9\x30'

e = espnow.ESPNow()
e.active(True)

try:
    e.add_peer(mac_address, b'')
    print(f"Peer was added: {mac_address}")
except Exception as err:
    print(f"Error of adding peer {err}")
    
counter = 0

while True:
    text = f"Hello, counter is {counter}"
    msg = text.encode()
    try:
        try:
            e.send(mac_address, msg, False)
        except TypeError:
            e.send(mac_address, msg)
        
        print(f"Sended: {text}")
        
    except OSError as err:
        print(f"Error {err}")
        
    counter += 1
    time.sleep(2)
    

