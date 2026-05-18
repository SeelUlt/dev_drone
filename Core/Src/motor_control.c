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

/* Состояние одноразового теста */
static volatile MotorTestState_t one_shot_test_state = MOTOR_TEST_LOCKED;

/* Перевод throttle 0..1000 в OneShot125 pulse 125..250 */
static uint16_t Motor_ThrottleToPulse(uint16_t throttle)
{
    if (throttle > MOTOR_THROTTLE_MAX)
    {
        throttle = MOTOR_THROTTLE_MAX;
    }

    return (uint16_t)(ONESHOT125_MIN_PULSE +
                     (((uint32_t)throttle *
                     (ONESHOT125_MAX_PULSE - ONESHOT125_MIN_PULSE))
                     / MOTOR_THROTTLE_MAX));
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

/* Внутренняя функция полного нуля PWM */
static void Motor_SetZeroPulse(uint32_t channel)
{
    __HAL_TIM_SET_COMPARE(&htim4, channel, 0);
}

void Motors_Init(void)
{
    /* Запуск PWM на всех 4 каналах */
    HAL_TIM_PWM_Start(&htim4, MOTOR_1_CHANNEL);
    HAL_TIM_PWM_Start(&htim4, MOTOR_2_CHANNEL);
    HAL_TIM_PWM_Start(&htim4, MOTOR_3_CHANNEL);
    HAL_TIM_PWM_Start(&htim4, MOTOR_4_CHANNEL);

    /*
     * Сразу подаём минимальный OneShot125.
     * Это нужно, чтобы ESC увидели "минимальный газ" при старте.
     */
    Motors_StopAll();

    /* Инициализация ESC */
    HAL_Delay(3000);

    /*
     * ВАЖНО:
     * Раньше тут было motors_armed = 1.
     * Теперь НЕ армим автоматически.
     * После подачи питания дрон должен молчать.
     */
    motors_armed = 0;
    one_shot_test_state = MOTOR_TEST_LOCKED;

    Motors_StopAll();

    printf("Motors init complete. Motors locked.\r\n");
}

void Motors_StopAll(void)
{
    /*
     * Для ESC лучше слать минимальный валидный импульс,
     * а не 0, чтобы они понимали состояние "минимальный газ".
     */
    Motor_SetPulse(MOTOR_1_CHANNEL, ONESHOT125_MIN_PULSE);
    Motor_SetPulse(MOTOR_2_CHANNEL, ONESHOT125_MIN_PULSE);
    Motor_SetPulse(MOTOR_3_CHANNEL, ONESHOT125_MIN_PULSE);
    Motor_SetPulse(MOTOR_4_CHANNEL, ONESHOT125_MIN_PULSE);
}

void Motor_SetThrottle(uint32_t channel, uint16_t throttle)
{
    uint16_t pulse;

    /*
     * Главная защита:
     * моторы можно крутить только если:
     * 1. они заармлены;
     * 2. одноразовый тест находится в состоянии RUNNING.
     */
    if (!motors_armed || !Motors_OneShotTestIsRunning())
    {
        Motor_SetPulse(channel, ONESHOT125_MIN_PULSE);
        return;
    }

    pulse = Motor_ThrottleToPulse(throttle);
    Motor_SetPulse(channel, pulse);
}

void Motors_SetAll(uint16_t throttle)
{
    if (!motors_armed || !Motors_OneShotTestIsRunning())
    {
        Motors_StopAll();
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
    /*
     * Обычный Arm теперь не должен обходить защиту одноразового теста.
     * Поэтому армим только если тест подготовлен к запуску.
     */
    if (one_shot_test_state != MOTOR_TEST_READY &&
        one_shot_test_state != MOTOR_TEST_RUNNING)
    {
        Motors_StopAll();
        motors_armed = 0;
        return;
    }

    Motors_StopAll();
    HAL_Delay(500);

    motors_armed = 1;
}

uint8_t Motors_IsArmed(void)
{
    return motors_armed;
}

/* ================= ОДНОРАЗОВЫЙ ТЕСТ ================= */

void Motors_ArmOnce(void)
{
    /*
     * Разрешить один тест можно только из LOCKED.
     * После FINISHED уже нельзя.
     */
    if (one_shot_test_state == MOTOR_TEST_LOCKED)
    {
        one_shot_test_state = MOTOR_TEST_READY;
        printf("One-shot motor test ready.\r\n");
    }
}

void Motors_StartOneShotTest(void)
{
    /*
     * Запуск возможен только один раз.
     */
    if (one_shot_test_state == MOTOR_TEST_READY)
    {
        one_shot_test_state = MOTOR_TEST_RUNNING;

        Motors_Arm();

        printf("One-shot motor test started.\r\n");
    }
}

void Motors_FinishOneShotTest(void)
{
    Motors_StopAll();
    Motors_Disarm();

    /*
     * После FINISHED повторный запуск запрещён до reset STM32.
     */
    one_shot_test_state = MOTOR_TEST_FINISHED;

    printf("One-shot motor test finished. Motors locked until reset.\r\n");
}

uint8_t Motors_OneShotTestIsRunning(void)
{
    return one_shot_test_state == MOTOR_TEST_RUNNING;
}

uint8_t Motors_OneShotTestIsFinished(void)
{
    return one_shot_test_state == MOTOR_TEST_FINISHED;
}

MotorTestState_t Motors_GetOneShotTestState(void)
{
    return one_shot_test_state;
}

/* ================= СТАРАЯ ПРОВЕРКА МОТОРОВ, НО ТЕПЕРЬ ОДНОРАЗОВАЯ ================= */

void Motors_TestProverka(void)
{
    /*
     * Если тест уже был завершён — ничего не делаем.
     */
    if (Motors_OneShotTestIsFinished())
    {
        printf("Motor test already finished. Reset STM32 to run again.\r\n");
        Motors_StopAll();
        return;
    }

    printf("=== MOTOR TEST START ===\r\n");

    Motors_ArmOnce();
    Motors_StartOneShotTest();

    if (!Motors_OneShotTestIsRunning())
    {
        printf("Motor test cannot start.\r\n");
        Motors_StopAll();
        return;
    }

    for (int i = 1; i <= 4; i++)
    {
        printf("Testing motor %d...\r\n", i);

        Motors_StopAll();
        HAL_Delay(500);

        switch (i)
        {
            case 1:
                Motor_SetThrottle(MOTOR_1_CHANNEL, 300);
                break;

            case 2:
                Motor_SetThrottle(MOTOR_2_CHANNEL, 300);
                break;

            case 3:
                Motor_SetThrottle(MOTOR_3_CHANNEL, 300);
                break;

            case 4:
                Motor_SetThrottle(MOTOR_4_CHANNEL, 300);
                break;

            default:
                break;
        }

        HAL_Delay(3000);

        Motors_StopAll();

        printf("Motor %d done\r\n", i);

        HAL_Delay(1000);
    }

    printf("=== TEST COMPLETE ===\r\n");

    Motors_FinishOneShotTest();
}
