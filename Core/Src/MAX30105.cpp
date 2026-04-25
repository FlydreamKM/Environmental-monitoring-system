#include "MAX30105.h"
#include <string.h>   // 用于 memcpy

// 寄存器地址定义（与原始库相同）
static const uint8_t MAX30105_INTSTAT1 =      0x00;
static const uint8_t MAX30105_INTSTAT2 =      0x01;
static const uint8_t MAX30105_INTENABLE1 =    0x02;
static const uint8_t MAX30105_INTENABLE2 =    0x03;
static const uint8_t MAX30105_FIFOWRITEPTR =  0x04;
static const uint8_t MAX30105_FIFOOVERFLOW =  0x05;
static const uint8_t MAX30105_FIFOREADPTR =   0x06;
static const uint8_t MAX30105_FIFODATA =      0x07;
static const uint8_t MAX30105_FIFOCONFIG =    0x08;
static const uint8_t MAX30105_MODECONFIG =    0x09;
static const uint8_t MAX30105_PARTICLECONFIG =0x0A;
static const uint8_t MAX30105_LED1_PULSEAMP = 0x0C;
static const uint8_t MAX30105_LED2_PULSEAMP = 0x0D;
static const uint8_t MAX30105_LED3_PULSEAMP = 0x0E;
static const uint8_t MAX30105_LED_PROX_AMP =  0x10;
static const uint8_t MAX30105_MULTILEDCONFIG1=0x11;
static const uint8_t MAX30105_MULTILEDCONFIG2=0x12;
static const uint8_t MAX30105_DIETEMPINT =    0x1F;
static const uint8_t MAX30105_DIETEMPFRAC =   0x20;
static const uint8_t MAX30105_DIETEMPCONFIG = 0x21;
static const uint8_t MAX30105_PROXINTTHRESH = 0x30;
static const uint8_t MAX30105_REVISIONID =    0xFE;
static const uint8_t MAX30105_PARTID =        0xFF;

// 常量定义
static const uint8_t MAX30105_INT_A_FULL_MASK =     (uint8_t)~0b10000000;
static const uint8_t MAX30105_INT_A_FULL_ENABLE =   0x80;
static const uint8_t MAX30105_INT_A_FULL_DISABLE =  0x00;
static const uint8_t MAX30105_INT_DATA_RDY_MASK =   (uint8_t)~0b01000000;
static const uint8_t MAX30105_INT_DATA_RDY_ENABLE = 0x40;
static const uint8_t MAX30105_INT_DATA_RDY_DISABLE=0x00;
static const uint8_t MAX30105_INT_ALC_OVF_MASK =    (uint8_t)~0b00100000;
static const uint8_t MAX30105_INT_ALC_OVF_ENABLE =  0x20;
static const uint8_t MAX30105_INT_ALC_OVF_DISABLE = 0x00;
static const uint8_t MAX30105_INT_PROX_INT_MASK =   (uint8_t)~0b00010000;
static const uint8_t MAX30105_INT_PROX_INT_ENABLE = 0x10;
static const uint8_t MAX30105_INT_PROX_INT_DISABLE=0x00;
static const uint8_t MAX30105_INT_DIE_TEMP_RDY_MASK=(uint8_t)~0b00000010;
static const uint8_t MAX30105_INT_DIE_TEMP_RDY_ENABLE = 0x02;
static const uint8_t MAX30105_INT_DIE_TEMP_RDY_DISABLE=0x00;

static const uint8_t MAX30105_SAMPLEAVG_MASK = (uint8_t)~0b11100000;
static const uint8_t MAX30105_SAMPLEAVG_1 = 0x00;
static const uint8_t MAX30105_SAMPLEAVG_2 = 0x20;
static const uint8_t MAX30105_SAMPLEAVG_4 = 0x40;
static const uint8_t MAX30105_SAMPLEAVG_8 = 0x60;
static const uint8_t MAX30105_SAMPLEAVG_16 = 0x80;
static const uint8_t MAX30105_SAMPLEAVG_32 = 0xA0;
static const uint8_t MAX30105_ROLLOVER_MASK = 0xEF;
static const uint8_t MAX30105_ROLLOVER_ENABLE = 0x10;
static const uint8_t MAX30105_ROLLOVER_DISABLE = 0x00;
static const uint8_t MAX30105_A_FULL_MASK = 0xF0;

