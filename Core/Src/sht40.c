/********************************Copyright (c)**********************************\
**
**                   (c) Copyright 2023, Main, China, 被钢琴支配的悲惨大学生.
**                           All Rights Reserved
**
**                           By(被钢琴支配的悲惨大学生 personally owned)
**                           https://blog.csdn.net/m0_71226271?type=blog
**
**----------------------------------文件信息------------------------------------
** 文件名称: SHT40.c
** 创建人员: 被钢琴支配的悲惨大学生
** 创建日期: 2023-09-04
** 文档描述:基于STM32H7B0VBT6的HAL库SHT40驱动源码，使用硬件I2C
\********************************End of Head************************************/
#include "main.h"
#include "sht40.h"
/*************************************************************************************************
*	函 数 名: SHT40_Read_Temperature_Humidity
*	入口参数: Temperature温度指针，Humidity湿度指针
*	返回值:无
*	函数功能: 以高精度读取温度和湿度
*	说    明:不对CRC校验码做验证
*************************************************************************************************/
void SHT40_Read_Temperature_Humidity(double *Temperature,double *Humidity)
{
	uint32_t Temperature_Byte;
	uint32_t Temperature_Checksum;
	uint32_t Humidity_Byte;
	uint32_t  Humidity_Checksum;
 
	uint8_t I2C_Transmit_Data[1];
	I2C_Transmit_Data[0]=SHT40_MEASURE_TEMPERATURE_HUMIDITY;
	uint8_t I2C_Receive_Data[6];
	HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, I2C_Transmit_Data,1,HAL_MAX_DELAY);
    HAL_Delay(10);
	HAL_I2C_Master_Receive(&hi2c2, SHT30_Read, I2C_Receive_Data,6,HAL_MAX_DELAY);
 
	Temperature_Byte = I2C_Receive_Data[0] << 8| I2C_Receive_Data[1];
	Temperature_Checksum= I2C_Receive_Data[2];
	Humidity_Byte = I2C_Receive_Data[3] << 8| I2C_Receive_Data[4];
	Humidity_Checksum = I2C_Receive_Data[5];
    *Temperature = -45 + 175 * Temperature_Byte/65535.0;
	*Humidity = -6 + 125 * Humidity_Byte/65535.0;
 
}
/*************************************************************************************************
*	函 数 名: SHT40_Read_Serial_Number
*	入口参数: 无
*	返回值:32bit的序列号
*	函数功能: 读取SHT40的出场唯一序列号
*	说    明:无
*************************************************************************************************/
uint32_t SHT40_Read_Serial_Number()
{
	uint32_t Serial_Number;
	uint8_t I2C_Transmit_Data[1];
	I2C_Transmit_Data[0]=SHT40_READ_SERIAL_NUMBER;
	uint8_t I2C_Receive_Data[6];
	HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, I2C_Transmit_Data,1,HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c2, SHT30_Read, I2C_Receive_Data,6,HAL_MAX_DELAY);
	Serial_Number=(I2C_Receive_Data[0] << 24)|
			(I2C_Receive_Data[1] << 16)|
			(I2C_Receive_Data[3] << 8)|
			(I2C_Receive_Data[4] << 0);
	return Serial_Number;
 
 
}
/*************************************************************************************************
*	函 数 名: SHT40_Heater_200mW_1s
*	入口参数: 无
*	返回值:无
*	函数功能: 开始内部加热器，以200mW加热1秒（一秒）
*	说    明:加热时间不能超过运行时间的10％，否则会过热。详情说明请参考数据手册12页
*************************************************************************************************/
void SHT40_Heater_200mW_1s()
{
	uint8_t I2C_Transmit_Data[1];
	I2C_Transmit_Data[0]=SHT40_HEATER_200mW_1s;
	HAL_I2C_Master_Transmit(&hi2c2, SHT30_Write, I2C_Transmit_Data,1,HAL_MAX_DELAY);
}

/*************************************************************************************************
*   非阻塞温湿度读取实现
*   使用方式：
*     1. 调用 SHT40_NB_Start() 发送测量命令（约占用 I2C 发送时间，<1ms）
*     2. 主循环中定期调用 SHT40_NB_Poll()，函数内部自动等待 >=10ms 后读取结果
*     3. 当 Poll 返回 1 时，表示一次测量完成，可获取最新温湿度
*************************************************************************************************/
static SHT40_NB_State sht40_nb_state = SHT40_NB_IDLE;
static uint32_t sht40_nb_tick = 0;
static uint8_t sht40_nb_buf[6];

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
        return 0;
    }

    if (HAL_I2C_Master_Receive(&hi2c2, SHT30_Read, sht40_nb_buf, 6, 100) == HAL_OK) {
        uint16_t t = (sht40_nb_buf[0] << 8) | sht40_nb_buf[1];
        uint16_t h = (sht40_nb_buf[3] << 8) | sht40_nb_buf[4];
        *Temperature = -45 + 175 * t / 65535.0;
        *Humidity = -6 + 125 * h / 65535.0;
    }

    sht40_nb_state = SHT40_NB_IDLE;
    return 1;
}
