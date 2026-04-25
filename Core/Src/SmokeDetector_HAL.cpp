/**
  ******************************************************************************
  * @file    SmokeDetector_HAL.cpp
  * @brief   基于 MAX30105 红外反射的烟雾检测器抽象层。
  *          实现基线校准、低通滤波、迟滞报警以及
  *          环境烟雾监测的 ppm 估算。
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#include "SmokeDetector_HAL.h"
#include <math.h>

SmokeDetectorHAL::SmokeDetectorHAL()
    : _baselineIR(0.0f),
      _filteredIR(0.0f),
      _scaleFactor(0.03f), /*!< 经验值：使用已知烟雾源进行调参 */
      _threshold(50.0f),
      _hysteresis(5.0f),
      _triggerFlag(false),
      _aboveThreshold(false)
{
}

bool SmokeDetectorHAL::begin(void)
{
    if (!_sensor.begin()) return false;

    /* 烟雾检测模式：仅使用红外 LED，400 Hz，215 us 脉冲，4096 量程。 */
    _sensor.setup(0x1F, 4, 2, 400, 215, 4096);
    _sensor.disableSlots();
    _sensor.enableSlot(1, SLOT_IR_LED);
    _sensor.clearFIFO();
    return true;
}

void SmokeDetectorHAL::calibrateBaseline(uint16_t samples)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < samples; i++) {
        sum += _readIR();
        _delay(5);
    }
    _baselineIR = sum / samples;
    _filteredIR = _baselineIR;
}

float SmokeDetectorHAL::_readIR(void)
{
    if (_sensor.safeCheck(10)) {
        return (float)_sensor.getIR();
    }
    return _filteredIR; /*!< 超时：返回上次滤波值。 */
}

float SmokeDetectorHAL::_lowPassFilter(float newValue)
{
    const float alpha = 0.2f;
    _filteredIR = alpha * newValue + (1.0f - alpha) * _filteredIR;
    return _filteredIR;
}

float SmokeDetectorHAL::getSmokeConcentration(void)
{
    float raw = _readIR();
    float filtered = _lowPassFilter(raw);
    float diff = filtered - _baselineIR;
    if (diff < 0.0f) diff = 0.0f;
    float ppm = diff * _scaleFactor;

    /* 迟滞比较器，用于报警边沿检测。 */
    if (!_aboveThreshold && ppm > _threshold) {
        _aboveThreshold = true;
        _triggerFlag = true;
    } else if (_aboveThreshold && ppm <= (_threshold - _hysteresis)) {
        _aboveThreshold = false;
    }
    return ppm;
}

void SmokeDetectorHAL::setThreshold(float ppm_threshold, float hysteresis)
{
    _threshold  = ppm_threshold;
    _hysteresis = hysteresis;
}

bool SmokeDetectorHAL::isInterrupted(void)
{
    bool ret = _triggerFlag;
    _triggerFlag = false; /*!< 边沿触发：读取后清除。 */
    return ret;
}

void SmokeDetectorHAL::clearInterrupt(void)
{
    _triggerFlag = false;
}