static const uint8_t MAX30105_SHUTDOWN_MASK = 0x7F;
static const uint8_t MAX30105_SHUTDOWN = 0x80;
static const uint8_t MAX30105_WAKEUP = 0x00;
static const uint8_t MAX30105_RESET_MASK = 0xBF;
static const uint8_t MAX30105_RESET = 0x40;
static const uint8_t MAX30105_MODE_MASK = 0xF8;
static const uint8_t MAX30105_MODE_REDONLY = 0x02;
static const uint8_t MAX30105_MODE_REDIRONLY = 0x03;
static const uint8_t MAX30105_MODE_MULTILED = 0x07;

static const uint8_t MAX30105_ADCRANGE_MASK = 0x9F;
static const uint8_t MAX30105_ADCRANGE_2048 = 0x00;
static const uint8_t MAX30105_ADCRANGE_4096 = 0x20;
static const uint8_t MAX30105_ADCRANGE_8192 = 0x40;
static const uint8_t MAX30105_ADCRANGE_16384 = 0x60;
static const uint8_t MAX30105_SAMPLERATE_MASK = 0xE3;
static const uint8_t MAX30105_SAMPLERATE_50 = 0x00;
static const uint8_t MAX30105_SAMPLERATE_100 = 0x04;
static const uint8_t MAX30105_SAMPLERATE_200 = 0x08;
static const uint8_t MAX30105_SAMPLERATE_400 = 0x0C;
static const uint8_t MAX30105_SAMPLERATE_800 = 0x10;
static const uint8_t MAX30105_SAMPLERATE_1000 = 0x14;
static const uint8_t MAX30105_SAMPLERATE_1600 = 0x18;
static const uint8_t MAX30105_SAMPLERATE_3200 = 0x1C;
static const uint8_t MAX30105_PULSEWIDTH_MASK = 0xFC;
static const uint8_t MAX30105_PULSEWIDTH_69 = 0x00;
static const uint8_t MAX30105_PULSEWIDTH_118 = 0x01;
static const uint8_t MAX30105_PULSEWIDTH_215 = 0x02;
static const uint8_t MAX30105_PULSEWIDTH_411 = 0x03;

static const uint8_t MAX30105_SLOT1_MASK = 0xF8;
static const uint8_t MAX30105_SLOT2_MASK = 0x8F;
static const uint8_t MAX30105_SLOT3_MASK = 0xF8;
static const uint8_t MAX30105_SLOT4_MASK = 0x8F;
/* 槽位常量已移至 MAX30105.h */

static const uint8_t MAX_30105_EXPECTEDPARTID = 0x15;
static const int STORAGE_SIZE = 4;

struct Record {
    uint32_t red[STORAGE_SIZE];
    uint32_t IR[STORAGE_SIZE];
    uint32_t green[STORAGE_SIZE];
    uint8_t head;
    uint8_t tail;
} sense;

// 构造函数
MAX30105::MAX30105() {
    // 初始化成员变量
    _i2caddr = MAX30105_ADDRESS;
    activeLEDs = 2;  // 默认 Red+IR
    revisionID = 0;
    sense.head = 0;
    sense.tail = 0;
}

// 初始化
bool MAX30105::begin(uint8_t i2caddr) {
    _i2caddr = i2caddr;
    if (readPartID() != MAX_30105_EXPECTEDPARTID) {
        return false;
    }
    readRevisionID();
    return true;
}

// 底层 I2C 读写（HAL 实现）
uint8_t MAX30105::readRegister8(uint8_t address, uint8_t reg) {
    uint8_t data = 0;
    HAL_I2C_Mem_Read(&hi2c1, address << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
    return data;
}

void MAX30105::writeRegister8(uint8_t address, uint8_t reg, uint8_t value) {
    HAL_I2C_Mem_Write(&hi2c1, address << 1, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, HAL_MAX_DELAY);
}

// 中断状态读取
uint8_t MAX30105::getINT1(void) {
    return readRegister8(_i2caddr, MAX30105_INTSTAT1);
}
uint8_t MAX30105::getINT2(void) {
    return readRegister8(_i2caddr, MAX30105_INTSTAT2);
}

void MAX30105::enableAFULL(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_A_FULL_MASK, MAX30105_INT_A_FULL_ENABLE);
}
void MAX30105::disableAFULL(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_A_FULL_MASK, MAX30105_INT_A_FULL_DISABLE);
}
void MAX30105::enableDATARDY(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_DATA_RDY_MASK, MAX30105_INT_DATA_RDY_ENABLE);
}
void MAX30105::disableDATARDY(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_DATA_RDY_MASK, MAX30105_INT_DATA_RDY_DISABLE);
}
void MAX30105::enableALCOVF(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_ALC_OVF_MASK, MAX30105_INT_ALC_OVF_ENABLE);
}
void MAX30105::disableALCOVF(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_ALC_OVF_MASK, MAX30105_INT_ALC_OVF_DISABLE);
}
void MAX30105::enablePROXINT(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_PROX_INT_MASK, MAX30105_INT_PROX_INT_ENABLE);
}
void MAX30105::disablePROXINT(void) {
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_PROX_INT_MASK, MAX30105_INT_PROX_INT_DISABLE);
}
void MAX30105::enableDIETEMPRDY(void) {
    bitMask(MAX30105_INTENABLE2, MAX30105_INT_DIE_TEMP_RDY_MASK, MAX30105_INT_DIE_TEMP_RDY_ENABLE);
}
void MAX30105::disableDIETEMPRDY(void) {
    bitMask(MAX30105_INTENABLE2, MAX30105_INT_DIE_TEMP_RDY_MASK, MAX30105_INT_DIE_TEMP_RDY_DISABLE);
}

