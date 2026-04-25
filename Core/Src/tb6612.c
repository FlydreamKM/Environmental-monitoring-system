/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tb6612.c
  * @brief   TB6612FNG driver – self-contained TIM + GPIO init.
  *          Not affected by STM32CubeMX re-generation.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "tb6612.h"

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef htim1_tb;
static TIM_HandleTypeDef htim2_tb;

/* Private defines -----------------------------------------------------------*/
#define PWMA_CHANNEL    TIM_CHANNEL_1
#define PWMB_CHANNEL    TIM_CHANNEL_1

#define PWMA_GPIO_PORT  GPIOA
#define PWMA_GPIO_PIN   GPIO_PIN_8    /* TIM1_CH1 */

#define PWMB_GPIO_PORT  GPIOA
#define PWMB_GPIO_PIN   GPIO_PIN_15   /* TIM2_CH1 (partial remap) */

/* Private function prototypes -----------------------------------------------*/
static void TB6612_TIM1_Init(void);
static void TB6612_TIM2_Init(void);
static void TB6612_GPIO_Init(void);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  TIM1 PWM init (PA8 – PWMA).
  *         Prescaler=0, Period=65535 => ~1.1 kHz @ 72 MHz.
  */
static void TB6612_TIM1_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim1_tb.Instance = TIM1;
    htim1_tb.Init.Prescaler = 0;
    htim1_tb.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1_tb.Init.Period = 65535;
    htim1_tb.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1_tb.Init.RepetitionCounter = 0;
    htim1_tb.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim1_tb);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim1_tb, &sMasterConfig);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1_tb, &sConfigOC, PWMA_CHANNEL);

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    HAL_TIMEx_ConfigBreakDeadTime(&htim1_tb, &sBreakDeadTimeConfig);
}

/**
  * @brief  TIM2 PWM init (PA15 – PWMB, partial remap).
  */
static void TB6612_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2_tb.Instance = TIM2;
    htim2_tb.Init.Prescaler = 0;
    htim2_tb.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2_tb.Init.Period = 65535;
    htim2_tb.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2_tb.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2_tb);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2_tb, &sMasterConfig);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2_tb, &sConfigOC, PWMB_CHANNEL);
}

/**
  * @brief  GPIO init for direction pins, STBY and PWM AF pins.
  *         Clocks are enabled here so the driver is fully standalone.
  */
static void TB6612_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Direction pins: PA9/PA10 (AIN2/AIN1), PA11/PA12 (BIN1/BIN2) */
    HAL_GPIO_WritePin(GPIOA,
                      ain1_Pin | ain2_Pin | bin1_Pin | bin2_Pin,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = ain1_Pin | ain2_Pin | bin1_Pin | bin2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* STBY – PB15 */
    HAL_GPIO_WritePin(stby_GPIO_Port, stby_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = stby_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(stby_GPIO_Port, &GPIO_InitStruct);

    /* PWMA – TIM1_CH1 on PA8 (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin = PWMA_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWMA_GPIO_PORT, &GPIO_InitStruct);

    /* PWMB – TIM2_CH1 on PA15 (AF_PP + partial remap) */
    GPIO_InitStruct.Pin = PWMB_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWMB_GPIO_PORT, &GPIO_InitStruct);
    __HAL_AFIO_REMAP_TIM2_PARTIAL_1();

    /* TIM peripheral clocks – enable explicitly so we do not rely on CubeMX MSP */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
}

/* Exported functions --------------------------------------------------------*/

void TB6612_Init(void)
{
    TB6612_GPIO_Init();
    TB6612_TIM1_Init();
    TB6612_TIM2_Init();

    HAL_TIM_PWM_Start(&htim1_tb, PWMA_CHANNEL);
    HAL_TIM_PWM_Start(&htim2_tb, PWMB_CHANNEL);

    /* Leave standby so motors can be driven */
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
