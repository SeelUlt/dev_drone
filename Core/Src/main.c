/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "icm20948.h"
#include "madgwick.h"
#include "motor_control.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */

typedef struct {
    float kp;
    float ki;
    float kd;

    float integral;
    float prev_error;

    float out_min;
    float out_max;
} PID_t;

/* USER CODE END PTD */

/* USER CODE BEGIN PD */

#define RC_PACKET_SIZE 2

#define PI_reverse_180 57.2957795f

/*
 * TIM2 должен быть 500 Гц.
 */
#define PID_DT 0.002f

#define MAX_ANGLE_CMD_DEG 20.0f

/*
 * Базовый газ.
 * Для первого теста с yaw лучше не ставить сразу 300.
 */
//#define BASE_THROTTLE 300.0f
#define THROTTLE_START 150.0f
#define THROTTLE_END   350.0f
#define THROTTLE_RAMP_TIME_MS 7000

#define ROLL_TRIM_PERCENT -0.07f
#define ROLL_TRIM_MAX     30.0f

/*
 * 1 = реально писать в моторы.
 * 0 = только считать и выводить в UART.
 */
#define CONTROL_MOTORS 1

#define PITCH_TRIM_PERCENT 0.15f
#define PITCH_TRIM_MAX     45.0f

/*
 * Одноразовый тест.
 */
#define PID_TEST_DELAY_MS     3000
#define PID_TEST_DURATION_MS  8000

/*
 * YAW_MIX_SIGN:
 *
 * Если yaw-коррекция раскручивает дрон сильнее,
 * поменять 1.0f на -1.0f.
 */
#define YAW_MIX_SIGN 1.0f

/* USER CODE END PD */

/* USER CODE BEGIN PV */
volatile float current_throttle_dbg = 0.0f;

volatile uint8_t imu_flag = 0;

volatile uint8_t pid_enable = 0;
volatile uint8_t motor_update_flag = 0;

volatile uint8_t attitude_ref_captured = 0;
volatile float roll_ref_deg = 0.0f;
volatile float pitch_ref_deg = 0.0f;

volatile int8_t rc_roll = 0;
volatile int8_t rc_pitch = 0;
volatile uint32_t rc_last_update = 0;
volatile uint8_t rc_valid = 0;

static uint8_t uart_rx_byte;
static uint8_t uart_state = 0;
static uint8_t uart_buf[RC_PACKET_SIZE];

volatile uint32_t uart2_bytes_received = 0;

MadgwickFilter Filter;
volatile euler_t angles = {0};

/*
 * Debug accelerometer.
 */
volatile float accel_x_dbg = 0.0f;
volatile float accel_y_dbg = 0.0f;
volatile float accel_z_dbg = 0.0f;

volatile int16_t accel_x_raw_dbg = 0;
volatile int16_t accel_y_raw_dbg = 0;
volatile int16_t accel_z_raw_dbg = 0;

/*
 * Debug gyro.
 */
volatile float gyro_x_dbg = 0.0f;
volatile float gyro_y_dbg = 0.0f;
volatile float gyro_z_dbg = 0.0f;

volatile int16_t gyro_x_raw_dbg = 0;
volatile int16_t gyro_y_raw_dbg = 0;
volatile int16_t gyro_z_raw_dbg = 0;

/*
 * PID test state.
 */
volatile uint8_t pid_test_started = 0;
volatile uint8_t pid_test_finished = 0;
volatile uint32_t pid_test_start_time = 0;

/*
 * Roll PID.
 */
static PID_t pid_roll = {
    .kp = 0.05f,
    .ki = 0.0f,
    .kd = 0.01f,

    .integral = 0.0f,
    .prev_error = 0.0f,

    .out_min = -30.0f,
    .out_max = 30.0f
};

static PID_t pid_pitch = {
    .kp = 0.05f,
    .ki = 0.0f,
    .kd = 0.01f,

    .integral = 0.0f,
    .prev_error = 0.0f,

    .out_min = -30.0f,
    .out_max = 30.0f
};

