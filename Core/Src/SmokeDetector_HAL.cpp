#include "SmokeDetector_HAL.h"
#include <math.h>

SmokeDetectorHAL::SmokeDetectorHAL() {
    _baselineIR = 0.0f;
    _filteredIR = 0.0f;
    _scaleFactor = 0.03f;   // 需实际标定，初始经验值
    _threshold = 50.0f;
    _hysteresis = 5.0f;
    _triggerFlag = false;
    _aboveThreshold = false;
}

bool SmokeDetectorHAL::begin(void) {
    if (!_sensor.begin()) return false;
    // 设置传感器为烟雾检测模式：只使用 IR LED，采样率 400Hz，脉冲宽度 215us
    _sensor.setup(0x1F, 4, 2, 400, 215, 4096);
    // 确保只使能 IR 槽位
    _sensor.disableSlots();
    _sensor.enableSlot(1, SLOT_IR_LED);
    _sensor.clearFIFO();
    return true;
}

void SmokeDetectorHAL::calibrateBaseline(uint16_t samples) {
    float sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        sum += _readIR();
        _delay(5);
    }
    _baselineIR = sum / samples;
    _filteredIR = _baselineIR;
}

float SmokeDetectorHAL::_readIR(void) {
    if (_sensor.safeCheck(10)) {
        return (float)_sensor.getIR();
    }
    return _filteredIR;   // 超时则返回上次值
}

float SmokeDetectorHAL::_lowPassFilter(float newValue) {
    const float alpha = 0.2f;
    _filteredIR = alpha * newValue + (1.0f - alpha) * _filteredIR;
    return _filteredIR;
}

float SmokeDetectorHAL::getSmokeConcentration(void) {
    float raw = _readIR();
    float filtered = _lowPassFilter(raw);
    float diff = filtered - _baselineIR;
    if (diff < 0) diff = 0;
    float ppm = diff * _scaleFactor;
    
    // 滞回比较，更新触发标志
    if (!_aboveThreshold && ppm > _threshold) {
        _aboveThreshold = true;
        _triggerFlag = true;
    } else if (_aboveThreshold && ppm <= (_threshold - _hysteresis)) {
        _aboveThreshold = false;
    }
    return ppm;
}

void SmokeDetectorHAL::setThreshold(float ppm_threshold, float hysteresis) {
    _threshold = ppm_threshold;
    _hysteresis = hysteresis;
}

bool SmokeDetectorHAL::isInterrupted(void) {
    bool ret = _triggerFlag;
    _triggerFlag = false;   // 沿触发，读取后清除
    return ret;
}

void SmokeDetectorHAL::clearInterrupt(void) {
    _triggerFlag = false;
}