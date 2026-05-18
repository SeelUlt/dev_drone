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

/* Границы обычной тяги */
#define MOTOR_THROTTLE_MIN 0
#define MOTOR_THROTTLE_MAX 1000

/* Границы OneShot125 */
#define ONESHOT125_MIN_PULSE 125
#define ONESHOT125_MAX_PULSE 250

/* Состояние одноразового теста */
typedef enum
{
    MOTOR_TEST_LOCKED = 0,   // моторы запрещены
    MOTOR_TEST_READY,        // тест разрешён, но ещё не запущен
    MOTOR_TEST_RUNNING,      // тест идёт
    MOTOR_TEST_FINISHED      // тест завершён, повтор запрещён
} MotorTestState_t;

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

/* ================= ОДНОРАЗОВЫЙ ТЕСТ ================= */

/*
 * Разрешить один тест.
 * Можно вызвать после нажатия кнопки или после задержки.
 */
void Motors_ArmOnce(void);

/*
 * Запустить одноразовый тест.
 * После этого Motors_OneShotTestIsRunning() начнёт возвращать 1.
 */
void Motors_StartOneShotTest(void);

/*
 * Завершить одноразовый тест.
 * Моторы выключаются, повторный запуск запрещается до reset STM32.
 */
void Motors_FinishOneShotTest(void);

/*
 * Проверка: можно ли сейчас писать значения в моторы.
 */
uint8_t Motors_OneShotTestIsRunning(void);

/*
 * Проверка: завершён ли одноразовый тест.
 */
uint8_t Motors_OneShotTestIsFinished(void);

/*
 * Получить текущее состояние одноразового теста.
 */
MotorTestState_t Motors_GetOneShotTestState(void);

#endif /* INC_MOTOR_CONTROL_H_ */
