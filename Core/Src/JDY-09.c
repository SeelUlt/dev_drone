/* Core/Src/JDY-09.c */
#include "JDY-09.h"

/* Private variables */
static volatile uint8_t rx_buffer[BT_PACKET_SIZE];
static volatile uint8_t rx_index = 0;
static volatile bool packet_ready = false;
static volatile BT_Control_t control_data = {0};
static UART_HandleTypeDef *bt_huart = NULL;

/* Functions */

void BT_Init(UART_HandleTypeDef *huart)
{
    bt_huart = huart;
    rx_index = 0;
    packet_ready = false;

    // Запуск приема первого байта
    HAL_UART_Receive_IT(bt_huart, (uint8_t*)&rx_buffer[0], 1);
}

bool BT_GetControlData(BT_Control_t* ctrl)
{
    if (packet_ready)
    {
        packet_ready = false;
        *ctrl = control_data;
        ctrl->is_new = true;
        ctrl->timestamp = HAL_GetTick();
        return true;
    }
    ctrl->is_new = false;
    return false;
}

HAL_StatusTypeDef BT_SendByte(UART_HandleTypeDef *huart, uint8_t data)
{
    return HAL_UART_Transmit(huart, &data, 1, 100);
}

HAL_StatusTypeDef BT_SendPacket(UART_HandleTypeDef *huart, uint8_t* data, uint8_t len)
{
    return HAL_UART_Transmit(huart, data, len, 100);
}

void BT_UART_Callback(UART_HandleTypeDef *huart, UART_HandleTypeDef *target_huart)
{
    if (huart->Instance == target_huart->Instance)
    {
        // Проверка стартового байта
        if (rx_index == 0 && rx_buffer[0] != BT_SYNC_BYTE)
        {
            HAL_UART_Receive_IT(bt_huart, (uint8_t*)&rx_buffer[0], 1);
            return;
        }

        rx_index++;

        if (rx_index >= BT_PACKET_SIZE)
        {
            // Пакет собран
            control_data.pitch = rx_buffer[0];
            control_data.roll = rx_buffer[1];
            control_data.yaw = rx_buffer[2];
            control_data.throttle = rx_buffer[3];
            control_data.flags = rx_buffer[4];

            packet_ready = true;
            rx_index = 0;

            // Запуск приема следующего пакета
            HAL_UART_Receive_IT(bt_huart, (uint8_t*)&rx_buffer[0], 1);
        }
        else
        {
            // Прием следующего байта
            HAL_UART_Receive_IT(bt_huart, (uint8_t*)&rx_buffer[rx_index], 1);
        }
    }
}
