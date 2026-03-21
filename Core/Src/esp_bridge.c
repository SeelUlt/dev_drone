#include "esp_bridge.h"
#include "main.h"
#include <string.h>

static uint8_t rx_buffer[6];
static uint8_t rx_index = 0;
static uint8_t sync_found = 0;
static uint32_t debug_count = 0;

void ESP_Init(UART_HandleTypeDef *huart)
{
    rx_index = 0;
    sync_found = 0;
    debug_count = 0;
}

void ESP_Poll(UART_HandleTypeDef *huart, ESP_Control_t *ctrl)
{
    uint8_t byte;
    HAL_StatusTypeDef status;

    status = HAL_UART_Receive(huart, &byte, 1, 0);

    if (status != HAL_OK) {
        return;
    }

    debug_count++;

    // Отладка: каждые 10 байт печатаем что пришло
    if (debug_count % 10 == 0) {
        // printf("DBG: byte=0x%02X idx=%d sync=%d\r\n", byte, rx_index, sync_found);
    }

    if (!sync_found) {
        if (byte == ESP_START_BYTE) {
            sync_found = 1;
            rx_index = 1;
            // printf("Found START 0xAA\r\n");
        }
        return;
    }

    if (rx_index < 6) {
        rx_buffer[rx_index] = byte;
        rx_index++;

        if (rx_index == 6) {
            ctrl->pitch = (int8_t)rx_buffer[1];
            ctrl->roll = (int8_t)rx_buffer[2];
            ctrl->yaw = (int8_t)rx_buffer[3];
            ctrl->throttle = (int8_t)rx_buffer[4];
            ctrl->flags = rx_buffer[5];
            ctrl->last_update = HAL_GetTick();
            ctrl->is_valid = 1;

            // Отладка: печатаем сырые байты
            printf("RAW: %02X %02X %02X %02X %02X %02X\r\n",
                   rx_buffer[0], rx_buffer[1], rx_buffer[2],
                   rx_buffer[3], rx_buffer[4], rx_buffer[5]);

            sync_found = 0;
            rx_index = 0;
        }
    } else {
        sync_found = 0;
        rx_index = 0;
    }
}

uint8_t ESP_CheckTimeout(ESP_Control_t *ctrl)
{
    if (!ctrl->is_valid) return 1;
    if (HAL_GetTick() - ctrl->last_update > ESP_TIMEOUT_MS) {
        ctrl->is_valid = 0;
        return 1;
    }
    return 0;
}
