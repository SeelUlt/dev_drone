/* Core/Inc/JDY-09.h */
#ifndef JDY_09_H
#define JDY_09_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "main.h"
#include "usart.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Defines */
#define BT_PACKET_SIZE    5
#define BT_SYNC_BYTE      0xAA

/* Types */
typedef struct {
    uint8_t pitch;
    uint8_t roll;
    uint8_t yaw;
    uint8_t throttle;
    uint8_t flags;
    bool is_new;
    uint32_t timestamp;
} BT_Control_t;

/* Prototypes */
void BT_Init(UART_HandleTypeDef *huart);
bool BT_GetControlData(BT_Control_t* ctrl);
void BT_UART_Callback(UART_HandleTypeDef *huart, UART_HandleTypeDef *bt_huart);
HAL_StatusTypeDef BT_SendByte(UART_HandleTypeDef *huart, uint8_t data);
HAL_StatusTypeDef BT_SendPacket(UART_HandleTypeDef *huart, uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* JDY_09_H */
