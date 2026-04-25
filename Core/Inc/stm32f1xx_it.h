/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.h
  * @brief   STM32F1xx 中断服务例程原型声明。
  *          声明 Cortex-M3 系统异常处理程序及健康监测项目中使用的外设中断
  *          （DMA1 通道 2/3 用于 SPI）。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __STM32F1xx_IT_H
#define __STM32F1xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

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

/* ---------- Cortex-M3 系统异常处理程序 ---------- */
void NMI_Handler(void);           /*!< 不可屏蔽中断 */
void HardFault_Handler(void);     /*!< 硬 fault（总线/用法/内存） */
void MemManage_Handler(void);     /*!< 内存管理 fault */
void BusFault_Handler(void);      /*!< 总线 fault */
void UsageFault_Handler(void);    /*!< 用法 fault / 未定义指令 */
void SVC_Handler(void);           /*!< 通过 SWI 的 SVCall */
void DebugMon_Handler(void);      /*!< 调试监控器 */
void PendSV_Handler(void);        /*!< 可挂起的服务请求 */
void SysTick_Handler(void);       /*!< 系统滴答定时器（1 ms） */

/* ---------- 外设 IRQ 处理程序 ---------- */
void DMA1_Channel2_IRQHandler(void); /*!< SPI1 RX DMA 完成 / 半传输中断 */
void DMA1_Channel3_IRQHandler(void); /*!< SPI1 TX DMA 完成 / 半传输中断 */

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_IT_H */
