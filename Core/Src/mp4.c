/**
  ******************************************************************************
  * @file    mp4.c
  * @brief   MP-4 可燃气体传感器驱动实现（3.3 V 加热）。
  * @note    核心公式链：
  *          1) Vrl = ADC/4095 * 3.3f / 分压比
  *          2) Rs  = RL * (Vc - Vrl) / Vrl
  *          3) Ratio = Rs / R0
  *          4) 3.3 V 补偿: Ratio_comp = Ratio / 加热补偿系数
  *          5) ppm = pow((Ratio_comp / a), (1/b))
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#include "mp4.h"
#include <math.h>

/* 在 main.c 中声明的外部 ADC 句柄 */
extern ADC_HandleTypeDef MP4_ADC_HANDLE;

/* 内部数据实例 */
static MP4_Data_t g_mp4 = {0};

/* ===================== 私有函数 ===================== */

/**
  * @brief  执行单次 ADC 转换并返回 VRL 电压（单位：伏特）。
  * @retval 负载电阻电压（V），超时返回 0.0。
  */
static float MP4_ReadVoltageOnce(void)
{
    uint32_t adc_raw = 0;
    float voltage = 0.0f;

    HAL_ADC_Start(&MP4_ADC_HANDLE);

    if (HAL_ADC_PollForConversion(&MP4_ADC_HANDLE, 100) == HAL_OK) {
        adc_raw = HAL_ADC_GetValue(&MP4_ADC_HANDLE);
        voltage = ((float)adc_raw / 4095.0f) * 3.3f / MP4_VOLTAGE_DIV_RATIO;
    }

    HAL_ADC_Stop(&MP4_ADC_HANDLE);
    return voltage;
}

/**
  * @brief  对多个 ADC 采样取平均，并剔除异常值。
  * @param  times  要获取的采样次数。
  * @retval 平均后的 VRL 电压（V）。
  */
static float MP4_ReadVoltageAvg(uint8_t times)
{
    float sum = 0.0f;
    float max = 0.0f;
    float min = 999.0f;
    uint8_t valid = 0;

    for (uint8_t i = 0; i < times; i++) {
        float v = MP4_ReadVoltageOnce();

        /* 剔除明显异常值（开路/短路）。 */
        if (v > 0.01f && v < 3.29f) {
            sum += v;
            valid++;
            if (v > max) max = v;
            if (v < min) min = v;
        }
        HAL_Delay(5); /*!< 5 ms 间隔以避免 ADC 总线争用。 */
    }

    if (valid >= 3) {
        /* 去掉一个最大值和一个最小值，然后对剩余值取平均。 */
        sum = sum - max - min;
        return sum / (valid - 2);
    } else if (valid > 0) {
        return sum / valid;
    } else {
        return 0.0f;
    }
}

/* ===================== 公共 API 实现 ===================== */

void MP4_Init(void)
{
    g_mp4.Vrl          = 0.0f;
    g_mp4.Rs           = 0.0f;
    g_mp4.R0           = 0.0f;
    g_mp4.Ratio        = 0.0f;
    g_mp4.Ppm          = 0.0f;
    g_mp4.IsCalibrated = 0;
    g_mp4.PowerOnTime  = 0;
    g_mp4.IsWarmedUp   = 0;
}

MP4_Status_t MP4_CalibrateR0(void)
{
    float vrl = MP4_ReadVoltageAvg(MP4_SAMPLE_TIMES);

    if (vrl < 0.05f || vrl >= MP4_VC_VOLTAGE * 0.99f) {
        return MP4_STATUS_ERROR;
    }

    g_mp4.Vrl = vrl;
    g_mp4.R0  = MP4_RL_RESISTANCE * (MP4_VC_VOLTAGE - vrl) / vrl;
    g_mp4.IsCalibrated = 1;

    return MP4_STATUS_OK;
}

MP4_Status_t MP4_Read(void)
{
    /* 累计上电时间（假设调用频率约 1 Hz）。 */
    g_mp4.PowerOnTime++;

    if (g_mp4.PowerOnTime >= MP4_WARMUP_SECONDS) {
        g_mp4.IsWarmedUp = 1;
    }

    /* 使用剔除异常值的平均法获取 VRL。 */
    float vrl = MP4_ReadVoltageAvg(MP4_SAMPLE_TIMES);
    if (vrl < 0.05f) {
        return MP4_STATUS_ERROR;
    }

    /* 限制以防止除零。 */
    if (vrl >= MP4_VC_VOLTAGE * 0.99f) {
        vrl = MP4_VC_VOLTAGE * 0.99f;
    }

    /* 计算传感器电阻 Rs。 */
    g_mp4.Vrl = vrl;
    g_mp4.Rs  = MP4_RL_RESISTANCE * (MP4_VC_VOLTAGE - vrl) / vrl;

    /* 未校准无法计算 ppm。 */
    if (!g_mp4.IsCalibrated || g_mp4.R0 < 1.0f) {
        return MP4_STATUS_NOT_CALIBRATED;
    }

    /* 比值 Rs/R0 及 3.3 V 加热补偿。 */
    g_mp4.Ratio = g_mp4.Rs / g_mp4.R0;
    float ratio_comp = g_mp4.Ratio / MP4_HEAT_COMPENSATION;

    /* 逆对数线性模型：ppm = (Ratio_comp / a) ^ (1/b)。 */
    if (ratio_comp > 0.0f) {
        float exponent = 1.0f / MP4_CURVE_B;
        g_mp4.Ppm = powf((ratio_comp / MP4_CURVE_A), exponent);
    } else {
        g_mp4.Ppm = 0.0f;
    }

    if (!g_mp4.IsWarmedUp) {
        return MP4_STATUS_WARMING_UP;
    }

    return MP4_STATUS_OK;
}

MP4_Data_t* MP4_GetData(void)
{
    return &g_mp4;
}

uint8_t MP4_IsAlarm(void)
{
    if (g_mp4.IsCalibrated && g_mp4.IsWarmedUp) {
        return (g_mp4.Ppm >= MP4_ALARM_THRESHOLD) ? 1 : 0;
    }
    return 0;
}

void MP4_SetWarmUpTime(uint32_t seconds)
{
    g_mp4.PowerOnTime = seconds;
    g_mp4.IsWarmedUp  = (seconds >= MP4_WARMUP_SECONDS) ? 1 : 0;
}
