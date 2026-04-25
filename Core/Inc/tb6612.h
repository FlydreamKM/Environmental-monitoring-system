/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tb6612.h
  * @brief   TB6612FNG dual DC motor driver (lightweight)
  *          TIM & GPIO init are self-contained to survive CubeMX re-gen.
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __TB6612_H
#define __TB6612_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    TB6612_STOP = 0,   /*!< Brake to GND (short brake) */
    TB6612_CW,         /*!< Clockwise / Forward */
    TB6612_CCW         /*!< Counter-Clockwise / Reverse */
} TB6612_Dir_t;

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialise TIM1/2 PWM and direction GPIO.
  *         Call once after HAL_Init() and SystemClock_Config().
  */
void TB6612_Init(void);

/**
  * @brief  Control STBY pin.
  * @param  enable 1 = standby (motor driver sleep), 0 = active
  */
void TB6612_Standby(uint8_t enable);

/**
  * @brief  Set motor A direction and PWM duty.
  * @param  dir   TB6612_CW / TB6612_CCW / TB6612_STOP
  * @param  pwm   0 .. 65535 (matches TIM Period = 65535)
  */
void TB6612_SetMotorA(TB6612_Dir_t dir, uint16_t pwm);

/**
  * @brief  Set motor B direction and PWM duty.
  * @param  dir   TB6612_CW / TB6612_CCW / TB6612_STOP
  * @param  pwm   0 .. 65535
  */
void TB6612_SetMotorB(TB6612_Dir_t dir, uint16_t pwm);

/**
  * @brief  Signed speed control for motor A.
  * @param  speed  -65535 .. +65535.  Negative = CCW, Positive = CW, 0 = STOP
  */
void TB6612_SetMotorA_Speed(int16_t speed);

/**
  * @brief  Signed speed control for motor B.
  * @param  speed  -65535 .. +65535
  */
void TB6612_SetMotorB_Speed(int16_t speed);

#ifdef __cplusplus
}
#endif

#endif /* __TB6612_H */
