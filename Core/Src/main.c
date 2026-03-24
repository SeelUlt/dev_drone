/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "icm20948.h"
#include "madgwick.h"
#include "motor_control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t imu_flag = 0;
#define RC_PACKET_SIZE 2
volatile int8_t rc_roll = 0;
volatile int8_t rc_pitch = 0;
volatile uint32_t rc_last_update = 0;
volatile uint8_t rc_valid = 0;

static uint8_t uart_rx_byte;
static uint8_t uart_state = 0;
static uint8_t uart_buf[RC_PACKET_SIZE];

#define PI_reverse_180 57.2957795f

MadgwickFilter Filter;
volatile euler_t angles = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void init_fail_detector(init_status status){
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
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("STM32 active\r\n");
  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);      // Высший приоритет
  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);    // Низкий приоритет
  // Запуск приёма UART через прерывание
  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_GPIO_WritePin(spi_cs_port, spi_cs_pin, SET);

  if (who_am_i() == HAL_OK){printf("Im here!\r\n");}
    else {printf("Its so sad\r\n");}
    // Инициализация icm 
    init_status init_st = icm20948_init(_500dps, _4g, 1);
    if (init_st != init_success){
  	  printf("Init error\r\n");
    }
    else{
  	  printf("Init success\r\n");
    }
    init_fail_detector(init_st);

    // Инициализация фильтра 
    madgwick_init(&Filter, 1);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	    static uint32_t last_print = 0;

	    // Печать не чаще 10 раз в секунду (каждые 100 мс)
	    if (HAL_GetTick() - last_print > 500)
	    {
	        last_print = HAL_GetTick();
	        printf("DBG: valid=%d | r=%4d p=%4d\r\n",
	               rc_valid, rc_roll, rc_pitch);

	        printf("Roll: %6d Pitch: %6d Yaw: %6d\r\n",
	        		(int)(angles.roll),
					(int)(angles.pitch),
					(int)(angles.yaw));
	    }
	  /*
	uint8_t packet[2] = {0};
    if (HAL_UART_Receive(&huart2, packet, 2, 5) == HAL_OK){
    int8_t converted[2] = {(int8_t)packet[0], (int8_t)packet[1]};
    printf("first bit: %d, second bit: %d\r\n", converted[0], converted[1]);
    }
    */
  /* USER CODE END 3 */
}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
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

volatile uint32_t uart2_bytes_received = 0;

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
        // === ПРОВЕРКА: Мигаем светодиодом при каждом 250-м вызове (раз в 0.5 сек) ===
        static uint32_t tick_counter = 0;
        tick_counter++;

        if (tick_counter % 250 == 0) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  // Мигание = таймер работает!
        }

        // === Твой существующий код обработки ===
        imu_flag = 1;
        RC_Process_Failure();

        if (rc_valid) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        }

        // Чтение и обработка...
        icm20948_accel_read(&accel);
        icm20948_gyro_read(&gyro);
        icm20948_scale_accel(&accel, &scaled_accel);
        icm20948_scale_gyro(&gyro, &scaled_gyro);
        icm20948_apply_calib(&scaled_accel, &scaled_gyro);

        icm20948_primary_accel_calib(&scaled_accel);
        icm20948_apply_calib(&scaled_accel, &scaled_gyro);


        madgwick_run(&Filter, &scaled_accel, &scaled_gyro);

        // Обновляем глобальные volatile углы
        euler_t temp = madgwick_get_euler(&Filter);
        angles.roll = temp.roll * PI_reverse_180;
        angles.pitch = temp.pitch * PI_reverse_180;
        angles.yaw = temp.yaw * PI_reverse_180;
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
      // Можно моргнуть светодиодом для индикации ошибки
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Если есть светодиод на PC13
      HAL_Delay(200);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
