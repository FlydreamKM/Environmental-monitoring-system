/**
  * @file    mp4.c
  * @brief   MP-4 传感器 HAL 驱动实现 (3.3V 加热降级版)
  * @note    核心公式:
  *          1) Vrl = ADC/4095 * 3.3f / 分压比
  *          2) Rs  = RL * (Vc - Vrl) / Vrl
  *          3) Ratio = Rs / R0
  *          4) 3.3V 补偿: Ratio_comp = Ratio / MP4_HEAT_COMPENSATION
  *          5) ppm = pow( (Ratio_comp / a), (1/b) )
  */

#include "mp4.h"
#include <math.h>

/* 外部声明 ADC 句柄 (由 CubeMX 生成于 main.c) */
extern ADC_HandleTypeDef MP4_ADC_HANDLE;

/* 内部数据实例 */
static MP4_Data_t g_mp4 = {0};

/* ===================== 私有函数 ===================== */

/**
  * @brief  单次 ADC 采样，返回电压 (V)
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
  * @brief  多次采样去极值平均，抑制噪声
  * @param  times: 采样次数
  * @retval 平均电压 (V)
  */
static float MP4_ReadVoltageAvg(uint8_t times)
{
    float sum = 0.0f;
    float max = 0.0f;
    float min = 999.0f;
    uint8_t valid = 0;

    for (uint8_t i = 0; i < times; i++) {
        float v = MP4_ReadVoltageOnce();
        
        /* 过滤明显异常值 (ADC 开路/短路) */
        if (v > 0.01f && v < 3.29f) {
            sum += v;
            valid++;
            if (v > max) max = v;
            if (v < min) min = v;
        }
        HAL_Delay(5);  /* 间隔 5ms，避免采样过于密集 */
    }

    if (valid >= 3) {
        /* 去掉一个最大和一个最小，剩余平均 */
        sum = sum - max - min;
        return sum / (valid - 2);
    } else if (valid > 0) {
        return sum / valid;
    } else {
        return 0.0f;
    }
}

/* ===================== 公共函数 ===================== */

void MP4_Init(void)
{
    g_mp4.Vrl = 0.0f;
    g_mp4.Rs  = 0.0f;
    g_mp4.R0  = 0.0f;
    g_mp4.Ratio = 0.0f;
    g_mp4.Ppm = 0.0f;
    g_mp4.IsCalibrated = 0;
    g_mp4.PowerOnTime = 0;
    g_mp4.IsWarmedUp = 0;
}

MP4_Status_t MP4_CalibrateR0(void)
{
    /* 3.3V 下预热时间更长，校准前建议已预热 30 分钟以上 */
    float vrl = MP4_ReadVoltageAvg(MP4_SAMPLE_TIMES);

    if (vrl < 0.05f || vrl >= MP4_VC_VOLTAGE * 0.99f) {
        return MP4_STATUS_ERROR;
    }

    /* 计算 3.3V 下的洁净空气电阻 R0 */
    g_mp4.Vrl = vrl;
    g_mp4.R0 = MP4_RL_RESISTANCE * (MP4_VC_VOLTAGE - vrl) / vrl;
    g_mp4.IsCalibrated = 1;

    return MP4_STATUS_OK;
}

MP4_Status_t MP4_Read(void)
{
    /* 累加上电时间 (假设以 1Hz 调用) */
    g_mp4.PowerOnTime++;

    if (g_mp4.PowerOnTime >= MP4_WARMUP_SECONDS) {
        g_mp4.IsWarmedUp = 1;
    }

    /* 读取 VRL (50 次采样去极值平均) */
    float vrl = MP4_ReadVoltageAvg(MP4_SAMPLE_TIMES);
    if (vrl < 0.05f) {
        return MP4_STATUS_ERROR;
    }

    /* 限幅，防止除零 */
    if (vrl >= MP4_VC_VOLTAGE * 0.99f) {
        vrl = MP4_VC_VOLTAGE * 0.99f;
    }

    /* 计算传感器电阻 Rs */
    g_mp4.Vrl = vrl;
    g_mp4.Rs = MP4_RL_RESISTANCE * (MP4_VC_VOLTAGE - vrl) / vrl;

    /* 未校准则无法计算浓度 */
    if (!g_mp4.IsCalibrated || g_mp4.R0 < 1.0f) {
        return MP4_STATUS_NOT_CALIBRATED;
    }

    /* 计算比值 RS/R0 */
    g_mp4.Ratio = g_mp4.Rs / g_mp4.R0;

    /* 3.3V 加热补偿: 灵敏度衰减，等效 Ratio 需放大 */
    float ratio_comp = g_mp4.Ratio / MP4_HEAT_COMPENSATION;

    /* 反推浓度 (对数线性模型) */
    if (ratio_comp > 0.0f) {
        float exponent = 1.0f / MP4_CURVE_B;
        g_mp4.Ppm = powf((ratio_comp / MP4_CURVE_A), exponent);
    } else {
        g_mp4.Ppm = 0.0f;
    }

    /* 下限处理: 原 300ppm 下限基于 5V 标定手册，3.3V 加热灵敏度显著下降，
     * 大量有效响应会被强制归零，故取消硬下限，保留原始计算值以便观察。 */
#if 0
    if (g_mp4.Ppm < 300.0f) {
        g_mp4.Ppm = 0.0f;
    }
#endif

    /* 返回状态 */
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
    g_mp4.IsWarmedUp = (seconds >= MP4_WARMUP_SECONDS) ? 1 : 0;
}