/*
 * Yaw PID.
 *
 * Пока делаем не удержание angles.yaw,
 * а гашение скорости вращения по gyro Z.
 *
 * target_yaw_rate = 0
 */
static PID_t pid_yaw = {
    .kp = 0.0075f,
    .ki = 0.00125f,
    .kd = 0.0025f,

    .integral = 0.0f,
    .prev_error = 0.0f,

    .out_min = -25.0f,
    .out_max = 25.0f
};

volatile float target_roll_deg = 0.0f;
volatile float target_pitch_deg = 0.0f;
volatile float target_yaw_rate = 0.0f;

volatile float roll_corr_dbg = 0.0f;
volatile float pitch_corr_dbg = 0.0f;
volatile float yaw_corr_dbg = 0.0f;

volatile uint16_t motor1_dbg = 0;
volatile uint16_t motor2_dbg = 0;
volatile uint16_t motor3_dbg = 0;
volatile uint16_t motor4_dbg = 0;

/* USER CODE END PV */

void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

void init_fail_detector(init_status status);

static float PID_Update(PID_t *pid, float target, float current, float dt);
static void PID_Reset(PID_t *pid);

static uint16_t clamp_throttle(float value);
static float rc_to_angle_deg(int8_t rc_value);

void RC_Process_Failure(void);
void PID_Test_Service(void);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

void init_fail_detector(init_status status)
{
    switch(status)
    {
        case init_success:
            printf("Init success\r\n");
            break;

        case who_am_i_fail:
            printf("WHO_AM_I failed\r\n");
            break;

        case reset_fail:
            printf("Reset failed\r\n");
            break;

        case wake_up_fail:
            printf("Wake up failed\r\n");
            break;

        case clock_source_fail:
            printf("Clock source config failed\r\n");
            break;

        case odr_fail:
            printf("ODR align failed\r\n");
            break;

        case spi_slave_enable_fail:
            printf("SPI slave enable failed\r\n");
            break;

        case gyro_srd_fail:
            printf("Gyro sample rate divider failed\r\n");
            break;

        case gyro_fsf_fail:
            printf("Gyro full scale config failed\r\n");
            break;

        case accel_fsf_fail:
            printf("Accel full scale config failed\r\n");
            break;

        default:
            printf("Unknown init error\r\n");
            break;
    }
}

static float PID_Update(PID_t *pid, float target, float current, float dt)
{
    float error = target - current;

    pid->integral += error * dt;

    /*
     * Anti-windup.
     */
    if (pid->integral > 50.0f)
    {
        pid->integral = 50.0f;
    }

    if (pid->integral < -50.0f)
    {
        pid->integral = -50.0f;
    }

    float derivative = (error - pid->prev_error) / dt;

    float output = pid->kp * error
                 + pid->ki * pid->integral
                 + pid->kd * derivative;

    if (output > pid->out_max)
    {
        output = pid->out_max;
    }

    if (output < pid->out_min)
    {
        output = pid->out_min;
    }

    pid->prev_error = error;

    return output;
}

static void PID_Reset(PID_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}

static uint16_t clamp_throttle(float value)
{
    if (value < MOTOR_THROTTLE_MIN)
    {
        return MOTOR_THROTTLE_MIN;
    }

    if (value > MOTOR_THROTTLE_MAX)
    {
        return MOTOR_THROTTLE_MAX;
    }

    return (uint16_t)value;
}

static float rc_to_angle_deg(int8_t rc_value)
{
    return ((float)rc_value / 100.0f) * MAX_ANGLE_CMD_DEG;
}

/*
 * Одноразовый PID motor test.
 */
