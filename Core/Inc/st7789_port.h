#ifndef __ST7789_PORT_H
#define __ST7789_PORT_H

#include "main.h"

#ifndef ST7789_SPI_PORT
#define ST7789_SPI_PORT hspi1
#endif
extern SPI_HandleTypeDef ST7789_SPI_PORT;

#ifndef ST7789_USE_DMA
#define ST7789_USE_DMA 1
#endif

#ifndef ST7789_SPI_TIMEOUT
#define ST7789_SPI_TIMEOUT HAL_MAX_DELAY
#endif

#if !defined(CFG_NO_CS) && !defined(ST7789_USE_CS)
#define ST7789_USE_CS
#endif

#ifndef ST7789_RST_PORT
#define ST7789_RST_PORT GPIOA
#endif

#ifndef ST7789_RST_PIN
#define ST7789_RST_PIN GPIO_PIN_3
#endif

#ifndef ST7789_DC_PORT
#define ST7789_DC_PORT GPIOA
#endif

#ifndef ST7789_DC_PIN
#define ST7789_DC_PIN GPIO_PIN_2
#endif

#ifndef ST7789_BL_PORT
#define ST7789_BL_PORT GPIOA
#endif

#ifndef ST7789_BL_PIN
#define ST7789_BL_PIN GPIO_PIN_6
#endif

#ifndef CFG_NO_CS
#ifndef ST7789_CS_PORT
#define ST7789_CS_PORT GPIOA
#endif

#ifndef ST7789_CS_PIN
#define ST7789_CS_PIN GPIO_PIN_4
#endif
#endif

#define ST7789_BL_On()  HAL_GPIO_WritePin(ST7789_BL_PORT, ST7789_BL_PIN, GPIO_PIN_SET)
#define ST7789_BL_Off() HAL_GPIO_WritePin(ST7789_BL_PORT, ST7789_BL_PIN, GPIO_PIN_RESET)

#define ST7789_DELAY(ms) HAL_Delay(ms)

#define ST7789_SPI_WRITE(data, size) \
	HAL_SPI_Transmit(&ST7789_SPI_PORT, (uint8_t *)(data), (uint16_t)(size), ST7789_SPI_TIMEOUT)

#if ST7789_USE_DMA
#define ST7789_SPI_WRITE_DMA(data, size) \
	HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)(data), (uint16_t)(size))

#define ST7789_SPI_WAIT_DMA() \
	while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY) {}
#endif

#define ST7789_RST_Clr() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_Set() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)

#define ST7789_DC_Clr() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define ST7789_DC_Set() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)

#ifndef CFG_NO_CS
#define ST7789_Select() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
#define ST7789_UnSelect() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)
#else
#define ST7789_Select() asm("nop")
#define ST7789_UnSelect() asm("nop")
#endif

#endif