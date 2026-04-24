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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define do_Pin GPIO_PIN_0
#define do_GPIO_Port GPIOA
#define dc_Pin GPIO_PIN_2
#define dc_GPIO_Port GPIOA
#define rst_Pin GPIO_PIN_3
#define rst_GPIO_Port GPIOA
#define cs_Pin GPIO_PIN_4
#define cs_GPIO_Port GPIOA
#define blk_Pin GPIO_PIN_6
#define blk_GPIO_Port GPIOA
#define beep_Pin GPIO_PIN_1
#define beep_GPIO_Port GPIOB
#define stby_Pin GPIO_PIN_15
#define stby_GPIO_Port GPIOB
#define ain2_Pin GPIO_PIN_9
#define ain2_GPIO_Port GPIOA
#define ain1_Pin GPIO_PIN_10
#define ain1_GPIO_Port GPIOA
#define bin1_Pin GPIO_PIN_11
#define bin1_GPIO_Port GPIOA
#define bin2_Pin GPIO_PIN_12
#define bin2_GPIO_Port GPIOA
#define max35105_int_Pin GPIO_PIN_6
#define max35105_int_GPIO_Port GPIOB
#define max35102_int_Pin GPIO_PIN_7
#define max35102_int_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