void PID_Test_Service(void)
{
    if (pid_test_finished)
    {
        pid_enable = 0;
        Motors_StopAll();
        return;
    }

    /*
     * Ждём несколько секунд после старта.
     */
    if (!pid_test_started)
    {
        if (HAL_GetTick() >= PID_TEST_DELAY_MS)
        {
            Motors_ArmOnce();
            Motors_StartOneShotTest();

            pid_test_start_time = HAL_GetTick();
            pid_test_started = 1;

            attitude_ref_captured = 0;

            PID_Reset(&pid_roll);
            PID_Reset(&pid_pitch);
            PID_Reset(&pid_yaw);

            pid_enable = 1;

            printf("PID MOTOR TEST STARTED WITH YAW.\r\n");
        }
        else
        {
            pid_enable = 0;
            Motors_StopAll();
            return;
        }
    }

    /*
     * Автостоп.
     */
    if (pid_test_started && Motors_OneShotTestIsRunning())
    {
        if (HAL_GetTick() - pid_test_start_time >= PID_TEST_DURATION_MS)
        {
            pid_enable = 0;

            PID_Reset(&pid_roll);
            PID_Reset(&pid_pitch);
            PID_Reset(&pid_yaw);

            Motors_FinishOneShotTest();

            pid_test_finished = 1;

            motor1_dbg = 0;
            motor2_dbg = 0;
            motor3_dbg = 0;
            motor4_dbg = 0;

            printf("PID MOTOR TEST FINISHED.\r\n");

            return;
        }
    }

#if CONTROL_MOTORS
    if (motor_update_flag)
    {
        motor_update_flag = 0;

        if (Motors_OneShotTestIsRunning() && Motors_IsArmed())
        {
            Motor_SetThrottle(MOTOR_1_CHANNEL, motor1_dbg);
            Motor_SetThrottle(MOTOR_2_CHANNEL, motor2_dbg);
            Motor_SetThrottle(MOTOR_3_CHANNEL, motor3_dbg);
            Motor_SetThrottle(MOTOR_4_CHANNEL, motor4_dbg);
        }
        else
        {
            Motors_StopAll();
        }
    }
#else
    Motors_StopAll();
#endif
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  printf("STM32 active\r\n");

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);

  /*
   * PWM стартует, но motor_control держит моторы locked,
   * пока не будет Motors_ArmOnce + Motors_StartOneShotTest.
   */
  Motors_Init();
  Motors_StopAll();

  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);

  HAL_GPIO_WritePin(spi_cs_port, spi_cs_pin, SET);

  if (who_am_i() == HAL_OK)
  {
      printf("Im here!\r\n");
  }
  else
  {
      printf("Its so sad\r\n");
  }

  init_status init_st = icm20948_init(_500dps, _4g, 1);

  if (init_st != init_success)
  {
      printf("Init error\r\n");
  }
  else
  {
      printf("Init success\r\n");
  }

  init_fail_detector(init_st);

  madgwick_init(&Filter, 1);

  HAL_TIM_Base_Start_IT(&htim2);

  printf("System ready. PID motor test starts after %lu ms.\r\n",
         (uint32_t)PID_TEST_DELAY_MS);

  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */

    PID_Test_Service();

    static uint32_t last_print = 0;

    if (HAL_GetTick() - last_print > 300)
    {
        last_print = HAL_GetTick();

        printf("\r\n--- PID YAW TEST ---\r\n");

        printf("Angles: R=%6d P=%6d Y=%6d\r\n",
               (int)(angles.roll),
               (int)(angles.pitch),
               (int)(angles.yaw));

        printf("Accel scaled: X=%7d Y=%7d Z=%7d\r\n",
               (int)(accel_x_dbg),
               (int)(accel_y_dbg),
               (int)(accel_z_dbg));

        printf("Gyro scaled:  X=%7d Y=%7d Z=%7d\r\n",
               (int)(gyro_x_dbg),
               (int)(gyro_y_dbg),
               (int)(gyro_z_dbg));

        printf("Target: R=%6d P=%6d Yrate=%6d\r\n",
               (int)(target_roll_deg),
               (int)(target_pitch_deg),
               (int)(target_yaw_rate));

        printf("PID: R=%6d P=%6d Y=%6d\r\n",
               (int)(roll_corr_dbg),
               (int)(pitch_corr_dbg),
               (int)(yaw_corr_dbg));

        printf("Motors: M1=%4u M2=%4u M3=%4u M4=%4u\r\n",
               motor1_dbg,
               motor2_dbg,
               motor3_dbg,
               motor4_dbg);

        printf("M1=rear right | M2=rear left | M3=front left | M4=front right\r\n");

        printf("pid_enable=%d started=%d finished=%d rc_valid=%d state=%d\r\n",
               pid_enable,
               pid_test_started,
               pid_test_finished,
               rc_valid,
               Motors_GetOneShotTestState());
    }

    /* USER CODE END 3 */
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
                              | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1
                              | RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uart2_bytes_received++;

        uart_buf[uart_state] = uart_rx_byte;
        uart_state++;

        if (uart_state >= RC_PACKET_SIZE)
        {
            rc_roll = (int8_t)uart_buf[0];
            rc_pitch = (int8_t)uart_buf[1];

            rc_last_update = HAL_GetTick();
            rc_valid = 1;

            uart_state = 0;
        }

        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    }
}

