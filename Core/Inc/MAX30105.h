/**
  ******************************************************************************
  * @file    MAX30105.h
  * @brief   MAX30105 多LED光学传感器驱动头文件。
  *          从 SparkFun MAX3010x 库移植到 STM32 HAL I2C。
  *          用于通过红外 LED 反射进行烟雾检测。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Based on SparkFun MAX30105 library, ported to STM32 HAL.
  ******************************************************************************
  */

#ifndef MAX30105_H
#define MAX30105_H

#include "main.h"

/* ===================== I2C 配置 ===================== */

extern I2C_HandleTypeDef hi2c1; /*!< MAX30105 使用的 I2C1 句柄 */

#define MAX30105_ADDRESS 0x57 /*!< 7位 I2C 地址 */

/* ===================== 多LED时隙常量 ===================== */

static const uint8_t SLOT_NONE        = 0x00;
static const uint8_t SLOT_RED_LED     = 0x01;
static const uint8_t SLOT_IR_LED      = 0x02;
static const uint8_t SLOT_GREEN_LED   = 0x03;
static const uint8_t SLOT_NONE_PILOT  = 0x04;
static const uint8_t SLOT_RED_PILOT   = 0x05;
static const uint8_t SLOT_IR_PILOT    = 0x06;
static const uint8_t SLOT_GREEN_PILOT = 0x07;

typedef uint8_t byte;

/* ===================== MAX30105 类 ===================== */

/**
  * @brief  MAX30105 光学传感器驱动。
  * @note   通过 I2C1 通信。先调用 begin() 再调用 setup() 进行配置。
  */
class MAX30105 {
public:
    MAX30105(void);

    /**
      * @brief  初始化 I2C 通信并验证器件 ID。
      * @param  i2caddr  7位 I2C 地址（默认 0x57）
      * @retval 器件 ID 匹配返回 true，通信失败返回 false。
      */
    bool begin(uint8_t i2caddr = MAX30105_ADDRESS);

    /* 数据访问 */
    uint32_t getRed(void);
    uint32_t getIR(void);
    uint32_t getGreen(void);
    bool safeCheck(uint8_t maxTimeToCheck);

    /* 配置 */
    void softReset();
    void shutDown();
    void wakeUp();
    void setLEDMode(uint8_t mode);
    void setADCRange(uint8_t adcRange);
    void setSampleRate(uint8_t sampleRate);
    void setPulseWidth(uint8_t pulseWidth);
    void setPulseAmplitudeRed(uint8_t value);
    void setPulseAmplitudeIR(uint8_t value);
    void setPulseAmplitudeGreen(uint8_t value);
    void setPulseAmplitudeProximity(uint8_t value);
    void setProximityThreshold(uint8_t threshMSB);

    /* 多LED时隙配置 */
    void enableSlot(uint8_t slotNumber, uint8_t device);
    void disableSlots(void);

    /* 中断控制 */
    uint8_t getINT1(void);
    uint8_t getINT2(void);
    void enableAFULL(void);
    void disableAFULL(void);
    void enableDATARDY(void);
    void disableDATARDY(void);
    void enableALCOVF(void);
    void disableALCOVF(void);
    void enablePROXINT(void);
    void disablePROXINT(void);
    void enableDIETEMPRDY(void);
    void disableDIETEMPRDY(void);

    /* FIFO 配置 */
    void setFIFOAverage(uint8_t samples);
    void enableFIFORollover();
    void disableFIFORollover();
    void setFIFOAlmostFull(uint8_t samples);
    void clearFIFO(void);
    uint8_t getWritePointer(void);
    uint8_t getReadPointer(void);

    /* FIFO 数据读取 */
    uint16_t check(void);
    uint8_t available(void);
    void nextSample(void);
    uint32_t getFIFORed(void);
    uint32_t getFIFOIR(void);
    uint32_t getFIFOGreen(void);

    /* 接近检测阈值 */
    void setPROXINTTHRESH(uint8_t val);

    /* 温度读取 */
    float readTemperature();
    float readTemperatureF();

    /* ID 与版本 */
    uint8_t getRevisionID();
    uint8_t readPartID();

    /**
      * @brief  使用常用参数快速配置。
      * @param  powerLevel      LED 电流（0x00..0xFF，默认 0x1F）
      * @param  sampleAverage   FIFO 平均（1,2,4,8,16,32，默认 4）
      * @param  ledMode         LED 模式（1=红光, 2=红光+红外, 3=红光+红外+绿光，默认 3）
      * @param  sampleRate      采样率（50..3200 Hz，默认 400）
      * @param  pulseWidth      脉冲宽度，单位微秒（69,118,215,411，默认 411）
      * @param  adcRange        ADC 量程（2048,4096,8192,16384，默认 4096）
      */
    void setup(uint8_t powerLevel = 0x1F, uint8_t sampleAverage = 4,
               uint8_t ledMode = 3, int sampleRate = 400,
               int pulseWidth = 411, int adcRange = 4096);

    /* 底层 I2C */
    uint8_t readRegister8(uint8_t address, uint8_t reg);
    void writeRegister8(uint8_t address, uint8_t reg, uint8_t value);

private:
    uint8_t _i2caddr;
    uint8_t activeLEDs; /*!< 有效 LED 数量（1..3） */
    uint8_t revisionID;

    void readRevisionID();
    void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);

    uint32_t _millis(void) { return HAL_GetTick(); }
    void _delay(uint32_t ms) { HAL_Delay(ms); }
};

#endif /* MAX30105_H */
