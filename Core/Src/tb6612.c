/**
  ******************************************************************************
  * @file    tb6612.c
  * @brief   TB6612FNG 双路直流电机驱动实现。
  *          TIM1（PA8）驱动电机 A；TIM2（PA15，部分重映射）驱动电机 B。
  *          方向引脚、待机控制和 PWM 完全自包含。
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#include "tb6612.h"

/* ===================== 私有变量 ===================== */

static TIM_HandleTypeDef htim1_tb; /*!< 电机 A PWM 的 TIM1 句柄 */
static TIM_HandleTypeDef htim2_tb; /*!< 电机 B PWM 的 TIM2 句柄 */

/* ===================== 私有定义 ===================== */

#define PWMA_CHANNEL   TIM_CHANNEL_1
#define PWMB_CHANNEL   TIM_CHANNEL_1

#define PWMA_GPIO_PORT GPIOA
#define PWMA_GPIO_PIN  GPIO_PIN_8  /*!< TIM1_CH1 */

#define PWMB_GPIO_PORT GPIOA
#define PWMB_GPIO_PIN  GPIO_PIN_15 /*!< TIM2_CH1（部分重映射） */

/* ===================== 私有函数原型 ===================== */

static void TB6612_TIM1_Init(void);
static void TB6612_TIM2_Init(void);
static void TB6612_GPIO_Init(void);

/* ===================== TIM 初始化 ===================== */

/**
  * @brief  在 PA8 上初始化 TIM1 PWM，用于电机 A 调速。
  * @note   预分频器 = 0，周期 = 65535 => 在 72 MHz TIM 时钟下约 1.1 kHz。
  */
static void TB6612_TIM1_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef      sConfigOC     = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim1_tb.Instance               = TIM1;
    htim1_tb.Init.Prescaler         = 0;
    htim1_tb.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1_tb.Init.Period            = 65535;
    htim1_tb.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1_tb.Init.RepetitionCounter = 0;
    htim1_tb.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim1_tb);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim1_tb, &sMasterConfig);

    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 0;
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1_tb, &sConfigOC, PWMA_CHANNEL);

    sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime         = 0;
    sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
    HAL_TIMEx_ConfigBreakDeadTime(&htim1_tb, &sBreakDeadTimeConfig);
}

/**
  * @brief  在 PA15（部分重映射）上初始化 TIM2 PWM，用于电机 B 调速。
  */
static void TB6612_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef      sConfigOC     = {0};

    htim2_tb.Instance               = TIM2;
    htim2_tb.Init.Prescaler         = 0;
    htim2_tb.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2_tb.Init.Period            = 65535;
    htim2_tb.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2_tb.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2_tb);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2_tb, &sMasterConfig);

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2_tb, &sConfigOC, PWMB_CHANNEL);
}

/* ===================== GPIO 初始化 ===================== */

/**
  * @brief  初始化 TB6612 所需的所有 GPIO。
  * @note   方向引脚（PA9/PA10/PA11/PA12）、STBY（PB15）、
  *         PWM 复用引脚（PA8/PA15）和 TIM 时钟。
  */
static void TB6612_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 方向引脚：PA9/PA10（AIN2/AIN1），PA11/PA12（BIN1/BIN2） */
    HAL_GPIO_WritePin(GPIOA,
                      ain1_Pin | ain2_Pin | bin1_Pin | bin2_Pin,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = ain1_Pin | ain2_Pin | bin1_Pin | bin2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* STBY – PB15 */
    HAL_GPIO_WritePin(stby_GPIO_Port, stby_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = stby_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(stby_GPIO_Port, &GPIO_InitStruct);

    /* PWMA – PA8 上的 TIM1_CH1（复用推挽） */
    GPIO_InitStruct.Pin   = PWMA_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWMA_GPIO_PORT, &GPIO_InitStruct);

    /* PWMB – PA15 上的 TIM2_CH1（复用推挽 + 部分重映射） */
    GPIO_InitStruct.Pin   = PWMB_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWMB_GPIO_PORT, &GPIO_InitStruct);
    __HAL_AFIO_REMAP_TIM2_PARTIAL_1();

    /* 显式使能 TIM 外设时钟。 */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
}

/* ===================== 导出函数 ===================== */

void TB6612_Init(void)
{
    TB6612_GPIO_Init();
    TB6612_TIM1_Init();
    TB6612_TIM2_Init();

    HAL_TIM_PWM_Start(&htim1_tb, PWMA_CHANNEL);
    HAL_TIM_PWM_Start(&htim2_tb, PWMB_CHANNEL);

    /* 释放待机，使电机可以驱动。 */
    TB6612_Standby(0);
}

void TB6612_Standby(uint8_t enable)
{
    HAL_GPIO_WritePin(stby_GPIO_Port, stby_Pin,
                      (enable) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void TB6612_SetMotorA(TB6612_Dir_t dir, uint16_t pwm)
{
    if (pwm > 65535U) {
        pwm = 65535U;
    }

    switch (dir) {
        case TB6612_CW:
            HAL_GPIO_WritePin(ain1_GPIO_Port, ain1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(ain2_GPIO_Port, ain2_Pin, GPIO_PIN_RESET);
            break;
        case TB6612_CCW:
            HAL_GPIO_WritePin(ain1_GPIO_Port, ain1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(ain2_GPIO_Port, ain2_Pin, GPIO_PIN_SET);
            break;
        case TB6612_STOP:
        default:
            HAL_GPIO_WritePin(ain1_GPIO_Port, ain1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(ain2_GPIO_Port, ain2_Pin, GPIO_PIN_RESET);
            pwm = 0;
            break;
    }
    __HAL_TIM_SET_COMPARE(&htim1_tb, PWMA_CHANNEL, pwm);
}

void TB6612_SetMotorB(TB6612_Dir_t dir, uint16_t pwm)
{
    if (pwm > 65535U) {
        pwm = 65535U;
    }

    switch (dir) {
        case TB6612_CW:
            HAL_GPIO_WritePin(bin1_GPIO_Port, bin1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(bin2_GPIO_Port, bin2_Pin, GPIO_PIN_RESET);
            break;
        case TB6612_CCW:
            HAL_GPIO_WritePin(bin1_GPIO_Port, bin1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(bin2_GPIO_Port, bin2_Pin, GPIO_PIN_SET);
            break;
        case TB6612_STOP:
        default:
            HAL_GPIO_WritePin(bin1_GPIO_Port, bin1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(bin2_GPIO_Port, bin2_Pin, GPIO_PIN_RESET);
            pwm = 0;
            break;
    }
    __HAL_TIM_SET_COMPARE(&htim2_tb, PWMB_CHANNEL, pwm);
}

void TB6612_SetMotorA_Speed(int16_t speed)
{
    if (speed > 0) {
        TB6612_SetMotorA(TB6612_CW, (uint16_t)speed);
    } else if (speed < 0) {
        TB6612_SetMotorA(TB6612_CCW, (uint16_t)(-speed));
    } else {
        TB6612_SetMotorA(TB6612_STOP, 0);
    }
}

void TB6612_SetMotorB_Speed(int16_t speed)
{
    if (speed > 0) {
        TB6612_SetMotorB(TB6612_CW, (uint16_t)speed);
    } else if (speed < 0) {
        TB6612_SetMotorB(TB6612_CCW, (uint16_t)(-speed));
    } else {
        TB6612_SetMotorB(TB6612_STOP, 0);
    }
}
