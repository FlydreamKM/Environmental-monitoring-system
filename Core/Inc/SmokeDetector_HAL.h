/**
  ******************************************************************************
  * @file    SmokeDetector_HAL.h
  * @brief   基于 MAX30105 红外反射的烟雾检测器抽象层。
  *          提供基线校准、低通滤波、迟滞报警以及 ppm 估算。
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#ifndef SMOKEDETECTOR_HAL_H
#define SMOKEDETECTOR_HAL_H

#include "MAX30105.h"

/**
  * @brief  基于 MAX30105 的烟雾检测器封装类。
  * @note   使用红外 LED 反射检测颗粒物（烟雾）。
  *         依赖读数前请在洁净空气中校准基线。
  */
class SmokeDetectorHAL {
public:
    SmokeDetectorHAL();

    /**
      * @brief  初始化 MAX30105 并配置为烟雾检测模式。
      * @note   仅启用红外 LED 时隙，400 Hz，215 us 脉冲，4096 ADC 量程。
      * @retval 若传感器初始化成功则为 true，出错则为 false。
      */
    bool begin(void);

    /**
      * @brief  在洁净空气中校准红外基线电平。
      * @param  samples  用于平均的红外采样数（默认 200）。
      */
    void calibrateBaseline(uint16_t samples = 200);

    /**
      * @brief  获取当前烟雾浓度估算值。
      * @note   应用低通滤波并根据基线偏差计算 ppm。
      * @retval 估算的烟雾浓度，单位为 ppm。
      */
    float getSmokeConcentration(void);

    /**
      * @brief  设置报警阈值与迟滞。
      * @param  ppm_threshold  触发报警的 ppm 电平
      * @param  hysteresis     防止抖动的 ppm 迟滞（默认 5.0）
      */
    void setThreshold(float ppm_threshold, float hysteresis = 5.0f);

    /**
      * @brief  检查是否发生阈值穿越（边沿触发）。
      * @note   读取后标志位被清除。
      * @retval 若自上次调用后触发报警边沿则为 true。
      */
    bool isInterrupted(void);

    /**
      * @brief  手动清除报警触发标志。
      */
    void clearInterrupt(void);

private:
    MAX30105 _sensor;
    float _baselineIR;   /*!< 校准后的洁净空气红外电平 */
    float _filteredIR;   /*!< 低通滤波后的红外读数 */
    float _scaleFactor;  /*!< ADC 差值到 ppm 的转换系数 */
    float _threshold;    /*!< 报警阈值，单位为 ppm */
    float _hysteresis;   /*!< 迟滞带，单位为 ppm */
    bool  _triggerFlag;  /*!< 边沿触发的报警标志 */
    bool  _aboveThreshold; /*!< 当前状态是否高于阈值 */

    float _readIR(void);
    float _lowPassFilter(float newValue);
    uint32_t _millis(void) { return HAL_GetTick(); }
    void _delay(uint32_t ms) { HAL_Delay(ms); }
};

#endif /* SMOKEDETECTOR_HAL_H */
