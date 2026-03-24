/*
 * motor_control.h
 *
 *  Created on: 22 мар. 2026 г.
 *      Author: worki
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#include "main.h"
#include <stdint.h>

/* Каналы моторов */
#define MOTOR_1_CHANNEL TIM_CHANNEL_1
#define MOTOR_2_CHANNEL TIM_CHANNEL_2
#define MOTOR_3_CHANNEL TIM_CHANNEL_3
#define MOTOR_4_CHANNEL TIM_CHANNEL_4

/* Границы OneShot125 */
#define MOTOR_THROTTLE_MIN 0
#define MOTOR_THROTTLE_MAX 1000

#define ONESHOT125_MIN_PULSE 125
#define ONESHOT125_MAX_PULSE 250

/* Внешний таймер из main.c */
extern TIM_HandleTypeDef htim4;

/* Инициализация моторов / запуск PWM / arm ESC */
void Motors_Init(void);

/* Полная остановка всех моторов */
void Motors_StopAll(void);

/* Установка тяги одному мотору */
void Motor_SetThrottle(uint32_t channel, uint16_t throttle);

/* Установка тяги всем моторам сразу */
void Motors_SetAll(uint16_t throttle);

/* Дизарм моторов */
void Motors_Disarm(void);

/* Арм моторов */
void Motors_Arm(void);

/* Проверка, заАрмлены ли моторы */
uint8_t Motors_IsArmed(void);

/* Тестовая последовательность моторов */
void Motors_TestProverka(void);

#endif /* INC_MOTOR_CONTROL_H_ */
