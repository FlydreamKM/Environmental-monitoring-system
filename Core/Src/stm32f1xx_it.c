/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务例程实现。
  *          处理系统异常（HardFault、SysTick 等）和
  *          外设中断（DMA1 通道 2/3 用于 SPI1 传输）。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_spi1_rx; /*!< SPI1 RX DMA 句柄 */
extern DMA_HandleTypeDef hdma_spi1_tx; /*!< SPI1 TX DMA 句柄 */
/* USER CODE BEGIN EV */
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 处理器中断与异常处理程序                                */
/******************************************************************************/

/**
  * @brief  不可屏蔽中断处理程序。
  * @note   由硬件错误（时钟失效等）触发。
  *         进入无限循环以便调试。
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief  硬 fault 中断处理程序。
  * @note   由内存访问违规、总线错误或用法错误触发。
  *         进入无限循环以便连接调试器。
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief  内存管理 fault 处理程序。
  * @note   由 MPU 访问违规（MPU 使能时）或非法访问触发。
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief  总线 fault 处理程序。
  * @note   由无效的内存/总线访问（例如空指针）触发。
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief  用法 fault 处理程序。
  * @note   由未定义指令或未对齐访问触发。
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief  通过 SWI 指令发起的系统服务调用。
  * @note   由 RTOS 或系统调用机制使用。
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */
  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */
  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief  调试监控中断处理程序。
  * @note   当 CPU 被调试器暂停时触发。
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */
  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief  可挂起的系统服务请求。
  * @note   由 RTOS 用于上下文切换。
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */
  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */
  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief  系统滴答定时器中断处理程序（1 ms 节拍）。
  * @note   递增 HAL 全局节拍计数器，供 HAL_Delay / HAL_GetTick 使用。
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx 外设中断处理程序                                                  */
/******************************************************************************/

/**
  * @brief  DMA1 通道 2 全局中断处理程序。
  * @note   路由到 HAL DMA 处理程序，用于 SPI1 RX（hdma_spi1_rx）。
  */
void DMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */
  /* USER CODE END DMA1_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
  /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */
  /* USER CODE END DMA1_Channel2_IRQn 1 */
}

/**
  * @brief  DMA1 通道 3 全局中断处理程序。
  * @note   路由到 HAL DMA 处理程序，用于 SPI1 TX（hdma_spi1_tx）。
  *         供 ST7735 异步填充/图像传输使用。
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */
  /* USER CODE END DMA1_Channel3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
  /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */
  /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/* USER CODE BEGIN 1 */
/* USER CODE END 1 */
