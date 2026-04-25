/**
  ******************************************************************************
  * @file    st7735.h
  * @brief   ST7735 128x128 1.44 英寸 TFT LCD 驱动头文件。
  *          提供基于 SPI 的绘图原语、DMA 异步传输、
  *          颜色定义和显示方向宏。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Based on public-domain Adafruit ST7735 code, adapted for STM32 HAL.
  ******************************************************************************
  */

#ifndef __ST7735_H__
#define __ST7735_H__

#include "fonts.h"
#include <stdbool.h>

/* ===================== ST7735 命令常量 ===================== */

#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04

/* ===================== 硬件端口映射 ===================== */

#define ST7735_SPI_PORT hspi1          /*!< 用于 ST7735 的 HAL SPI 句柄 */
extern SPI_HandleTypeDef ST7735_SPI_PORT;

#define ST7735_RES_Pin       GPIO_PIN_3
#define ST7735_RES_GPIO_Port GPIOA
#define ST7735_CS_Pin        GPIO_PIN_4
#define ST7735_CS_GPIO_Port  GPIOA
#define ST7735_DC_Pin        GPIO_PIN_2
#define ST7735_DC_GPIO_Port  GPIOA

/* ===================== 显示几何参数（1.44 英寸 128x128） ===================== */

#define ST7735_IS_128X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128
#define ST7735_XSTART 2
#define ST7735_YSTART 3
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB)

/* ===================== ST7735 寄存器定义 ===================== */

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

/* ===================== 颜色定义（RGB565） ===================== */

#define ST7735_BLACK   0x0000
#define ST7735_BLUE    0x001F
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF
#define ST7735_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

/* ===================== 伽马曲线选择 ===================== */

typedef enum {
    GAMMA_10 = 0x01,
    GAMMA_25 = 0x02,
    GAMMA_22 = 0x04,
    GAMMA_18 = 0x08
} GammaDef;

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== API 函数原型 ===================== */

/**
  * @brief  释放片选（在初始化其他 SPI 设备前调用）。
  */
void ST7735_Unselect(void);

/**
  * @brief  初始化 ST7735：复位、发送初始化命令列表、退出睡眠模式。
  */
void ST7735_Init(void);

/**
  * @brief  绘制单个像素。
  * @param  x      水平坐标（0..ST7735_WIDTH-1）
  * @param  y      垂直坐标（0..ST7735_HEIGHT-1）
  * @param  color  RGB565 颜色值
  */
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/**
  * @brief  使用指定字体绘制以空字符结尾的 ASCII 字符串。
  * @param  x       起始 X
  * @param  y       起始 Y
  * @param  str     要绘制的字符串
  * @param  font    字体定义
  * @param  color   前景 RGB565 颜色
  * @param  bgcolor 背景 RGB565 颜色
  */
void ST7735_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font,
                        uint16_t color, uint16_t bgcolor);

/**
  * @brief  同步填充矩形（阻塞式 SPI）。
  */
void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
  * @brief  通过 DMA 异步填充矩形（非阻塞）。
  * @note   调用前检查 ST7735_IsBusy()；完成后回调释放 CS。
  */
void ST7735_FillRectangleAsync(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
  * @brief  FillRectangleAsync 的阻塞包装函数；等待 DMA 完成。
  */
void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
  * @brief  同步填充整个屏幕。
  */
void ST7735_FillScreen(uint16_t color);

/**
  * @brief  通过 DMA 异步填充整个屏幕。
  */
void ST7735_FillScreenAsync(uint16_t color);

/**
  * @brief  FillScreenAsync 的阻塞包装函数；等待 DMA 完成。
  */
void ST7735_FillScreenFast(uint16_t color);

/**
  * @brief  同步绘制图像（阻塞式）。
  * @param  data  指向 RGB565 像素数组的指针，按行主序排列
  */
void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

/**
  * @brief  通过 DMA 异步绘制图像（非阻塞）。
  */
void ST7735_DrawImageAsync(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

/**
  * @brief  将预格式化的 RGB565 缓冲区推送到屏幕区域（阻塞式）。
  * @note   由 GUI 后缓冲刷新使用。
  */
void ST7735_PushBuffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *buffer);

/**
  * @brief  查询异步 DMA 传输是否仍在进行中。
  * @retval 忙时返回 true，空闲时返回 false
  */
bool ST7735_IsBusy(void);

/**
  * @brief  全局反转显示颜色。
  * @param  invert  true = 反转，false = 正常
  */
void ST7735_InvertColors(bool invert);

/**
  * @brief  选择伽马曲线。
  * @param  gamma  GammaDef 值之一
  */
void ST7735_SetGamma(GammaDef gamma);

#ifdef __cplusplus
}
#endif

#endif /* __ST7735_H__ */