// 软复位
void MAX30105::softReset(void) {
    bitMask(MAX30105_MODECONFIG, MAX30105_RESET_MASK, MAX30105_RESET);
    uint32_t startTime = _millis();
    while (_millis() - startTime < 100) {
        uint8_t response = readRegister8(_i2caddr, MAX30105_MODECONFIG);
        if ((response & MAX30105_RESET) == 0) break;
        _delay(1);
    }
}

void MAX30105::shutDown(void) {
    bitMask(MAX30105_MODECONFIG, MAX30105_SHUTDOWN_MASK, MAX30105_SHUTDOWN);
}
void MAX30105::wakeUp(void) {
    bitMask(MAX30105_MODECONFIG, MAX30105_SHUTDOWN_MASK, MAX30105_WAKEUP);
}
void MAX30105::setLEDMode(uint8_t mode) {
    bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, mode);
}
void MAX30105::setADCRange(uint8_t adcRange) {
    bitMask(MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_MASK, adcRange);
}
void MAX30105::setSampleRate(uint8_t sampleRate) {
    bitMask(MAX30105_PARTICLECONFIG, MAX30105_SAMPLERATE_MASK, sampleRate);
}
void MAX30105::setPulseWidth(uint8_t pulseWidth) {
    bitMask(MAX30105_PARTICLECONFIG, MAX30105_PULSEWIDTH_MASK, pulseWidth);
}
void MAX30105::setPulseAmplitudeRed(uint8_t amplitude) {
    writeRegister8(_i2caddr, MAX30105_LED1_PULSEAMP, amplitude);
}
void MAX30105::setPulseAmplitudeIR(uint8_t amplitude) {
    writeRegister8(_i2caddr, MAX30105_LED2_PULSEAMP, amplitude);
}
void MAX30105::setPulseAmplitudeGreen(uint8_t amplitude) {
    writeRegister8(_i2caddr, MAX30105_LED3_PULSEAMP, amplitude);
}
void MAX30105::setPulseAmplitudeProximity(uint8_t amplitude) {
    writeRegister8(_i2caddr, MAX30105_LED_PROX_AMP, amplitude);
}
void MAX30105::setProximityThreshold(uint8_t threshMSB) {
    writeRegister8(_i2caddr, MAX30105_PROXINTTHRESH, threshMSB);
}

void MAX30105::enableSlot(uint8_t slotNumber, uint8_t device) {
    switch (slotNumber) {
        case 1:
            bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, device);
            break;
        case 2:
            bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, device << 4);
            break;
        case 3:
            bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, device);
            break;
        case 4:
            bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT4_MASK, device << 4);
            break;
        default: break;
    }
}

void MAX30105::disableSlots(void) {
    writeRegister8(_i2caddr, MAX30105_MULTILEDCONFIG1, 0);
    writeRegister8(_i2caddr, MAX30105_MULTILEDCONFIG2, 0);
}

void MAX30105::setFIFOAverage(uint8_t numberOfSamples) {
    bitMask(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_MASK, numberOfSamples);
}

void MAX30105::clearFIFO(void) {
    writeRegister8(_i2caddr, MAX30105_FIFOWRITEPTR, 0);
    writeRegister8(_i2caddr, MAX30105_FIFOOVERFLOW, 0);
    writeRegister8(_i2caddr, MAX30105_FIFOREADPTR, 0);
}

