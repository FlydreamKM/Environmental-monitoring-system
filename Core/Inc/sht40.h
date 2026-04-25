/**
  ******************************************************************************
  * @file    sht40.h
  * @brief   SHT40 温湿度传感器驱动头文件。
  *          通过 I2C2 提供阻塞式和非阻塞式 (NB) API。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright 基于公开 SHT40 示例，适配 STM32 HAL。
  ******************************************************************************
  */

#ifndef __SHT40_H
#define __SHT40_H

#include "main.h"

/* ===================== I2C 地址 ===================== */

#define SHT30_Write (0x44 << 1)      /*!< SHT40 I2C 写地址 */
#define SHT30_Read  ((0x44 << 1) + 1) /*!< SHT40 I2C 读地址 */

/* ===================== 命令常量 ===================== */

#define SHT40_MEASURE_TEMPERATURE_HUMIDITY 0xFD /*!< 高精度测量 */
#define SHT40_READ_SERIAL_NUMBER           0x89 /*!< 读取唯一 32 位序列号 */
#define SHT40_HEATER_200mW_1s              0x39 /*!< 加热器 200 mW，持续 1 秒 */

/* ===================== 阻塞式 API ===================== */

/**
  * @brief  读取温度和湿度（阻塞式，约 20 ms，含延时）。
  * @param  Temperature  温度输出指针，单位为 °C
  * @param  Humidity     相对湿度输出指针，单位为 %RH
  * @note   CRC 字节已接收但未校验。
  */
void SHT40_Read_Temperature_Humidity(double *Temperature, double *Humidity);

/**
  * @brief  读取工厂烧录的 32 位序列号。
  * @retval 序列号值。
  */
uint32_t SHT40_Read_Serial_Number(void);

/**
  * @brief  激活内部加热器（200 mW，持续 1 秒）以去除冷凝水。
  * @note   使用时间不得超过总运行时间的 10%，以避免过热。
  */
void SHT40_Heater_200mW_1s(void);

/* ===================== 非阻塞式 API ===================== */

/**
  * @brief  非阻塞状态机状态。
  */
typedef enum {
    SHT40_NB_IDLE = 0,   /*!< 就绪，可开始新测量 */
    SHT40_NB_WAITING     /*!< 测量命令已发送，等待转换完成 */
} SHT40_NB_State;

/**
  * @brief  获取当前非阻塞状态。
  * @retval 当前 SHT40_NB_State 值。
  */
SHT40_NB_State SHT40_NB_GetState(void);

/**
  * @brief  启动非阻塞测量（发送 I2C 命令，耗时 < 1 ms）。
  */
void SHT40_NB_Start(void);

/**
  * @brief  轮询非阻塞测量完成状态。
  * @note   在 Start() 后自动等待 >= 10 ms 再读取数据。
  * @param  Temperature  温度输出指针，单位为 °C
  * @param  Humidity     相对湿度输出指针，单位为 %RH
  * @retval 若读取到新数据且状态已返回 IDLE 则为 1，否则为 0。
  */
uint8_t SHT40_NB_Poll(double *Temperature, double *Humidity);

#endif /* __SHT40_H */
