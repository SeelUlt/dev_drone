#include <esp_now.h>
#include <WiFi.h>

// Новая сигнатура для ядра ESP32 v3.0+
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  // Нам не важен адрес отправителя (recv_info->src_addr), 
  // просто выкидываем данные в UART
  Serial1.write(incomingData, len);
}

void setup() {
  Serial.begin(115200);

  // Настройка UART для STM32
  // baudrate 460800, пины 20 (RX) и 21 (TX)
  Serial1.begin(460800, SERIAL_8N1, 20, 21);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Теперь типизация совпадет
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("C3 Ready (V3 API). Forwarding...");
}

void loop() {
  // Все так же спим
}