void MAX30105::enableFIFORollover(void) {
    bitMask(MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK, MAX30105_ROLLOVER_ENABLE);
}
void MAX30105::disableFIFORollover(void) {
    bitMask(MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK, MAX30105_ROLLOVER_DISABLE);
}
void MAX30105::setFIFOAlmostFull(uint8_t numberOfSamples) {
    bitMask(MAX30105_FIFOCONFIG, MAX30105_A_FULL_MASK, numberOfSamples);
}
uint8_t MAX30105::getWritePointer(void) {
    return readRegister8(_i2caddr, MAX30105_FIFOWRITEPTR);
}
uint8_t MAX30105::getReadPointer(void) {
    return readRegister8(_i2caddr, MAX30105_FIFOREADPTR);
}

float MAX30105::readTemperature() {
    writeRegister8(_i2caddr, MAX30105_DIETEMPCONFIG, 0x01);
    uint32_t startTime = _millis();
    while (_millis() - startTime < 100) {
        uint8_t response = readRegister8(_i2caddr, MAX30105_DIETEMPCONFIG);
        if ((response & 0x01) == 0) break;
        _delay(1);
    }
    int8_t tempInt = (int8_t)readRegister8(_i2caddr, MAX30105_DIETEMPINT);
    uint8_t tempFrac = readRegister8(_i2caddr, MAX30105_DIETEMPFRAC);
    return (float)tempInt + ((float)tempFrac * 0.0625f);
}
float MAX30105::readTemperatureF() {
    float temp = readTemperature();
    if (temp != -999.0f) temp = temp * 1.8f + 32.0f;
    return temp;
}

void MAX30105::setPROXINTTHRESH(uint8_t val) {
    writeRegister8(_i2caddr, MAX30105_PROXINTTHRESH, val);
}

uint8_t MAX30105::readPartID() {
    return readRegister8(_i2caddr, MAX30105_PARTID);
}
void MAX30105::readRevisionID() {
    revisionID = readRegister8(_i2caddr, MAX30105_REVISIONID);
}
uint8_t MAX30105::getRevisionID() {
    return revisionID;
}

void MAX30105::setup(uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode,
                     int sampleRate, int pulseWidth, int adcRange) {
    softReset();

    // FIFO 平均配置
    if (sampleAverage == 1) setFIFOAverage(MAX30105_SAMPLEAVG_1);
    else if (sampleAverage == 2) setFIFOAverage(MAX30105_SAMPLEAVG_2);
    else if (sampleAverage == 4) setFIFOAverage(MAX30105_SAMPLEAVG_4);
    else if (sampleAverage == 8) setFIFOAverage(MAX30105_SAMPLEAVG_8);
    else if (sampleAverage == 16) setFIFOAverage(MAX30105_SAMPLEAVG_16);
    else if (sampleAverage == 32) setFIFOAverage(MAX30105_SAMPLEAVG_32);
    else setFIFOAverage(MAX30105_SAMPLEAVG_4);
    enableFIFORollover();

    // LED 模式
    if (ledMode == 3) setLEDMode(MAX30105_MODE_MULTILED);
    else if (ledMode == 2) setLEDMode(MAX30105_MODE_REDIRONLY);
    else setLEDMode(MAX30105_MODE_REDONLY);
    activeLEDs = ledMode;

    // ADC 范围
    if (adcRange < 4096) setADCRange(MAX30105_ADCRANGE_2048);
    else if (adcRange < 8192) setADCRange(MAX30105_ADCRANGE_4096);
    else if (adcRange < 16384) setADCRange(MAX30105_ADCRANGE_8192);
    else if (adcRange == 16384) setADCRange(MAX30105_ADCRANGE_16384);
    else setADCRange(MAX30105_ADCRANGE_2048);

    // 采样率
    if (sampleRate < 100) setSampleRate(MAX30105_SAMPLERATE_50);
    else if (sampleRate < 200) setSampleRate(MAX30105_SAMPLERATE_100);
    else if (sampleRate < 400) setSampleRate(MAX30105_SAMPLERATE_200);
    else if (sampleRate < 800) setSampleRate(MAX30105_SAMPLERATE_400);
    else if (sampleRate < 1000) setSampleRate(MAX30105_SAMPLERATE_800);
    else if (sampleRate < 1600) setSampleRate(MAX30105_SAMPLERATE_1000);
    else if (sampleRate < 3200) setSampleRate(MAX30105_SAMPLERATE_1600);
    else if (sampleRate == 3200) setSampleRate(MAX30105_SAMPLERATE_3200);
    else setSampleRate(MAX30105_SAMPLERATE_50);

    // 脉冲宽度
    if (pulseWidth < 118) setPulseWidth(MAX30105_PULSEWIDTH_69);
    else if (pulseWidth < 215) setPulseWidth(MAX30105_PULSEWIDTH_118);
    else if (pulseWidth < 411) setPulseWidth(MAX30105_PULSEWIDTH_215);
    else if (pulseWidth == 411) setPulseWidth(MAX30105_PULSEWIDTH_411);
    else setPulseWidth(MAX30105_PULSEWIDTH_69);

    // LED 电流
    setPulseAmplitudeRed(powerLevel);
    setPulseAmplitudeIR(powerLevel);
    setPulseAmplitudeGreen(powerLevel);
    setPulseAmplitudeProximity(powerLevel);

    // 多 LED 槽位
    enableSlot(1, SLOT_RED_LED);
    if (ledMode > 1) enableSlot(2, SLOT_IR_LED);
    if (ledMode > 2) enableSlot(3, SLOT_GREEN_LED);

    clearFIFO();
}

