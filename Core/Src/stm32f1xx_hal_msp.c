/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_msp.c
  * @brief   HAL MSP（MCU 支持包）初始化和反初始化。
  *          配置项目中所用外设 ADC1、I2C1/2、RTC、SPI1 和 TIM1/2 的
  *          外设时钟、GPIO 复用功能和 DMA 链路。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* 在 main.c 中声明的外部 DMA 句柄 */
extern DMA_HandleTypeDef hdma_spi1_rx;
extern DMA_HandleTypeDef hdma_spi1_tx;

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */
/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */
/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */
/* USER CODE END ExternalFunctions */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  初始化全局 MSP。
  * @note   使能 AFIO 和 PWR 时钟，重映射 JTAG-DP 关闭并保留 SW-DP。
  * @retval None
  */
void HAL_MspInit(void)
{
    /* USER CODE BEGIN MspInit 0 */
    /* USER CODE END MspInit 0 */

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* 系统中断初始化 */

    /** NOJTAG: JTAG-DP 禁用，SW-DP 使能 */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    /* USER CODE BEGIN MspInit 1 */
    /* USER CODE END MspInit 1 */
}

/* ===================== ADC MSP ===================== */

/**
  * @brief  ADC1 MSP 初始化。
  * @note   使能 ADC1 时钟并将 PA1 配置为模拟输入（ADC1_IN1）。
  * @param  hadc  ADC 句柄指针
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hadc->Instance == ADC1)
    {
        /* USER CODE BEGIN ADC1_MspInit 0 */
        /* USER CODE END ADC1_MspInit 0 */

        /* 外设时钟使能 */
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA1 -> ADC1_IN1（MP-4 VRL） */
        GPIO_InitStruct.Pin  = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USER CODE BEGIN ADC1_MspInit 1 */
        /* USER CODE END ADC1_MspInit 1 */
    }
}

/**
  * @brief  ADC1 MSP 反初始化。
  * @param  hadc  ADC 句柄指针
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        /* USER CODE BEGIN ADC1_MspDeInit 0 */
        /* USER CODE END ADC1_MspDeInit 0 */

        /* 外设时钟禁用 */
        __HAL_RCC_ADC1_CLK_DISABLE();

        /* PA1 -> ADC1_IN1 反初始化 */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);

        /* USER CODE BEGIN ADC1_MspDeInit 1 */
        /* USER CODE END ADC1_MspDeInit 1 */
    }
}

/* ===================== I2C MSP ===================== */

/**
  * @brief  I2C MSP 初始化。
  * @note   I2C1：PB8/PB9 重映射为 SCL/SDA（MAX30105）。
  *         I2C2：PB10/PB11 为 SCL/SDA（SHT40 + VEML7700）。
  * @param  hi2c  I2C 句柄指针
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C1)
    {
        /* USER CODE BEGIN I2C1_MspInit 0 */
        /* USER CODE END I2C1_MspInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PB8 -> I2C1_SCL, PB9 -> I2C1_SDA（重映射） */
        GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        __HAL_AFIO_REMAP_I2C1_ENABLE();

        /* 外设时钟使能 */
        __HAL_RCC_I2C1_CLK_ENABLE();

        /* USER CODE BEGIN I2C1_MspInit 1 */
        /* USER CODE END I2C1_MspInit 1 */
    }
    else if (hi2c->Instance == I2C2)
    {
        /* USER CODE BEGIN I2C2_MspInit 0 */
        /* USER CODE END I2C2_MspInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PB10 -> I2C2_SCL, PB11 -> I2C2_SDA */
        GPIO_InitStruct.Pin   = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* 外设时钟使能 */
        __HAL_RCC_I2C2_CLK_ENABLE();

        /* USER CODE BEGIN I2C2_MspInit 1 */
        /* USER CODE END I2C2_MspInit 1 */
    }
}

