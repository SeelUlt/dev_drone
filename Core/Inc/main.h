/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define HC_TX_Pin GPIO_PIN_2
#define HC_TX_GPIO_Port GPIOA
#define HC_RX_Pin GPIO_PIN_3
#define HC_RX_GPIO_Port GPIOA
#define ICM_SCK_Pin GPIO_PIN_5
#define ICM_SCK_GPIO_Port GPIOA
#define ICM_MISO_Pin GPIO_PIN_6
#define ICM_MISO_GPIO_Port GPIOA
#define ICM_MOSI_Pin GPIO_PIN_7
#define ICM_MOSI_GPIO_Port GPIOA
#define ICM_CS_Pin GPIO_PIN_0
#define ICM_CS_GPIO_Port GPIOB
#define LORA_CS_Pin GPIO_PIN_2
#define LORA_CS_GPIO_Port GPIOB
#define LORA_SCK_Pin GPIO_PIN_10
#define LORA_SCK_GPIO_Port GPIOB
#define LORA_MISO_Pin GPIO_PIN_14
#define LORA_MISO_GPIO_Port GPIOB
#define LORA_MOSI_Pin GPIO_PIN_15
#define LORA_MOSI_GPIO_Port GPIOB
#define DIO0_Pin GPIO_PIN_8
#define DIO0_GPIO_Port GPIOA
#define DIO0_EXTI_IRQn EXTI9_5_IRQn
#define DEB_TX_Pin GPIO_PIN_9
#define DEB_TX_GPIO_Port GPIOA
#define DEB_RX_Pin GPIO_PIN_10
#define DEB_RX_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
