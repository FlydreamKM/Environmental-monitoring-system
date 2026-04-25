/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @brief   健康监测系统主程序体。
  *          初始化所有外设、传感器和显示屏，进入裸机超级循环，
  *          轮询传感器、更新 GUI、控制报警蜂鸣器及排气电机。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sht40.h"
#include "gui.hpp"
#include "tb6612.h"
#include "mp4.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;      /*!< ADC1 句柄，用于 MP-4 气体传感器 (PA1) */

I2C_HandleTypeDef hi2c1;      /*!< I2C1 句柄，用于 MAX30105 烟雾传感器 */
I2C_HandleTypeDef hi2c2;      /*!< I2C2 句柄，用于 SHT40 和 VEML7700 */

RTC_HandleTypeDef hrtc;       /*!< RTC 句柄，由 LSE 32.768 kHz 提供时钟 */

SPI_HandleTypeDef hspi1;      /*!< SPI1 句柄，用于 ST7735 TFT 显示屏 */
DMA_HandleTypeDef hdma_spi1_rx; /*!< SPI1 RX DMA 句柄 */
DMA_HandleTypeDef hdma_spi1_tx; /*!< SPI1 TX DMA 句柄（ST7735 异步传输） */

TIM_HandleTypeDef htim1;      /*!< TIM1 句柄：电机 A 的 PWM 控制 (PA8) */
TIM_HandleTypeDef htim2;      /*!< TIM2 句柄：电机 B 的 PWM 控制 (PA15 重映射) */

/* USER CODE BEGIN PV */
static uint32_t last_sensor_tick = 0; /*!< 上次触发 SHT40 的时间戳 (ms) */
static uint32_t last_lux_tick    = 0; /*!< 上次读取 VEML7700 的时间戳 (ms) */
static double   sht40_temp       = 0.0; /*!< 最新温度值 (°C) */
static double   sht40_hum        = 0.0; /*!< 最新湿度值 (%RH) */
static uint8_t  gas_alarm        = 0;   /*!< 可燃气体报警标志 */
static uint8_t  smoke_alarm      = 0;   /*!< 烟雾报警标志 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ===================== Buzzer Alarm Control ===================== */

/**
  * @brief  非阻塞蜂鸣器状态机：三声短促蜂鸣。
  * @note   报警激活时：嘀-嘀-嘀（100 ms 响 / 50 ms 停，重复 3 次）
  *         -> 1200 ms 暂停 -> 循环。
  *         报警未激活时：立即关闭。
  * @param  alarm  1 = 触发报警，0 = 静音
  */
static void buzzer_update(uint8_t alarm)
{
    static uint32_t tick  = 0;
    static uint8_t  phase = 0; /*!< 0=空闲，1..5=通/断脉冲，6=长暂停 */
    uint32_t now = HAL_GetTick();

    if (!alarm) {
        if (phase != 0) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_RESET);
            phase = 0;
        }
        return;
    }

    if (phase == 0) {
        HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_SET);
        tick = now;
        phase = 1;
    } else if (phase == 1) { /* 第1次响：100 ms */
        if (now - tick >= 100) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_RESET);
            tick = now;
            phase = 2;
        }
    } else if (phase == 2) { /* 第1次停：50 ms */
        if (now - tick >= 50) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_SET);
            tick = now;
            phase = 3;
        }
    } else if (phase == 3) { /* 第2次响：100 ms */
        if (now - tick >= 100) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_RESET);
            tick = now;
            phase = 4;
        }
    } else if (phase == 4) { /* 第2次停：50 ms */
        if (now - tick >= 50) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_SET);
            tick = now;
            phase = 5;
        }
    } else if (phase == 5) { /* 第3次响：100 ms */
        if (now - tick >= 100) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_RESET);
            tick = now;
            phase = 6;
        }
    } else if (phase == 6) { /* 暂停：1200 ms */
        if (now - tick >= 1200) {
            HAL_GPIO_WritePin(beep_GPIO_Port, beep_Pin, GPIO_PIN_SET);
            tick = now;
            phase = 1; /* 重新开始三声蜂鸣 */
        }
    }
}

/* ===================== Motor Speed Mapping ===================== */

/** @brief 电机启动的 PPM 阈值（滞回开启）。 */
#define MOTOR_START_PPM   1000.0f
/** @brief 电机停止的 PPM 阈值（滞回关闭）。 */
#define MOTOR_STOP_PPM    900.0f
/** @brief 对应满占空比 PWM (65535) 的 PPM 阈值。 */
#define MOTOR_FULL_PPM    5000.0f

