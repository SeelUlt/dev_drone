#ifndef __ESP_BRIDGE_H
#define __ESP_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usart.h"
#include <stdint.h>

#define ESP_TIMEOUT_MS      2000
#define ESP_START_BYTE      0xAA
#define ESP_PACKET_SIZE     5

typedef struct {
    int8_t pitch;
    int8_t roll;
    int8_t yaw;
    int8_t throttle;
    uint8_t flags;
    uint32_t last_update;
    uint8_t is_valid;
} ESP_Control_t;

// Инициализация (теперь просто сбрасывает состояние)
void ESP_Init(UART_HandleTypeDef *huart);

// Опрос UART (вызывать в цикле while)
void ESP_Poll(UART_HandleTypeDef *huart, ESP_Control_t *ctrl);

// Проверка таймаута
uint8_t ESP_CheckTimeout(ESP_Control_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif
