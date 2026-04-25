#ifndef SMOKEDETECTOR_HAL_H
#define SMOKEDETECTOR_HAL_H

#include "MAX30105.h"   // 修改后的驱动类

class SmokeDetectorHAL {
public:
    SmokeDetectorHAL();
    bool begin(void);   // 初始化传感器
    void calibrateBaseline(uint16_t samples = 200);   // 基线校准
    float getSmokeConcentration(void);                // 获取当前 ppm
    void setThreshold(float ppm_threshold, float hysteresis = 5.0f);
    bool isInterrupted(void);                         // 检测是否触发阈值（沿触发）
    void clearInterrupt(void);

private:
    MAX30105 _sensor;
    float _baselineIR;
    float _filteredIR;
    float _scaleFactor;      // ADC差值转ppm系数
    float _threshold;
    float _hysteresis;
    bool _triggerFlag;
    bool _aboveThreshold;

    float _readIR(void);
    float _lowPassFilter(float newValue);
    uint32_t _millis(void) { return HAL_GetTick(); }
    void _delay(uint32_t ms) { HAL_Delay(ms); }
};

#endif