uint8_t MAX30105::available(void) {
    int8_t numberOfSamples = sense.head - sense.tail;
    if (numberOfSamples < 0) numberOfSamples += STORAGE_SIZE;
    return (uint8_t)numberOfSamples;
}

uint32_t MAX30105::getRed(void) {
    if (safeCheck(250)) return sense.red[sense.head];
    else return 0;
}
uint32_t MAX30105::getIR(void) {
    if (safeCheck(250)) return sense.IR[sense.head];
    else return 0;
}
uint32_t MAX30105::getGreen(void) {
    if (safeCheck(250)) return sense.green[sense.head];
    else return 0;
}
uint32_t MAX30105::getFIFORed(void) {
    return sense.red[sense.tail];
}
uint32_t MAX30105::getFIFOIR(void) {
    return sense.IR[sense.tail];
}
uint32_t MAX30105::getFIFOGreen(void) {
    return sense.green[sense.tail];
}
void MAX30105::nextSample(void) {
    if (available()) {
        sense.tail++;
        sense.tail %= STORAGE_SIZE;
    }
}

uint16_t MAX30105::check(void) {
    uint8_t readPtr = getReadPointer();
    uint8_t writePtr = getWritePointer();
    int16_t numSamples = writePtr - readPtr;
    if (numSamples < 0) numSamples += 32;
    if (numSamples == 0) return 0;

    uint16_t bytesToRead = numSamples * activeLEDs * 3;
    uint8_t fifoBuffer[bytesToRead];
    HAL_I2C_Mem_Read(&hi2c1, _i2caddr << 1, MAX30105_FIFODATA, I2C_MEMADD_SIZE_8BIT, fifoBuffer, bytesToRead, HAL_MAX_DELAY);

    uint8_t *ptr = fifoBuffer;
    for (int i = 0; i < numSamples; i++) {
        sense.head = (sense.head + 1) % STORAGE_SIZE;
        // 读取第一个 LED（RED）
        uint32_t val = ((uint32_t)ptr[0] << 16) | ((uint32_t)ptr[1] << 8) | ptr[2];
        sense.red[sense.head] = val & 0x3FFFF;
        ptr += 3;
        if (activeLEDs > 1) {
            val = ((uint32_t)ptr[0] << 16) | ((uint32_t)ptr[1] << 8) | ptr[2];
            sense.IR[sense.head] = val & 0x3FFFF;
            ptr += 3;
        }
        if (activeLEDs > 2) {
            val = ((uint32_t)ptr[0] << 16) | ((uint32_t)ptr[1] << 8) | ptr[2];
            sense.green[sense.head] = val & 0x3FFFF;
            ptr += 3;
        }
    }
    return (uint16_t)numSamples;
}

bool MAX30105::safeCheck(uint8_t maxTimeToCheck) {
    uint32_t markTime = _millis();
    while (1) {
        if (_millis() - markTime > maxTimeToCheck) return false;
        if (check() > 0) return true;
        _delay(1);
    }
}

void MAX30105::bitMask(uint8_t reg, uint8_t mask, uint8_t thing) {
    uint8_t original = readRegister8(_i2caddr, reg);
    original = original & mask;
    writeRegister8(_i2caddr, reg, original | thing);
}