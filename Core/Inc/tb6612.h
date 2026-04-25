/**
  ******************************************************************************
  * @file    tb6612.h
  * @brief   TB6612FNG 双路直流电机驱动头文件。
  *          TIM 和 GPIO 初始化自包含，以便在 CubeMX 重新生成后仍然可用。
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#ifndef __TB6612_H
#define __TB6612_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "main.h"

/* ===================== 导出类型 ===================== */

/** @brief 电机方向枚举。 */
typedef enum {
    TB6612_STOP = 0, /*!< 短接制动到地 */
    TB6612_CW,       /*!< 顺时针 / 正转 */
    TB6612_CCW       /*!< 逆时针 / 反转 */
} TB6612_Dir_t;

/* ===================== API 原型 ===================== */

/**
  * @brief  初始化 TIM1/2 PWM 和方向 GPIO。
  * @note   在 HAL_Init() 和 SystemClock_Config() 之后调用一次。
  *         自动释放待机。
  */
void TB6612_Init(void);

/**
  * @brief  控制待机引脚。
  * @param  enable  1 = 进入待机（电机关闭），0 = 激活。
  */
void TB6612_Standby(uint8_t enable);

/**
  * @brief  设置电机 A 方向和 PWM 占空比。
  * @param  dir  TB6612_CW / TB6612_CCW / TB6612_STOP
  * @param  pwm  0 .. 65535（与 TIM 周期 = 65535 匹配）
  */
void TB6612_SetMotorA(TB6612_Dir_t dir, uint16_t pwm);

/**
  * @brief  设置电机 B 方向和 PWM 占空比。
  * @param  dir  TB6612_CW / TB6612_CCW / TB6612_STOP
  * @param  pwm  0 .. 65535
  */
void TB6612_SetMotorB(TB6612_Dir_t dir, uint16_t pwm);

/**
  * @brief  电机 A 有符号速度控制。
  * @param  speed  -65535 .. +65535。负值 = 逆时针，正值 = 顺时针，0 = 停止。
  */
void TB6612_SetMotorA_Speed(int16_t speed);

/**
  * @brief  电机 B 有符号速度控制。
  * @param  speed  -65535 .. +65535。
  */
void TB6612_SetMotorB_Speed(int16_t speed);

#ifdef __cplusplus
}
#endif

#endif /* __TB6612_H */