/**
  * @brief  I2C MSP 反初始化。
  * @param  hi2c  I2C 句柄指针
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        /* USER CODE BEGIN I2C1_MspDeInit 0 */
        /* USER CODE END I2C1_MspDeInit 0 */

        __HAL_RCC_I2C1_CLK_DISABLE();

        /* PB8/PB9 反初始化 */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

        /* USER CODE BEGIN I2C1_MspDeInit 1 */
        /* USER CODE END I2C1_MspDeInit 1 */
    }
    else if (hi2c->Instance == I2C2)
    {
        /* USER CODE BEGIN I2C2_MspDeInit 0 */
        /* USER CODE END I2C2_MspDeInit 0 */

        __HAL_RCC_I2C2_CLK_DISABLE();

        /* PB10/PB11 反初始化 */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);

        /* USER CODE BEGIN I2C2_MspDeInit 1 */
        /* USER CODE END I2C2_MspDeInit 1 */
    }
}

/* ===================== RTC MSP ===================== */

/**
  * @brief  RTC MSP 初始化。
  * @note   使能备份域访问、BKP 和 RTC 时钟。
  * @param  hrtc  RTC 句柄指针
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        /* USER CODE BEGIN RTC_MspInit 0 */
        /* USER CODE END RTC_MspInit 0 */

        HAL_PWR_EnableBkUpAccess();
        __HAL_RCC_BKP_CLK_ENABLE();
        __HAL_RCC_RTC_ENABLE();

        /* USER CODE BEGIN RTC_MspInit 1 */
        /* USER CODE END RTC_MspInit 1 */
    }
}

/**
  * @brief  RTC MSP 反初始化。
  * @param  hrtc  RTC 句柄指针
  * @retval None
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        /* USER CODE BEGIN RTC_MspDeInit 0 */
        /* USER CODE END RTC_MspDeInit 0 */

        __HAL_RCC_RTC_DISABLE();

        /* USER CODE BEGIN RTC_MspDeInit 1 */
        /* USER CODE END RTC_MspDeInit 1 */
    }
}

/* ===================== SPI MSP ===================== */

/**
  * @brief  SPI1 MSP 初始化。
  * @note   使能 SPI1 时钟，将 PA5（SCK）和 PA7（MOSI）配置为复用推挽输出。
  *         同时初始化 DMA1 通道 2（RX）和通道 3（TX），普通模式，字节宽度。
  * @param  hspi  SPI 句柄指针
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hspi->Instance == SPI1)
    {
        /* USER CODE BEGIN SPI1_MspInit 0 */
        /* USER CODE END SPI1_MspInit 0 */

        /* 外设时钟使能 */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA5 -> SPI1_SCK, PA7 -> SPI1_MOSI */
        GPIO_InitStruct.Pin   = GPIO_PIN_5 | GPIO_PIN_7;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* SPI1 DMA RX：通道 2，外设到存储器，字节，普通模式，低优先级。 */
        hdma_spi1_rx.Instance                 = DMA1_Channel2;
        hdma_spi1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_spi1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_spi1_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_spi1_rx.Init.Mode                = DMA_NORMAL;
        hdma_spi1_rx.Init.Priority            = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK)
        {
            Error_Handler();
        }
        __HAL_LINKDMA(hspi, hdmarx, hdma_spi1_rx);

        /* SPI1 DMA TX：通道 3，存储器到外设，字节，普通模式，低优先级。 */
        hdma_spi1_tx.Instance                 = DMA1_Channel3;
        hdma_spi1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hdma_spi1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_spi1_tx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_spi1_tx.Init.Mode                = DMA_NORMAL;
        hdma_spi1_tx.Init.Priority            = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
        {
            Error_Handler();
        }
        __HAL_LINKDMA(hspi, hdmatx, hdma_spi1_tx);

        /* USER CODE BEGIN SPI1_MspInit 1 */
        /* USER CODE END SPI1_MspInit 1 */
    }
}