/**
  * @brief  将气体/烟雾浓度 (ppm) 映射为 PWM 占空比 (0..65535)。
  * @param  ppm  浓度，单位为百万分之一 (ppm)。
  * @retval uint16_t TIM 的 PWM 比较值。
  */
static uint16_t ppm_to_pwm(float ppm)
{
    if (ppm <= MOTOR_START_PPM) return 0;
    float ratio = (ppm - MOTOR_START_PPM) / (MOTOR_FULL_PPM - MOTOR_START_PPM);
    if (ratio > 1.0f) ratio = 1.0f;
    return (uint16_t)(ratio * 65535.0f);
}

/**
  * @brief  电机 A 更新：驱动烟雾排气风扇 (TIM1_CH1)。
  * @note   使用滞回特性，避免在阈值附近频繁启停。
  * @param  ppm  烟雾浓度 (ppm)。
  */
static void motorA_update(float ppm)
{
    static uint8_t running = 0;
    if (running) {
        if (ppm < MOTOR_STOP_PPM) {
            TB6612_SetMotorA(TB6612_STOP, 0);
            running = 0;
        } else {
            TB6612_SetMotorA(TB6612_CW, ppm_to_pwm(ppm));
        }
    } else {
        if (ppm > MOTOR_START_PPM) {
            running = 1;
            TB6612_SetMotorA(TB6612_CW, ppm_to_pwm(ppm));
        }
    }
}

/**
  * @brief  电机 B 更新：驱动可燃气体排气风扇 (TIM2_CH1)。
  * @note   使用滞回特性，避免在阈值附近频繁启停。
  * @param  ppm  气体浓度 (ppm)。
  */
