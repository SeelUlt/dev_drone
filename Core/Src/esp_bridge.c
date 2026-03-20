#include "esp_bridge.h"
#include "main.h"
#include <string.h>

static uint8_t rx_buffer[6];
static uint8_t rx_index = 0;
static uint8_t sync_found = 0;

static ESP_Control_t control_data = {0};
static UART_HandleTypeDef *esp_huart = NULL;

void ESP_Init(UART_HandleTypeDef *huart)
{
    esp_huart = huart;
    rx_index = 0;
    sync_found = 0;
    control_data.is_valid = 0;

    // Запуск приема первого байта
    HAL_UART_Receive_IT(esp_huart, &rx_buffer[0], 1);
}

uint8_t ESP_GetControlData(ESP_Control_t *ctrl)
{
    if (control_data.is_valid && !ESP_CheckTimeout()) {
        memcpy(ctrl, &control_data, sizeof(ESP_Control_t));
        return 1;
    }
    return 0;
}

uint8_t ESP_CheckTimeout(void)
{
    if (HAL_GetTick() - control_data.last_update > ESP_TIMEOUT_MS) {
        control_data.is_valid = 0;
        return 1;
    }
    return 0;
}

void ESP_UART_Callback(UART_HandleTypeDef *huart, UART_HandleTypeDef *expected_huart)
{
    if (huart != expected_huart || esp_huart == NULL) {
        return;
    }

    uint8_t byte = rx_buffer[rx_index];

    if (!sync_found) {
        if (byte == ESP_START_BYTE) {
            sync_found = 1;
            rx_index = 1;
        }
        HAL_UART_Receive_IT(esp_huart, &rx_buffer[0], 1);
        return;
    }

    if (rx_index < 6) {
        rx_buffer[rx_index] = byte;
        rx_index++;

        HAL_UART_Receive_IT(esp_huart, &rx_buffer[rx_index], 1);

        if (rx_index == 6) {
            control_data.pitch = (int8_t)rx_buffer[1];
            control_data.roll = (int8_t)rx_buffer[2];
            control_data.yaw = (int8_t)rx_buffer[3];
            control_data.throttle = (int8_t)rx_buffer[4];
            control_data.flags = rx_buffer[5];
            control_data.last_update = HAL_GetTick();
            control_data.is_valid = 1;

            sync_found = 0;
            rx_index = 0;
        }
    } else {
        sync_found = 0;
        rx_index = 0;
        HAL_UART_Receive_IT(esp_huart, &rx_buffer[0], 1);
    }
}