/**
  * @brief  SPI1 MSP 反初始化。
  * @param  hspi  SPI 句柄指针
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        /* USER CODE BEGIN SPI1_MspDeInit 0 */
        /* USER CODE END SPI1_MspDeInit 0 */

        __HAL_RCC_SPI1_CLK_DISABLE();

        /* PA5/PA7 反初始化 */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_7);

        /* SPI1 DMA 反初始化 */
        HAL_DMA_DeInit(hspi->hdmarx);
        HAL_DMA_DeInit(hspi->hdmatx);

        /* USER CODE BEGIN SPI1_MspDeInit 1 */
        /* USER CODE END SPI1_MspDeInit 1 */
    }
}

/* ===================== TIM MSP ===================== */

/**
  * @brief  TIM PWM MSP 初始化。
  * @note   使能 TIM1 和 TIM2 外设时钟。
  * @param  htim_pwm  TIM 句柄指针
  * @retval None
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim_pwm)
{
    if (htim_pwm->Instance == TIM1)
    {
        /* USER CODE BEGIN TIM1_MspInit 0 */
        /* USER CODE END TIM1_MspInit 0 */
        __HAL_RCC_TIM1_CLK_ENABLE();
        /* USER CODE BEGIN TIM1_MspInit 1 */
        /* USER CODE END TIM1_MspInit 1 */
    }
    else if (htim_pwm->Instance == TIM2)
    {
        /* USER CODE BEGIN TIM2_MspInit 0 */
        /* USER CODE END TIM2_MspInit 0 */
        __HAL_RCC_TIM2_CLK_ENABLE();
        /* USER CODE BEGIN TIM2_MspInit 1 */
        /* USER CODE END TIM2_MspInit 1 */
    }
}

/**
  * @brief  TIM MSP 后置初始化（GPIO 复用功能配置）。
  * @note   配置 PA8（TIM1_CH1）和 PA15（TIM2_CH1，部分重映射）。
  * @param  htim  TIM 句柄指针
  * @retval None
  */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM1)
    {
        /* USER CODE BEGIN TIM1_MspPostInit 0 */
        /* USER CODE END TIM1_MspPostInit 0 */

        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA8 -> TIM1_CH1（电机 A PWM） */
        GPIO_InitStruct.Pin   = GPIO_PIN_8;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USER CODE BEGIN TIM1_MspPostInit 1 */
        /* USER CODE END TIM1_MspPostInit 1 */
    }
    else if (htim->Instance == TIM2)
    {
        /* USER CODE BEGIN TIM2_MspPostInit 0 */
        /* USER CODE END TIM2_MspPostInit 0 */

        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA15 -> TIM2_CH1（电机 B PWM，部分重映射） */
        GPIO_InitStruct.Pin   = GPIO_PIN_15;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        __HAL_AFIO_REMAP_TIM2_PARTIAL_1();

        /* USER CODE BEGIN TIM2_MspPostInit 1 */
        /* USER CODE END TIM2_MspPostInit 1 */
    }
}

/**
  * @brief  TIM PWM MSP 反初始化。
  * @param  htim_pwm  TIM 句柄指针
  * @retval None
  */
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim_pwm)
{
    if (htim_pwm->Instance == TIM1)
    {
        /* USER CODE BEGIN TIM1_MspDeInit 0 */
        /* USER CODE END TIM1_MspDeInit 0 */
        __HAL_RCC_TIM1_CLK_DISABLE();
        /* USER CODE BEGIN TIM1_MspDeInit 1 */
        /* USER CODE END TIM1_MspDeInit 1 */
    }
    else if (htim_pwm->Instance == TIM2)
    {
        /* USER CODE BEGIN TIM2_MspDeInit 0 */
        /* USER CODE END TIM2_MspDeInit 0 */
        __HAL_RCC_TIM2_CLK_DISABLE();
        /* USER CODE BEGIN TIM2_MspDeInit 1 */
        /* USER CODE END TIM2_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
/* USER CODE END 1 */