static void motorB_update(float ppm)
{
    static uint8_t running = 0;
    if (running) {
        if (ppm < MOTOR_STOP_PPM) {
            TB6612_SetMotorB(TB6612_STOP, 0);
            running = 0;
        } else {
            TB6612_SetMotorB(TB6612_CW, ppm_to_pwm(ppm));
        }
    } else {
        if (ppm > MOTOR_START_PPM) {
            running = 1;
            TB6612_SetMotorB(TB6612_CW, ppm_to_pwm(ppm));
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  应用程序入口点。
  * @retval int（在嵌入式环境中永不返回）。
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration -----------------------------------------------------*/

    /* 复位所有外设，初始化 Flash 接口和 SysTick。 */
    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    /* 配置系统时钟为 72 MHz（HSE 8 MHz -> PLL x2 -> AHB/2）。 */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* 初始化所有已配置的外设。 */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();
    MX_SPI1_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_RTC_Init();

    /* USER CODE BEGIN 2 */

    /* 初始化电机驱动器（释放待机，PWM 为 0）。 */
    TB6612_Init();

    /* 初始化 TFT LCD 并绘制静态背景。 */
    ST7735_Init();
    gui_init();
    gui_draw_background();

    /* 初始化 MP-4 可燃气体传感器。
     * 跳过预热和校准：使用典型默认值 R0，使设备立即显示读数。
     * 如需精确测量，请在清洁空气中预热 30 分钟后执行 CalibrateR0()。 */
    MP4_Init();
    {
        MP4_Data_t *d = MP4_GetData();
        d->R0 = 40000.0f;      /*!< 3.3 V 加热电压下的典型默认 R0 值 */
        d->IsCalibrated = 1;
        d->IsWarmedUp   = 1;
    }

    /* USER CODE END 2 */

    /* Infinite loop ---------------------------------------------------------*/
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        uint32_t now = HAL_GetTick();

        /* ---------- SHT40 温度与湿度（非阻塞模式） ---------- */
        if ((now - last_sensor_tick >= 200) && (SHT40_NB_GetState() == SHT40_NB_IDLE)) {
            last_sensor_tick = now;
            SHT40_NB_Start();   /*!< 触发测量，立即返回 */
        }

        /* 在等待 >= 10 ms 的转换延时后，查询 SHT40 结果。 */
        if (SHT40_NB_Poll(&sht40_temp, &sht40_hum)) {
            gui_update_temp(sht40_temp);
            gui_update_hum(sht40_hum);
        }

        /* ---------- VEML7700 环境光（无等待模式） ---------- */
        if (now - last_lux_tick >= 100) {
            last_lux_tick = now;
            float lux = veml7700_read_lux();
            gui_update_lux(lux);
        }

        /* ---------- MAX30105 烟雾检测 ---------- */
        static uint32_t last_smoke_tick = 0;
        static float    smoke_ppm       = 0.0f;
        if (now - last_smoke_tick >= 200) {
            last_smoke_tick = now;
            smoke_ppm = smoke_detector_read();
            gui_update_smoke(smoke_ppm);
            if (smoke_ppm >= 0.0f) {
                smoke_alarm = (smoke_ppm >= 1000.0f) ? 1 : 0;
                motorA_update(smoke_ppm);
            } else {
                smoke_alarm = 0;
            }
        }

        /* ---------- MP-4 可燃气体 ---------- */
        static uint32_t last_gas_tick = 0;
        static float    gas_ppm       = 0.0f;
        if (now - last_gas_tick >= 500) {
            last_gas_tick = now;
            MP4_Status_t status = MP4_Read();
            if (status == MP4_STATUS_OK || status == MP4_STATUS_WARMING_UP) {
                MP4_Data_t *data = MP4_GetData();
                gas_ppm = data->Ppm;
                gui_update_gas((double)gas_ppm);
                gas_alarm = (gas_ppm >= MP4_ALARM_THRESHOLD) ? 1 : 0;
            } else {
                gui_update_gas(-1.0); /*!< 未校准或出错时显示 "--" */
                gas_ppm = 0.0f;
                gas_alarm = 0;
            }
            motorB_update(gas_ppm);
        }

        /* ---------- 蜂鸣器报警（烟雾与气体报警的逻辑或） ---------- */
        buzzer_update(gas_alarm || smoke_alarm);

        /* ---------- RTC 时间显示（每秒更新一次） ---------- */
        static uint32_t last_rtc_tick = 0;
        if (now - last_rtc_tick >= 1000) {
            last_rtc_tick = now;
            RTC_TimeTypeDef sTime;
            RTC_DateTypeDef sDate;
            HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            gui_update_time(sTime.Hours, sTime.Minutes, sTime.Seconds);
        }
        /* USER CODE END 3 */
    }
}

/**
  * @brief  系统时钟配置。
  * @note   HSE 8 MHz -> PLL x2 -> 系统时钟 72 MHz。
  *         AHB = 系统时钟/2 = 36 MHz。
  *         APB1 = AHB/2 = 18 MHz (最大值), APB2 = AHB/1 = 36 MHz。
  *         RTC 时钟 = LSE 32.768 kHz。
  *         ADC PCLK2 = /6。
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* 初始化 RCC 振荡器：HSE + LSE + PLL。 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.LSEState       = RCC_LSE_ON;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* 初始化 CPU、AHB 和 APB 总线时钟。 */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }

    /* 外设时钟源：RTC 来自 LSE，ADC 预分频。 */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_ADC;
    PeriphClkInit.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;
    PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  ADC1 初始化函数。
  * @note   配置为单次转换、软件触发、右对齐。
  *         通道 1 (PA1) 用于读取 MP-4 气体传感器的 VRL 电压。
  * @retval None
  */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* 通用配置。 */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 配置常规通道：PA1 (ADC1_IN1) 用于 MP-4 VRL。 */
    sConfig.Channel      = ADC_CHANNEL_1;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  I2C1 初始化函数。
  * @note   标准模式 100 kHz，7 位寻址。
  *         连接到 MAX30105 烟雾传感器。
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 100000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  I2C2 初始化函数。
  * @note   标准模式 100 kHz，7 位寻址。
  *         连接到 SHT40 (0x44) 和 VEML7700 (0x10)。
  * @retval None
  */
static void MX_I2C2_Init(void)
{
    hi2c2.Instance             = I2C2;
    hi2c2.Init.ClockSpeed      = 100000;
    hi2c2.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1     = 0;
    hi2c2.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2     = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  RTC 初始化函数。
  * @note   使用 LSE，自动配置 1 秒预分频器。
  *         备份寄存器 DR1 = 0xA5A5 标记已初始化状态，
  *         以便 VBAT 在复位后保持时间。
  * @retval None
  */
static void MX_RTC_Init(void)
{
    RTC_TimeTypeDef sTime         = {0};
    RTC_DateTypeDef DateToUpdate  = {0};

    /* 仅初始化 RTC。 */
    hrtc.Instance              = RTC;
    hrtc.Init.AsynchPrediv     = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut           = RTC_OUTPUTSOURCE_ALARM;
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }

    /* 检查备份寄存器：如果已初始化，则跳过时间设置。 */
    HAL_PWR_EnableBkUpAccess();
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == 0xA5A5) {
        HAL_PWR_DisableBkUpAccess();
        return;
    }

    /* 设置初始时间和日期（BCD 格式）。 */
    sTime.Hours   = 0x0;
    sTime.Minutes = 0x0;
    sTime.Seconds = 0x0;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }

    DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
    DateToUpdate.Month   = RTC_MONTH_JANUARY;
    DateToUpdate.Date    = 0x1;
    DateToUpdate.Year    = 0x0;
    if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }

    /* 标记备份寄存器，以便下次启动时跳过重新初始化。 */
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xA5A5);
    HAL_PWR_DisableBkUpAccess();
}

