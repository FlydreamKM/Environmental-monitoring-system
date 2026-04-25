/**
  ******************************************************************************
  * @file    VEML7700_HAL.h
  * @brief   VEML7700 环境光传感器 HAL 驱动头文件。
  *          从 Adafruit_VEML7700 Arduino 库移植到 STM32 HAL I2C。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright 基于 Adafruit VEML7700 驱动，移植到 STM32 HAL。
  ******************************************************************************
  */

#ifndef _VEML7700_HAL_H
#define _VEML7700_HAL_H

#include "main.h"

/* ===================== I2C 与寄存器定义 ===================== */

#define VEML7700_I2CADDR_DEFAULT 0x10 /*!< 默认 7 位 I2C 地址 */

#define VEML7700_ALS_CONFIG         0x00 /*!< 光照配置寄存器 */
#define VEML7700_ALS_THREHOLD_HIGH  0x01 /*!< 中断高阈值 */
#define VEML7700_ALS_THREHOLD_LOW   0x02 /*!< 中断低阈值 */
#define VEML7700_ALS_POWER_SAVE     0x03 /*!< 省电寄存器 */
#define VEML7700_ALS_DATA           0x04 /*!< ALS 数据输出 */
#define VEML7700_WHITE_DATA         0x05 /*!< 白光数据输出 */
#define VEML7700_INTERRUPTSTATUS    0x06 /*!< 中断状态寄存器 */

#define VEML7700_INTERRUPT_HIGH 0x4000 /*!< 高阈值中断标志 */
#define VEML7700_INTERRUPT_LOW  0x8000 /*!< 低阈值中断标志 */

/* ===================== 增益常量 ===================== */

#define VEML7700_GAIN_1   0x00 /*!< ALS 增益 1x */
#define VEML7700_GAIN_2   0x01 /*!< ALS 增益 2x */
#define VEML7700_GAIN_1_8 0x02 /*!< ALS 增益 1/8x */
#define VEML7700_GAIN_1_4 0x03 /*!< ALS 增益 1/4x */

/* ===================== 积分时间常量 ===================== */

#define VEML7700_IT_100MS 0x00 /*!< 100 ms */
#define VEML7700_IT_200MS 0x01 /*!< 200 ms */
#define VEML7700_IT_400MS 0x02 /*!< 400 ms */
#define VEML7700_IT_800MS 0x03 /*!< 800 ms */
#define VEML7700_IT_50MS  0x08 /*!< 50 ms */
#define VEML7700_IT_25MS  0x0C /*!< 25 ms */

/* ===================== 持久性常量 ===================== */

#define VEML7700_PERS_1 0x00 /*!< 中断持久性 1 个样本 */
#define VEML7700_PERS_2 0x01 /*!< 中断持久性 2 个样本 */
#define VEML7700_PERS_4 0x02 /*!< 中断持久性 4 个样本 */
#define VEML7700_PERS_8 0x03 /*!< 中断持久性 8 个样本 */

/* ===================== 省电模式常量 ===================== */

#define VEML7700_POWERSAVE_MODE1 0x00
#define VEML7700_POWERSAVE_MODE2 0x01
#define VEML7700_POWERSAVE_MODE3 0x02
#define VEML7700_POWERSAVE_MODE4 0x03

/* ===================== Lux 读取方式 ===================== */

/** @brief Lux 计算方式选择。 */
typedef enum {
    VEML_LUX_NORMAL,          /*!< 原始 ALS Lux */
    VEML_LUX_CORRECTED,       /*!< 非线性校正 Lux */
    VEML_LUX_AUTO,            /*!< 自动增益 / 积分时间 */
    VEML_LUX_NORMAL_NOWAIT,   /*!< 原始 Lux，不等待积分完成 */
    VEML_LUX_CORRECTED_NOWAIT /*!< 校正 Lux，不等待积分完成 */
} luxMethod;

/* ===================== VEML7700_HAL 类 ===================== */

/**
  * @brief  VEML7700 环境光传感器驱动类。
  * @note   直接使用 STM32 HAL I2C API。使用前请先调用 begin()。
  */
class VEML7700_HAL {
public:
    VEML7700_HAL();

    /**
      * @brief  初始化传感器并配置默认参数。
      * @param  hi2c  HAL I2C 句柄指针
      * @param  addr  7 位 I2C 地址（默认 0x10）
      * @retval 若设备响应则为 true，否则为 false。
      */
    bool begin(I2C_HandleTypeDef *hi2c, uint8_t addr = VEML7700_I2CADDR_DEFAULT);

    /* 电源控制 */
    void enable(bool enable);
    bool enabled(void);

    /* 中断控制 */
    void interruptEnable(bool enable);
    bool interruptEnabled(void);
    void setPersistence(uint8_t pers);
    uint8_t getPersistence(void);

    /* 增益与积分时间 */
    void setIntegrationTime(uint8_t it, bool wait = true);
    uint8_t getIntegrationTime(void);
    int getIntegrationTimeValue(void);
    void setGain(uint8_t gain);
    uint8_t getGain(void);
    float getGainValue(void);

    /* 省电模式 */
    void powerSaveEnable(bool enable);
    bool powerSaveEnabled(void);
    void setPowerSaveMode(uint8_t mode);
    uint8_t getPowerSaveMode(void);

    /* 阈值 */
    void setLowThreshold(uint16_t value);
    uint16_t getLowThreshold(void);
    void setHighThreshold(uint16_t value);
    uint16_t getHighThreshold(void);
    uint16_t interruptStatus(void);

    /* 数据读取 */
    uint16_t readALS(bool wait = false);
    uint16_t readWhite(bool wait = false);

    /**
      * @brief  读取校准后的 Lux 值。
      * @param  method  Lux 计算方式（默认 NORMAL）
      * @retval Lux 值，出错时返回 -1.0。
      */
    float readLux(luxMethod method = VEML_LUX_NORMAL);

private:
    static const float MAX_RES; /*!< 最大分辨率常量 */
    static const float GAIN_MAX; /*!< 最大增益常量 */
    static const float IT_MAX;   /*!< 最大积分时间常量 */

    float getResolution(void);
    float computeLux(uint16_t rawALS, bool corrected = false);
    float autoLux(void);
    void readWait(void);

    uint32_t lastRead; /*!< 上次读取的时间戳，用于积分时序 */

    I2C_HandleTypeDef *_hi2c;
    uint16_t _addr;    /*!< 8 位 I2C 地址（7 位地址左移 1 位） */

    bool writeRegister(uint8_t reg, uint16_t value);
    uint16_t readRegister(uint8_t reg);
};

#endif /* _VEML7700_HAL_H */