void RC_Process_Failure(void)
{
    if (rc_valid && (HAL_GetTick() - rc_last_update > 500))
    {
        rc_valid = 0;
        rc_roll = 0;
        rc_pitch = 0;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static axis_raw_t accel;
    static axis_raw_t gyro;
    static axis_scaled_t scaled_accel;
    static axis_scaled_t scaled_gyro;

    if (htim->Instance == TIM2)
    {
        imu_flag = 1;

        RC_Process_Failure();

        if (rc_valid)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        }

        /*
         * IMU read.
         */
        icm20948_accel_read(&accel);
        icm20948_gyro_read(&gyro);

        /*
         * Scale.
         */
        icm20948_scale_accel(&accel, &scaled_accel);
        icm20948_scale_gyro(&gyro, &scaled_gyro);

        /*
         * ВАЖНО:
         * primary_accel_calib не гоняем в цикле.
         */
        // icm20948_primary_accel_calib(&scaled_accel);

        icm20948_apply_calib(&scaled_accel, &scaled_gyro);

        /*
         * Debug accel.
         */
        accel_x_raw_dbg = accel.x;
        accel_y_raw_dbg = accel.y;
        accel_z_raw_dbg = accel.z;

        accel_x_dbg = scaled_accel.x;
        accel_y_dbg = scaled_accel.y;
        accel_z_dbg = scaled_accel.z;

        /*
         * Debug gyro.
         */
        gyro_x_raw_dbg = gyro.x;
        gyro_y_raw_dbg = gyro.y;
        gyro_z_raw_dbg = gyro.z;

        gyro_x_dbg = scaled_gyro.x;
        gyro_y_dbg = scaled_gyro.y;
        gyro_z_dbg = scaled_gyro.z;

        /*
         * Madgwick.
         */
        madgwick_run(&Filter, &scaled_accel, &scaled_gyro);

        euler_t temp = madgwick_get_euler(&Filter);

        float roll_deg  = temp.roll  * PI_reverse_180;
        float pitch_deg = temp.pitch * PI_reverse_180;
        float yaw_deg   = temp.yaw   * PI_reverse_180;

        angles.roll = roll_deg;
        angles.pitch = pitch_deg;
        angles.yaw = yaw_deg;

        /*
         * Если PID выключен — только обновляем IMU debug.
         */
        if (!pid_enable || !Motors_OneShotTestIsRunning())
        {
            roll_corr_dbg = 0.0f;
            pitch_corr_dbg = 0.0f;
            yaw_corr_dbg = 0.0f;

            motor1_dbg = 0;
            motor2_dbg = 0;
            motor3_dbg = 0;
            motor4_dbg = 0;

            motor_update_flag = 1;

            return;
        }

        /*
         * Захват текущего положения как целевого.
         */
        if (!attitude_ref_captured)
        {
            roll_ref_deg = roll_deg;
            pitch_ref_deg = pitch_deg;

            attitude_ref_captured = 1;

            PID_Reset(&pid_roll);
            PID_Reset(&pid_pitch);
            PID_Reset(&pid_yaw);
        }

        /*
         * Roll/Pitch target.
         */
        if (rc_valid)
        {
            target_roll_deg  = roll_ref_deg  + rc_to_angle_deg(rc_roll);
            target_pitch_deg = pitch_ref_deg + rc_to_angle_deg(rc_pitch);
        }
        else
        {
            target_roll_deg  = roll_ref_deg;
            target_pitch_deg = pitch_ref_deg;
        }

        /*
         * Roll / Pitch PID.
         */
        float roll_corr = PID_Update(
            &pid_roll,
            target_roll_deg,
            roll_deg,
            PID_DT
        );

        float pitch_corr = PID_Update(
            &pid_pitch,
            target_pitch_deg,
            pitch_deg,
            PID_DT
        );

        /*
         * Yaw rate PID.
         *
         * Не держим absolute yaw angle.
         * Гасим скорость вращения вокруг Z.
         *
         * target_yaw_rate = 0.
         */
        target_yaw_rate = 0.0f;

        float yaw_corr = PID_Update(
            &pid_yaw,
            target_yaw_rate,
            gyro_z_dbg,
            PID_DT
        );

        yaw_corr *= YAW_MIX_SIGN;

        roll_corr_dbg = roll_corr;
        pitch_corr_dbg = pitch_corr;
        yaw_corr_dbg = yaw_corr;

        /*
         * Реальная карта моторов:
         *
         * M1 = задний правый
         * M2 = задний левый
         * M3 = передний левый
         * M4 = передний правый
         *
         * Roll/Pitch:
         * - нос вниз        -> растут M3/M4
         * - нос вверх       -> растут M1/M2
         * - правый бок вниз -> растут M1/M4
         * - левый бок вниз  -> растут M2/M3
         *
         * Yaw:
         * Предполагаем, что диагонали M1/M3 и M2/M4 крутятся в разные стороны.
         * Если yaw усиливает вращение, поменять YAW_MIX_SIGN.
         */

        float elapsed_ms = (float)(HAL_GetTick() - pid_test_start_time);

        float ramp_k = elapsed_ms / (float)THROTTLE_RAMP_TIME_MS;

        if (ramp_k > 1.0f)
        {
            ramp_k = 1.0f;
        }

        if (ramp_k < 0.0f)
        {
            ramp_k = 0.0f;
        }

        float throttle = THROTTLE_START + ramp_k * (THROTTLE_END - THROTTLE_START);

        current_throttle_dbg = throttle;

        float pitch_trim = throttle * PITCH_TRIM_PERCENT;

        if (pitch_trim > PITCH_TRIM_MAX)
        {
            pitch_trim = PITCH_TRIM_MAX;
        }

        float roll_trim = throttle * ROLL_TRIM_PERCENT;

        if (roll_trim > ROLL_TRIM_MAX)
        {
            roll_trim = ROLL_TRIM_MAX;
        }

        if (roll_trim < -ROLL_TRIM_MAX)
        {
            roll_trim = -ROLL_TRIM_MAX;
        }

        uint16_t m1 = clamp_throttle(throttle + pitch_corr + roll_corr + yaw_corr - pitch_trim + roll_trim); // rear right
        uint16_t m2 = clamp_throttle(throttle + pitch_corr - roll_corr - yaw_corr - pitch_trim - roll_trim); // rear left
        uint16_t m3 = clamp_throttle(throttle - pitch_corr - roll_corr + yaw_corr + pitch_trim - roll_trim); // front left
        uint16_t m4 = clamp_throttle(throttle - pitch_corr + roll_corr - yaw_corr + pitch_trim + roll_trim); // front right

        motor1_dbg = m1;
        motor2_dbg = m2;
        motor3_dbg = m3;
        motor4_dbg = m4;

        motor_update_flag = 1;
    }
}

/* USER CODE END 4 */

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();

  while (1)
  {
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      HAL_Delay(200);
  }

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}

#endif