/**
  * @brief  SPI1 初始化函数。
  * @note   主模式，双线全双工，8 位数据，CPOL=LOW，CPHA=1EDGE。
  *         波特率 = PCLK2/2 = 18 MHz（ST7735 的最大速率）。
  *         软件 NSS 控制。
  * @retval None
  */
static void MX_SPI1_Init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  TIM1 初始化函数。
  * @note   通道 1 (PA8) 的 PWM 模式，用于电机 A 的速度控制。
  *         周期 = 65535 => 在 72 MHz TIM 时钟下约为 1.1 kHz。
  * @retval None
  */
static void MX_TIM1_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig       = {0};
    TIM_OC_InitTypeDef      sConfigOC           = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 0;
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 65535;
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 0;
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime         = 0;
    sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim1);
}

/**
  * @brief  TIM2 初始化函数。
  * @note   通道 1 (PA15，部分重映射) 的 PWM 模式，用于电机 B 的速度控制。
  *         周期 = 65535 => 在 72 MHz TIM 时钟下约为 1.1 kHz。
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef      sConfigOC     = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 0;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 65535;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief  DMA 控制器时钟使能及中断配置。
  * @note   DMA1 通道 2 (SPI1_RX) 和通道 3 (SPI1_TX) 中断使能，优先级为 0。
  * @retval None
  */
static void MX_DMA_Init(void)
{
    /* 使能 DMA1 时钟。 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA1_Channel2_IRQn：SPI1 接收。 */
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    /* DMA1_Channel3_IRQn：SPI1 发送。 */
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

/**
  * @brief  GPIO 初始化函数。
  * @note   配置项目中使用的所有 GPIO：
  *         - 输入：PA0 (MQ-x DO), PB6/PB7 (MAX3010x INT)
  *         - 输出：PA2/PA3/PA4/PA6 (ST7735 DC/RST/CS/BLK),
  *                 PA9/PA10/PA11/PA12 (TB6612 AIN/BIN),
  *                 PB1 (蜂鸣器), PB15 (TB6612 STBY)
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIO 端口时钟。 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA 控制引脚默认输出低电平。 */
    HAL_GPIO_WritePin(GPIOA,
                      dc_Pin | rst_Pin | cs_Pin | blk_Pin
                      | ain2_Pin | ain1_Pin | bin1_Pin | bin2_Pin,
                      GPIO_PIN_RESET);

    /* PB 控制引脚默认输出低电平。 */
    HAL_GPIO_WritePin(GPIOB, beep_Pin | stby_Pin, GPIO_PIN_RESET);

    /* PA0：MQ-x 数字输出（输入，无上拉/下拉）。 */
    GPIO_InitStruct.Pin  = do_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(do_GPIO_Port, &GPIO_InitStruct);

    /* PA2/3/4/6/9/10/11/12：ST7735 和电机方向输出。 */
    GPIO_InitStruct.Pin   = dc_Pin | rst_Pin | cs_Pin | blk_Pin
                          | ain2_Pin | ain1_Pin | bin1_Pin | bin2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB1/15：蜂鸣器和 TB6612 待机输出。 */
    GPIO_InitStruct.Pin   = beep_Pin | stby_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB6/7：MAX3010x 中断输入。 */
    GPIO_InitStruct.Pin  = max35105_int_Pin | max35102_int_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 打开 ST7735 背光（PA6 置高）。 */
    HAL_GPIO_WritePin(GPIOA, blk_Pin, GPIO_PIN_SET);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  HAL 初始化失败时执行的错误处理函数。
  * @note   禁止中断并无限循环，以防止进一步损坏。
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  报告 assert_param 失败的源文件和行号。
  * @param  file  指向源文件名的指针
  * @param  line  assert_param 错误的源行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* 用户可以在此处添加基于 printf 的报告，例如：
       printf("Wrong parameters value: file %s on line %d\r\n", file, line); */
}
#endif /* USE_FULL_ASSERT */
