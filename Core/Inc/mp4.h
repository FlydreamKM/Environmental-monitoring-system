/**
  ******************************************************************************
  * @file    mp4.h
  * @brief   MP-4 可燃气体传感器 HAL 驱动头文件（3.3 V 加热）。
  * @note    硬件连接：
  *            - 引脚 1,2（加热器）：直接接 3.3 V（持续加热）
  *            - 引脚 3,4（感应）：3.3 V -> RS -> RL (10K) -> GND
  *            - VRL（RL 中点）-> STM32 ADC1_IN1 (PA1)
  *          重要：3.3 V 加热功率约为 5 V 的 43%；灵敏度显著降低。
  *          R0 必须在 3.3 V 下重新校准。
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#ifndef __MP4_H
#define __MP4_H

#include "main.h"

/* ===================== 电路参数 ===================== */

#define MP4_ADC_HANDLE  hadc1          /*!< 用于读取 VRL 的 HAL ADC 句柄 */
#define MP4_ADC_CHANNEL ADC_CHANNEL_1  /*!< VRL 的 ADC 通道（PA1） */

#define MP4_VC_VOLTAGE        3.3f   /*!< 感应回路供电电压（V） */
#define MP4_RL_RESISTANCE     10000.0f /*!< 负载电阻 RL（欧姆） */
#define MP4_VOLTAGE_DIV_RATIO 1.0f   /*!< 外部分压比（1.0 = 直接连接） */

/* ===================== 3.3 V 加热补偿 ===================== */

/**
  * @brief  3.3 V 加热的经验补偿系数。
  * @note   3.3 V 功率 = (3.3/5)^2，约为 5 V 的 43.5%。传感器灵敏度下降。
  *         该系数（~0.65）将 Ratio 缩放回近似 5 V 曲线。
  *         使用已知浓度气体进行调校以提高精度。
  */
#define MP4_HEAT_COMPENSATION 0.65f

/* ===================== 浓度模型（对数线性） ===================== */

#define MP4_CURVE_A  12.5f   /*!< 曲线系数 a（5 V 数据手册） */
#define MP4_CURVE_B -0.699f  /*!< 曲线系数 b（5 V 数据手册） */

/* ===================== 报警与时序 ===================== */

#define MP4_ALARM_THRESHOLD 1000.0f /*!< ppm 报警阈值 */
#define MP4_WARMUP_SECONDS  1800    /*!< 预热时间：3.3 V 下 30 分钟 */
#define MP4_SAMPLE_TIMES    50      /*!< ADC 平均采样次数 */

/* ===================== 数据结构 ===================== */

/** @brief MP-4 驱动状态码。 */
typedef enum {
    MP4_STATUS_OK = 0,          /*!< 读取成功 */
    MP4_STATUS_WARMING_UP,      /*!< 预热中，数据尚未稳定 */
    MP4_STATUS_NOT_CALIBRATED,  /*!< 未进行 R0 校准 */
    MP4_STATUS_ERROR            /*!< ADC 读取失败或超出范围 */
} MP4_Status_t;

/** @brief MP-4 计算数据快照。 */
typedef struct {
    float    Vrl;           /*!< 负载电阻电压（V） */
    float    Rs;            /*!< 传感器电阻（欧姆） */
    float    R0;            /*!< 清洁空气电阻（欧姆）@ 3.3 V */
    float    Ratio;         /*!< Rs / R0 比值 */
    float    Ppm;           /*!< 估计甲烷浓度（ppm） */
    uint8_t  IsCalibrated;  /*!< 1 = R0 已校准 */
    uint32_t PowerOnTime;   /*!< 累计上电时间（秒） */
    uint8_t  IsWarmedUp;    /*!< 1 = 预热完成 */
} MP4_Data_t;

/* ===================== API 原型 ===================== */

/**
  * @brief  初始化内部数据结构（全部字段清零）。
  */
void MP4_Init(void);

/**
  * @brief  在清洁空气中执行 R0 校准。
  * @note   必须在清洁环境中预热至少 30 分钟后调用。
  *         3.3 V 下的 R0 与 5 V 下的 R0 不同；不要混用数值。
  * @retval MP4_Status_t
  */
MP4_Status_t MP4_CalibrateR0(void);

/**
  * @brief  读取传感器，计算 Rs、Ratio 和 ppm（含 3.3 V 补偿）。
  * @note   以约 1 Hz 频率调用。每次调用内部累加 PowerOnTime。
  * @retval MP4_Status_t
  */
MP4_Status_t MP4_Read(void);

/**
  * @brief  获取最新数据快照指针。
  * @retval 指向内部 MP4_Data_t 结构的指针。
  */
MP4_Data_t* MP4_GetData(void);

/**
  * @brief  检查当前浓度是否超过报警阈值。
  * @retval 0 = 安全，1 = 报警
  */
uint8_t MP4_IsAlarm(void);

/**
  * @brief  手动设置预热时间（用于调试/跳过长时间预热）。
  * @param  seconds  模拟的上电时间，单位秒。
  */
void MP4_SetWarmUpTime(uint32_t seconds);

#endif /* __MP4_H */
