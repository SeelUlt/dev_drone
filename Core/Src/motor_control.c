/*
 * motor_control.c
 *
 *  Created on: 22 мар. 2026 г.
 *      Author: worki
 */
#include "motor_control.h"
#include <stdio.h>

/* Флаг арма */
static uint8_t motors_armed = 0;

/* Перевод throttle 0..1000 в OneShot125 pulse 125..250 */
static uint16_t Motor_ThrottleToPulse(uint16_t throttle)
{
    if (throttle > MOTOR_THROTTLE_MAX)
    {
        throttle = MOTOR_THROTTLE_MAX;
    }

    return (uint16_t)(ONESHOT125_MIN_PULSE +
                     ((throttle * (ONESHOT125_MAX_PULSE - ONESHOT125_MIN_PULSE)) / MOTOR_THROTTLE_MAX));
}

/* Внутренняя функция прямой установки pulse */
static void Motor_SetPulse(uint32_t channel, uint16_t pulse)
{
    if (pulse < ONESHOT125_MIN_PULSE)
    {
        pulse = ONESHOT125_MIN_PULSE;
    }

    if (pulse > ONESHOT125_MAX_PULSE)
    {
        pulse = ONESHOT125_MAX_PULSE;
    }

    __HAL_TIM_SET_COMPARE(&htim4, channel, pulse);
}

void Motors_Init(void)
{
    /* Запуск PWM на всех 4 каналах */
    HAL_TIM_PWM_Start(&htim4, MOTOR_1_CHANNEL);
    HAL_TIM_PWM_Start(&htim4, MOTOR_2_CHANNEL);
    HAL_TIM_PWM_Start(&htim4, MOTOR_3_CHANNEL);
    HAL_TIM_PWM_Start(&htim4, MOTOR_4_CHANNEL);

    /* Сразу стоп */
    Motors_StopAll();

    /* Инициализация ESC */
    HAL_Delay(3000);

    motors_armed = 1;
}

void Motors_StopAll(void)
{
    Motor_SetPulse(MOTOR_1_CHANNEL, ONESHOT125_MIN_PULSE);
    Motor_SetPulse(MOTOR_2_CHANNEL, ONESHOT125_MIN_PULSE);
    Motor_SetPulse(MOTOR_3_CHANNEL, ONESHOT125_MIN_PULSE);
    Motor_SetPulse(MOTOR_4_CHANNEL, ONESHOT125_MIN_PULSE);
}

void Motor_SetThrottle(uint32_t channel, uint16_t throttle)
{
    uint16_t pulse;

    if (!motors_armed)
    {
        return;
    }

    pulse = Motor_ThrottleToPulse(throttle);
    Motor_SetPulse(channel, pulse);
}

void Motors_SetAll(uint16_t throttle)
{
    if (!motors_armed)
    {
        return;
    }

    Motor_SetThrottle(MOTOR_1_CHANNEL, throttle);
    Motor_SetThrottle(MOTOR_2_CHANNEL, throttle);
    Motor_SetThrottle(MOTOR_3_CHANNEL, throttle);
    Motor_SetThrottle(MOTOR_4_CHANNEL, throttle);
}

void Motors_Disarm(void)
{
    Motors_StopAll();
    motors_armed = 0;
}

void Motors_Arm(void)
{
    Motors_StopAll();
    HAL_Delay(3000);
    motors_armed = 1;
}

uint8_t Motors_IsArmed(void)
{
    return motors_armed;
}

void Motors_TestProverka(void)
{
    printf("=== MOTOR TEST START ===\r\n");

    Motors_Arm();

    for (int i = 1; i <= 4; i++)
    {
        printf("Testing motor %d...\r\n", i);

        Motors_StopAll();
        HAL_Delay(500);

        switch (i)
        {
            case 1:
                Motor_SetThrottle(MOTOR_1_CHANNEL, 200);
                break;

            case 2:
                Motor_SetThrottle(MOTOR_2_CHANNEL, 200);
                break;

            case 3:
                Motor_SetThrottle(MOTOR_3_CHANNEL, 200);
                break;

            case 4:
                Motor_SetThrottle(MOTOR_4_CHANNEL, 200);
                break;
        }

        HAL_Delay(3000);

        Motors_StopAll();
        printf("Motor %d done\r\n", i);

        HAL_Delay(1000);
    }

    printf("=== TEST COMPLETE ===\r\n");

    Motors_StopAll();
}
