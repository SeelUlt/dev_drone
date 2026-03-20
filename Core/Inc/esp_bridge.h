#ifndef __ESP_BRIDGE_H
#define __ESP_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usart.h"
#include <stdint.h>

// Размер пакета: 5 байт данных (без заголовка)
#define ESP_PACKET_SIZE     5
#define ESP_START_BYTE      0xAA
#define ESP_TIMEOUT_MS      500  // Таймаут потери связи

// Флаги управления
#define ESP_FLAG_ARMED      0x01
#define ESP_FLAG_CALIBRATE  0x02
#define ESP_FLAG_MODE_ACRO  0x04
#define ESP_FLAG_MODE_ANGLE 0x08

// Структура полученных данных
typedef struct {
    int8_t pitch;
    int8_t roll;
    int8_t yaw;
    int8_t throttle;
    uint8_t flags;
    uint32_t last_update;  // Время последнего пакета (для таймаута)
    uint8_t is_valid;      // Флаг актуальности данных
} ESP_Control_t;

// Инициализация приемника
void ESP_Init(UART_HandleTypeDef *huart);

// Получить данные (возвращает 1 если данные свежие)
uint8_t ESP_GetControlData(ESP_Control_t *ctrl);

// Обработчик прерывания UART (вызывать из HAL_UART_RxCpltCallback)
void ESP_UART_Callback(UART_HandleTypeDef *huart, UART_HandleTypeDef *expected_huart);

// Проверка таймаута (возвращает 1 если связь потеряна)
uint8_t ESP_CheckTimeout(void);

#ifdef __cplusplus
}
#endif

#endif
