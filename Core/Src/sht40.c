/**
  ******************************************************************************
  * @file    sht40.c
  * @brief   SHT40 温湿度传感器驱动实现。
  *          支持阻塞式读取和非阻塞状态机读取。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright 基于公开 SHT40 示例，适配 STM32 HAL。
  ******************************************************************************
  */

#include "main.h"
#include "sht40.h"

extern I2C_HandleTypeDef hi2c2; /*!< I2C2 句柄，定义于 main.c */

/* ===================== 阻塞式 API 实现 ===================== */

void SHT40_Read_Temperature_Humidity(double *Temperature, double *Humidity)
{
    uint32_t Temperature_Byte;
    uint32_t Temperature_Checksum;
    uint32_t Humidity_Byte;
    uint32_t Humidity_Checksum;

    uint8_t I2C_Transmit_Data[1];
    I2C_Transmit_Data[0] = SHT40_MEASURE_TEMPERATURE_HUMIDITY;
    uint8_t I2C_Receive_Data[6];

    HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, I2C_Transmit_Data, 1, HAL_MAX_DELAY);
    HAL_Delay(10);
    HAL_I2C_Master_Receive(&hi2c2, SHT30_Read, I2C_Receive_Data, 6, HAL_MAX_DELAY);

    Temperature_Byte     = (I2C_Receive_Data[0] << 8) | I2C_Receive_Data[1];
    Temperature_Checksum = I2C_Receive_Data[2];
    Humidity_Byte        = (I2C_Receive_Data[3] << 8) | I2C_Receive_Data[4];
    Humidity_Checksum    = I2C_Receive_Data[5];

    *Temperature = -45.0 + 175.0 * Temperature_Byte / 65535.0;
    *Humidity    = -6.0  + 125.0 * Humidity_Byte    / 65535.0;
}

uint32_t SHT40_Read_Serial_Number(void)
{
    uint32_t Serial_Number;
    uint8_t I2C_Transmit_Data[1];
    I2C_Transmit_Data[0] = SHT40_READ_SERIAL_NUMBER;
    uint8_t I2C_Receive_Data[6];

    HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, I2C_Transmit_Data, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c2, SHT30_Read, I2C_Receive_Data, 6, HAL_MAX_DELAY);

    Serial_Number = (I2C_Receive_Data[0] << 24)
                  | (I2C_Receive_Data[1] << 16)
                  | (I2C_Receive_Data[3] << 8)
                  | (I2C_Receive_Data[4] << 0);
    return Serial_Number;
}

void SHT40_Heater_200mW_1s(void)
{
    uint8_t I2C_Transmit_Data[1];
    I2C_Transmit_Data[0] = SHT40_HEATER_200mW_1s;
    HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, I2C_Transmit_Data, 1, HAL_MAX_DELAY);
}

/* ===================== 非阻塞式 API 实现 ===================== */

static SHT40_NB_State sht40_nb_state = SHT40_NB_IDLE;
static uint32_t       sht40_nb_tick  = 0;
static uint8_t        sht40_nb_buf[6];

SHT40_NB_State SHT40_NB_GetState(void)
{
    return sht40_nb_state;
}

void SHT40_NB_Start(void)
{
    uint8_t cmd = SHT40_MEASURE_TEMPERATURE_HUMIDITY;
    HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, &cmd, 1, 100);
    sht40_nb_tick = HAL_GetTick();
    sht40_nb_state = SHT40_NB_WAITING;
}

uint8_t SHT40_NB_Poll(double *Temperature, double *Humidity)
{
    if (sht40_nb_state != SHT40_NB_WAITING) {
        return 0;
    }
    if (HAL_GetTick() - sht40_nb_tick < 10) {
        return 0; /*!< 转换尚未完成。 */
    }

    if (HAL_I2C_Master_Receive(&hi2c2, SHT30_Read, sht40_nb_buf, 6, 100) == HAL_OK) {
        uint16_t t = (sht40_nb_buf[0] << 8) | sht40_nb_buf[1];
        uint16_t h = (sht40_nb_buf[3] << 8) | sht40_nb_buf[4];
        *Temperature = -45.0 + 175.0 * t / 65535.0;
        *Humidity    = -6.0  + 125.0 * h / 65535.0;
    }

    sht40_nb_state = SHT40_NB_IDLE;
    return 1;
}
