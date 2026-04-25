/**
  ******************************************************************************
  * @file    VEML7700_HAL.cpp
  * @brief   VEML7700 环境光传感器 HAL 驱动实现。
  *          从 Adafruit_VEML7700 Arduino 库移植到 STM32 HAL I2C。
  *          支持手动和自动 Lux 测量，并按照 Vishay 应用笔记 84323
  *          进行非线性校正。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright 基于 Adafruit VEML7700 驱动，移植到 STM32 HAL。
  ******************************************************************************
  */

#include "VEML7700_HAL.h"

/* ===================== 类常量 ===================== */

const float VEML7700_HAL::MAX_RES  = 0.0036f;
const float VEML7700_HAL::GAIN_MAX = 2.0f;
const float VEML7700_HAL::IT_MAX   = 800.0f;

/* ===================== 构造函数 ===================== */

VEML7700_HAL::VEML7700_HAL(void) {}

/* ===================== 初始化 ===================== */

bool VEML7700_HAL::begin(I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    _hi2c = hi2c;
    _addr = (uint16_t)(addr << 1); /*!< HAL 要求左对齐地址。 */

    /* 验证总线上是否存在该设备。 */
    if (HAL_I2C_IsDeviceReady(_hi2c, _addr, 3, 100) != HAL_OK) {
        return false;
    }

    /* 默认初始化序列（与原始 Arduino 驱动一致）。 */
    enable(false);
    interruptEnable(false);
    setPersistence(VEML7700_PERS_1);
    setGain(VEML7700_GAIN_1_8);
    setIntegrationTime(VEML7700_IT_100MS, false);
    powerSaveEnable(false);
    enable(true);

    lastRead = HAL_GetTick();
    return true;
}

/* ===================== 底层寄存器访问 ===================== */

bool VEML7700_HAL::writeRegister(uint8_t reg, uint16_t value)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(value & 0xFF);        /*!< 低字节在前 */
    buf[1] = (uint8_t)((value >> 8) & 0xFF); /*!< 高字节在后 */
    return (HAL_I2C_Mem_Write(_hi2c, _addr, reg,
                              I2C_MEMADD_SIZE_8BIT, buf, 2, 100) == HAL_OK);
}

