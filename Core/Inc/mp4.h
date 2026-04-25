/**
  * @file    mp4.h
  * @brief   MP-4 可燃气体传感器 STM32 HAL 驱动 (3.3V 加热降级版)
  * @note    硬件连接：
  *            - 1,2脚(加热极): 直连 3.3V (常供电，无GPIO控制)
  *            - 3,4脚(测量极): 一端接 3.3V(VC)，另一端串联 RL=10K 到 GND
  *            - RL 中点电压 VRL → STM32 ADC 引脚
  *          重要：3.3V 加热功率仅约 128mW（标准 5V 约 294mW），工作温度不足，
  *          灵敏度显著下降，必须重新标定 R0，且仅建议用于中高浓度泄漏报警。
  */

#ifndef __MP4_H
#define __MP4_H

#include "main.h"

/* ===================== 电路参数配置 ===================== */

/* ADC 硬件句柄与通道 (根据 CubeMX 配置修改) */
#define MP4_ADC_HANDLE          hadc1
#define MP4_ADC_CHANNEL         ADC_CHANNEL_1

/* 测量回路电压 VC = 3.3V (直连) */
#define MP4_VC_VOLTAGE          3.3f

/* 负载电阻 RL (单位: Ω)，建议 10KΩ，与传感器 RS 范围(1K~20K)匹配 */
#define MP4_RL_RESISTANCE       10000.0f

/* 分压比: 1.0 表示 ADC 直接测量 VRL，无外部电阻分压 */
#define MP4_VOLTAGE_DIV_RATIO   1.0f

/* ===================== 3.3V 加热补偿参数 ===================== */

/* 
 * 3.3V 加热功率仅为 5V 的 (3.3/5)^2 ≈ 43.5%，工作温度偏低，
 * 灵敏度衰减严重。此处引入经验补偿系数，将实测 Ratio 放大后
 * 再代入原 5V 标定曲线，以近似估算浓度。
 * 
 * 该值为经验估计 (sqrt(3.3/5.0) ≈ 0.65)，实际需用已知浓度气体标定。
 * 若测得浓度系统性偏低，请调小此值；若系统性偏高，请调大此值。
 */
#define MP4_HEAT_COMPENSATION   0.65f

/* 浓度计算模型参数 (基于手册图3甲烷曲线，5V标定) */
#define MP4_CURVE_A             12.5f
#define MP4_CURVE_B            -0.699f

/* 报警阈值 (ppm)。3.3V 下灵敏度低，建议放宽至 2000ppm 以上 */
#define MP4_ALARM_THRESHOLD     1000.0f

/* 预热时间 (秒)。3.3V 温度低，稳定极慢，建议不少于 30 分钟 */
#define MP4_WARMUP_SECONDS      1800    /* 1800s = 30min */

/* ADC 采样平均次数。3.3V 信号弱噪声大，建议 50 次滑动/突发平均 */
#define MP4_SAMPLE_TIMES        50

/* ===================== 数据结构 ===================== */

typedef enum {
    MP4_STATUS_OK = 0,
    MP4_STATUS_WARMING_UP,      /* 预热中，数据尚未稳定 */
    MP4_STATUS_NOT_CALIBRATED,  /* 未在洁净空气中执行 R0 校准 */
    MP4_STATUS_ERROR            /* ADC 读取失败 */
} MP4_Status_t;

typedef struct {
    float    Vrl;           /* 负载电阻电压 (V) */
    float    Rs;            /* 传感器当前电阻 (Ω) */
    float    R0;            /* 洁净空气下的传感器电阻 (Ω)，3.3V 下重新标定 */
    float    Ratio;         /* RS/R0 比值 */
    float    Ppm;           /* 估算甲烷浓度 (ppm) */
    uint8_t  IsCalibrated;  /* 校准标志: 1=已校准 */
    uint32_t PowerOnTime;   /* 上电累计时间 (秒) */
    uint8_t  IsWarmedUp;    /* 预热完成标志 */
} MP4_Data_t;

/* ===================== 函数接口 ===================== */

/**
  * @brief  初始化数据结构，清零状态
  */
void MP4_Init(void);

/**
  * @brief  在洁净空气中执行 R0 校准
  * @note   必须在 3.3V 预热至少 30 分钟后、洁净环境下调用。
  *         3.3V 的 R0 与 5V 的 R0 完全不同，不可混用！
  * @retval MP4_Status_t
  */
MP4_Status_t MP4_CalibrateR0(void);

/**
  * @brief  读取传感器并计算 RS、Ratio、Ppm (含 3.3V 补偿)
  * @note   建议以 1Hz 频率调用，内部自动累加上电时间
  * @retval MP4_Status_t
  */
MP4_Status_t MP4_Read(void);

/**
  * @brief  获取最近一次读取的数据结构指针
  * @retval MP4_Data_t*
  */
MP4_Data_t* MP4_GetData(void);

/**
  * @brief  检查当前浓度是否超过报警阈值
  * @retval 0=安全, 1=报警
  */
uint8_t MP4_IsAlarm(void);

/**
  * @brief  手动设置预热时间 (调试时跳过长时间预热用)
  */
void MP4_SetWarmUpTime(uint32_t seconds);

#endif /* __MP4_H */