#ifndef MAX30105_H
#define MAX30105_H

#include "main.h"

// 定义 I2C 句柄（外部声明，需在主程序中定义）
extern I2C_HandleTypeDef hi2c1;  // 默认使用 hi2c1，可修改

#define MAX30105_ADDRESS          0x57  // 7-bit I2C 地址

// 槽位常量（供 SmokeDetectorHAL 等外部使用）
static const uint8_t SLOT_NONE =        0x00;
static const uint8_t SLOT_RED_LED =     0x01;
static const uint8_t SLOT_IR_LED =      0x02;
static const uint8_t SLOT_GREEN_LED =   0x03;
static const uint8_t SLOT_NONE_PILOT =  0x04;
static const uint8_t SLOT_RED_PILOT =   0x05;
static const uint8_t SLOT_IR_PILOT =    0x06;
static const uint8_t SLOT_GREEN_PILOT = 0x07;

typedef uint8_t byte;

class MAX30105 {
public:
    MAX30105(void);

    // 初始化：不再需要传入 Wire 和速度参数，使用全局 hi2c1，速度由 CubeMX 配置
    bool begin(uint8_t i2caddr = MAX30105_ADDRESS);

    // 数据获取（与原始库一致）
    uint32_t getRed(void);
    uint32_t getIR(void);
    uint32_t getGreen(void);
    bool safeCheck(uint8_t maxTimeToCheck);

    // 配置函数（保持不变）
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

    // Multi-LED 槽位配置
    void enableSlot(uint8_t slotNumber, uint8_t device);
    void disableSlots(void);

    // 中断控制
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

    // FIFO 配置
    void setFIFOAverage(uint8_t samples);
    void enableFIFORollover();
    void disableFIFORollover();
    void setFIFOAlmostFull(uint8_t samples);
    void clearFIFO(void);
    uint8_t getWritePointer(void);
    uint8_t getReadPointer(void);

    // FIFO 数据读取
    uint16_t check(void);
    uint8_t available(void);
    void nextSample(void);
    uint32_t getFIFORed(void);
    uint32_t getFIFOIR(void);
    uint32_t getFIFOGreen(void);

    // 阈值设置
    void setPROXINTTHRESH(uint8_t val);

    // 温度读取
    float readTemperature();
    float readTemperatureF();

    // ID 与版本
    uint8_t getRevisionID();
    uint8_t readPartID();

    // 快速设置函数
    void setup(uint8_t powerLevel = 0x1F, uint8_t sampleAverage = 4, uint8_t ledMode = 3,
               int sampleRate = 400, int pulseWidth = 411, int adcRange = 4096);

    // 底层 I2C 读写（公开供外部使用，但一般内部调用）
    uint8_t readRegister8(uint8_t address, uint8_t reg);
    void writeRegister8(uint8_t address, uint8_t reg, uint8_t value);

private:
    uint8_t _i2caddr;
    uint8_t activeLEDs;          // 启用的 LED 数量（1~3）
    uint8_t revisionID;

    void readRevisionID();
    void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);

    // 替换 millis() 和 delay()
    uint32_t _millis(void) { return HAL_GetTick(); }
    void _delay(uint32_t ms) { HAL_Delay(ms); }
};

#endif