uint16_t VEML7700_HAL::readRegister(uint8_t reg)
{
    uint8_t buf[2];
    if (HAL_I2C_Mem_Read(_hi2c, _addr, reg,
                         I2C_MEMADD_SIZE_8BIT, buf, 2, 100) != HAL_OK) {
        return 0;
    }
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

/* ===================== 电源控制 ===================== */

void VEML7700_HAL::enable(bool enable)
{
    uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
    if (enable) {
        cfg &= ~(uint16_t)0x0001; /*!< bit 0 = 0 -> 激活 */
    } else {
        cfg |= 0x0001;            /*!< bit 0 = 1 -> 关断 */
    }
    writeRegister(VEML7700_ALS_CONFIG, cfg);

    /* 激活后等待 5 ms（数据手册规格：最小 2.5 ms）。 */
    if (enable) {
        HAL_Delay(5);
    }
}

bool VEML7700_HAL::enabled(void)
{
    return (readRegister(VEML7700_ALS_CONFIG) & 0x0001) == 0;
}

/* ===================== 中断控制 ===================== */

void VEML7700_HAL::interruptEnable(bool enable)
{
    uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
    if (enable) {
        cfg |= 0x0002;            /*!< bit 1 = 1 */
    } else {
        cfg &= ~(uint16_t)0x0002; /*!< bit 1 = 0 */
    }
    writeRegister(VEML7700_ALS_CONFIG, cfg);
}

bool VEML7700_HAL::interruptEnabled(void)
{
    return (readRegister(VEML7700_ALS_CONFIG) & 0x0002) != 0;
}

void VEML7700_HAL::setPersistence(uint8_t pers)
{
    uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
    cfg &= ~(uint16_t)(0x0003 << 4);       /*!< 清除 bit 4-5 */
    cfg |= (uint16_t)((pers & 0x03) << 4); /*!< 设置 bit 4-5 */
    writeRegister(VEML7700_ALS_CONFIG, cfg);
}

uint8_t VEML7700_HAL::getPersistence(void)
{
    return (uint8_t)((readRegister(VEML7700_ALS_CONFIG) >> 4) & 0x03);
}

/* ===================== 积分时间 ===================== */

void VEML7700_HAL::setIntegrationTime(uint8_t it, bool wait)
{
    int flushDelay = wait ? getIntegrationTimeValue() : 0;

    uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
    cfg &= ~(uint16_t)(0x000F << 6);       /*!< 清除 bit 6-9 */
    cfg |= (uint16_t)((it & 0x0F) << 6);   /*!< 设置 bit 6-9 */
    writeRegister(VEML7700_ALS_CONFIG, cfg);

    HAL_Delay(flushDelay);
    lastRead = HAL_GetTick();
}

uint8_t VEML7700_HAL::getIntegrationTime(void)
{
    return (uint8_t)((readRegister(VEML7700_ALS_CONFIG) >> 6) & 0x0F);
}

int VEML7700_HAL::getIntegrationTimeValue(void)
{
    switch (getIntegrationTime()) {
        case VEML7700_IT_25MS:  return 25;
        case VEML7700_IT_50MS:  return 50;
        case VEML7700_IT_100MS: return 100;
        case VEML7700_IT_200MS: return 200;
        case VEML7700_IT_400MS: return 400;
        case VEML7700_IT_800MS: return 800;
        default: return -1;
    }
}

/* ===================== 增益控制 ===================== */

void VEML7700_HAL::setGain(uint8_t gain)
{
    uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
    cfg &= ~(uint16_t)(0x0003 << 11);       /*!< 清除 bit 11-12 */
    cfg |= (uint16_t)((gain & 0x03) << 11); /*!< 设置 bit 11-12 */
    writeRegister(VEML7700_ALS_CONFIG, cfg);
    lastRead = HAL_GetTick(); /*!< 重置时间基准 */
}

uint8_t VEML7700_HAL::getGain(void)
{
    return (uint8_t)((readRegister(VEML7700_ALS_CONFIG) >> 11) & 0x03);
}

float VEML7700_HAL::getGainValue(void)
{
    switch (getGain()) {
        case VEML7700_GAIN_1_8: return 0.125f;
        case VEML7700_GAIN_1_4: return 0.25f;
        case VEML7700_GAIN_1:   return 1.0f;
        case VEML7700_GAIN_2:   return 2.0f;
        default: return -1.0f;
    }
}

/* ===================== 省电模式 ===================== */

void VEML7700_HAL::powerSaveEnable(bool enable)
{
    uint16_t psm = readRegister(VEML7700_ALS_POWER_SAVE);
    if (enable) {
        psm |= 0x0001;            /*!< bit 0 = 1 */
    } else {
        psm &= ~(uint16_t)0x0001; /*!< bit 0 = 0 */
    }
    writeRegister(VEML7700_ALS_POWER_SAVE, psm);
}

bool VEML7700_HAL::powerSaveEnabled(void)
{
    return (readRegister(VEML7700_ALS_POWER_SAVE) & 0x0001) != 0;
}

void VEML7700_HAL::setPowerSaveMode(uint8_t mode)
{
    uint16_t psm = readRegister(VEML7700_ALS_POWER_SAVE);
    psm &= ~(uint16_t)(0x0003 << 1);       /*!< 清除 bit 1-2 */
    psm |= (uint16_t)((mode & 0x03) << 1); /*!< 设置 bit 1-2 */
    writeRegister(VEML7700_ALS_POWER_SAVE, psm);
}

uint8_t VEML7700_HAL::getPowerSaveMode(void)
{
    return (uint8_t)((readRegister(VEML7700_ALS_POWER_SAVE) >> 1) & 0x03);
}

/* ===================== 阈值 ===================== */

void VEML7700_HAL::setLowThreshold(uint16_t value)
{
    writeRegister(VEML7700_ALS_THREHOLD_LOW, value);
}

uint16_t VEML7700_HAL::getLowThreshold(void)
{
    return readRegister(VEML7700_ALS_THREHOLD_LOW);
}

void VEML7700_HAL::setHighThreshold(uint16_t value)
{
    writeRegister(VEML7700_ALS_THREHOLD_HIGH, value);
}

uint16_t VEML7700_HAL::getHighThreshold(void)
{
    return readRegister(VEML7700_ALS_THREHOLD_HIGH);
}

uint16_t VEML7700_HAL::interruptStatus(void)
{
    return readRegister(VEML7700_INTERRUPTSTATUS);
}

/* ===================== 数据读取 ===================== */

uint16_t VEML7700_HAL::readALS(bool wait)
{
    if (wait)
        readWait();
    lastRead = HAL_GetTick();
    return readRegister(VEML7700_ALS_DATA);
}

uint16_t VEML7700_HAL::readWhite(bool wait)
{
    if (wait)
        readWait();
    lastRead = HAL_GetTick();
    return readRegister(VEML7700_WHITE_DATA);
}

float VEML7700_HAL::readLux(luxMethod method)
{
    bool wait = true;
    switch (method) {
        case VEML_LUX_NORMAL_NOWAIT:
            wait = false;
            /* 继续执行下一个 case */
        case VEML_LUX_NORMAL:
            return computeLux(readALS(wait));
        case VEML_LUX_CORRECTED_NOWAIT:
            wait = false;
            /* 继续执行下一个 case */
        case VEML_LUX_CORRECTED:
            return computeLux(readALS(wait), true);
        case VEML_LUX_AUTO:
            return autoLux();
        default:
            return -1.0f;
    }
}

/* ===================== Lux 计算 ===================== */

float VEML7700_HAL::getResolution(void)
{
    return MAX_RES * (IT_MAX / (float)getIntegrationTimeValue()) *
           (GAIN_MAX / getGainValue());
}

float VEML7700_HAL::computeLux(uint16_t rawALS, bool corrected)
{
    float lux = getResolution() * (float)rawALS;
    if (corrected) {
        /* Vishay 非线性校正多项式（应用笔记 84323）。 */
        lux = (((6.0135e-13f * lux - 9.3924e-9f) * lux + 8.1488e-5f) * lux +
               1.0023f) *
              lux;
    }
    return lux;
}

void VEML7700_HAL::readWait(void)
{
    uint32_t timeToWait = 2 * (uint32_t)getIntegrationTimeValue();
    uint32_t timeWaited = HAL_GetTick() - lastRead;

    if (timeWaited < timeToWait)
        HAL_Delay(timeToWait - timeWaited);
}

/* ===================== 自动 Lux（应用笔记流程图） ===================== */

float VEML7700_HAL::autoLux(void)
{
    const uint8_t gains[]    = {VEML7700_GAIN_1_8, VEML7700_GAIN_1_4,
                                VEML7700_GAIN_1,   VEML7700_GAIN_2};
    const uint8_t intTimes[] = {VEML7700_IT_25MS,  VEML7700_IT_50MS,
                                VEML7700_IT_100MS, VEML7700_IT_200MS,
                                VEML7700_IT_400MS, VEML7700_IT_800MS};

    uint8_t gainIndex    = 0;      /*!< 从 1/8 增益开始 */
    uint8_t itIndex      = 2;      /*!< 从 100 ms 开始 */
    bool useCorrection   = false;  /*!< 非线性校正标志 */

    setGain(gains[gainIndex]);
    setIntegrationTime(intTimes[itIndex]);

    uint16_t ALS = readALS(true);

    if (ALS <= 100) {
        /* 先增大增益，再增大积分时间。 */
        while ((ALS <= 100) && !((gainIndex == 3) && (itIndex == 5))) {
            if (gainIndex < 3) {
                setGain(gains[++gainIndex]);
            } else if (itIndex < 5) {
                setIntegrationTime(intTimes[++itIndex]);
            }
            ALS = readALS(true);
        }
    } else {
        /* 信号过强时减小积分时间。 */
        useCorrection = true;
        while ((ALS > 10000) && (itIndex > 0)) {
            setIntegrationTime(intTimes[--itIndex]);
            ALS = readALS(true);
        }
    }

    return computeLux(ALS, useCorrection);
}
