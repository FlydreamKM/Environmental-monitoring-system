/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.h
  * @brief   main.c 模块的头文件。
  *          包含健康监测项目的通用应用定义、外设句柄外部声明以及 GPIO 引脚映射。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
  *            This software is licensed under terms in the LICENSE file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7735.h"
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

/**
  * @brief  HAL 回调函数，在初始化完成后完成 TIM 外设引脚重映射。
  * @param  htim：指向 TIM 句柄的指针
  */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  错误处理函数：禁止中断并无限循环。
  */
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* ---------- Digital I/O & Interrupt pins ---------- */
#define do_Pin              GPIO_PIN_0   /*!< MQ-x 传感器数字输出 (PA0) */
#define do_GPIO_Port        GPIOA

#define max35105_int_Pin    GPIO_PIN_6   /*!< MAX30105 中断输入 (PB6) */
#define max35105_int_GPIO_Port GPIOB

#define max35102_int_Pin    GPIO_PIN_7   /*!< MAX30102 中断输入 (PB7) */
#define max35102_int_GPIO_Port GPIOB

/* ---------- ST7735 TFT LCD control pins (SPI1) ---------- */
#define dc_Pin              GPIO_PIN_2   /*!< ST7735 数据/命令选择 (PA2) */
#define dc_GPIO_Port        GPIOA

#define rst_Pin             GPIO_PIN_3   /*!< ST7735 复位 (PA3) */
#define rst_GPIO_Port       GPIOA

#define cs_Pin              GPIO_PIN_4   /*!< ST7735 片选 (PA4) */
#define cs_GPIO_Port        GPIOA

#define blk_Pin             GPIO_PIN_6   /*!< ST7735 背光控制 (PA6) */
#define blk_GPIO_Port       GPIOA

/* ---------- TB6612 Motor Driver control pins ---------- */
#define ain2_Pin            GPIO_PIN_9   /*!< 电机 A 方向引脚 2 (PA9) */
#define ain2_GPIO_Port      GPIOA

#define ain1_Pin            GPIO_PIN_10  /*!< 电机 A 方向引脚 1 (PA10) */
#define ain1_GPIO_Port      GPIOA

#define bin1_Pin            GPIO_PIN_11  /*!< 电机 B 方向引脚 1 (PA11) */
#define bin1_GPIO_Port      GPIOA

#define bin2_Pin            GPIO_PIN_12  /*!< 电机 B 方向引脚 2 (PA12) */
#define bin2_GPIO_Port      GPIOA

/* ---------- Buzzer & Standby pins ---------- */
#define beep_Pin            GPIO_PIN_1   /*!< 有源蜂鸣器驱动 (PB1) */
#define beep_GPIO_Port      GPIOB

#define stby_Pin            GPIO_PIN_15  /*!< TB6612 待机 (PB15) */
#define stby_GPIO_Port      GPIOB

/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
