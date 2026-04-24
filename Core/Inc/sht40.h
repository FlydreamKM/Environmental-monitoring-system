
/********************************Copyright (c)**********************************\
**
**                   (c) Copyright 2023, Main, China, 被钢琴支配的悲惨大学生.
**                           All Rights Reserved
**
**                           By(被钢琴支配的悲惨大学生 personally owned)
**                           https://blog.csdn.net/m0_71226271?type=blog
**
**----------------------------------文件信息------------------------------------
** 文件名称: SHT40.h
** 创建人员: 被钢琴支配的悲惨大学生
** 创建日期: 2023-09-04
** 文档描述:基于STM32H7B0VBT6的HAL库SHT40驱动源码，使用硬件I2C
\********************************End of Head************************************/
#include "main.h"
extern I2C_HandleTypeDef hi2c2;
/**************************I2C地址****************************/
#define SHT30_Write (0x44<<1)   //写入地址
#define SHT30_Read  ((0x44<<1)+1)   //读出地址
/**************************SHT40命令****************************/
#define SHT40_MEASURE_TEMPERATURE_HUMIDITY 0xFD  //高精度读取温湿度命令
#define SHT40_READ_SERIAL_NUMBER 0x89                         //读取唯一序列号命令
#define SHT40_HEATER_200mW_1s 0x39                               //200mW加热1秒命令
/**************************API****************************/
void SHT40_Read_Temperature_Humidity(double *Temperature,double *Humidity);
uint32_t SHT40_Read_Serial_Number(void);
void SHT40_Heater_200mW_1s(void);
/**************************非阻塞API****************************/
typedef enum {
    SHT40_NB_IDLE = 0,
    SHT40_NB_WAITING
} SHT40_NB_State;

SHT40_NB_State SHT40_NB_GetState(void);
void SHT40_NB_Start(void);
uint8_t SHT40_NB_Poll(double *Temperature, double *Humidity);
