import network
import espnow
import time
from machine import ADC, Pin

print("🚀 Start...")

try:
    # ================= НАСТРОЙКИ =================
    WIFI_CHANNEL = 3
    PEER_MAC = b'\x1c\xdb\xd4\xc3\xc9\x30'
    # b'\x1c\xdb\xd4\xc3\xc90'
    # 1c:db:d4:c3:c9:30
    # b'\x1c\xdb\xd4\xc3\xc9\x30'
    PIN_X = 34
    PIN_Y = 35
    ADC_CENTER_X = 2546
    ADC_CENTER_Y = 1189
    DEADZONE = 15
    DEBUG = True  # ← Включи отладку, чтобы видеть ошибки
    # =============================================

    # 1. ADC
    print("1. Init ADC...")
    adc_x = ADC(Pin(PIN_X))
    adc_y = ADC(Pin(PIN_Y))
    adc_x.atten(ADC.ATTN_11DB)
    adc_y.atten(ADC.ATTN_11DB)

    # 2. WiFi (режим STA без подключения)
    print("2. Init WiFi...")
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.config(channel=WIFI_CHANNEL)

    # 3. ESP-NOW
    print("3. Init ESP-NOW...")
    e = espnow.ESPNow()
    e.active(True)
    e.add_peer(PEER_MAC, b'')
    
    print("✅ Ready!")

    # 4. Главный цикл
    while True:
        raw_x = adc_x.read()
        raw_y = adc_y.read()
        
        x_val = (raw_x - ADC_CENTER_X) // 16
        y_val = (raw_y - ADC_CENTER_Y) // 16
        
        if abs(x_val) < DEADZONE: x_val = 0
        if abs(y_val) < DEADZONE: y_val = 0
        
        x_val = max(-128, min(127, x_val))
        y_val = max(-128, min(127, y_val))
        
        msg = bytes([x_val & 0xFF, y_val & 0xFF])
        
        try:
            e.send(PEER_MAC, msg)
            if DEBUG:
                print(f"X:{x_val:4d} Y:{y_val:4d}")
        except Exception as err:
            if DEBUG:
                print(f"Send err: {err}")
        
        time.sleep(0.01)

except Exception as e:
    # ← Если код упадёт здесь, ты увидишь причину!
    print(f"❌ CRITICAL ERROR: {e}")
    time.sleep(2)