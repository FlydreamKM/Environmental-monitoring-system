/**
  ******************************************************************************
  * @file    fonts.h
  * @brief   ASCII 字符的位图字体定义。
  *          提供三种尺寸：7x10、11x18 和 16x26 像素。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Based on public-domain font code, adapted for STM32 ST7735 driver.
  ******************************************************************************
  */

#ifndef __FONTS_H__
#define __FONTS_H__

#include <stdint.h>

/**
  * @brief  字体描述符结构体。
  * @note   每个字符位图存储为 uint16_t 行掩码数组。
  *         Font_7x10：10 行 16 位，最高位 = 最左侧像素。
  *         Font_11x18：18 行 16 位，最高位 = 最左侧像素。
  *         Font_16x26：26 行 16 位，最高位 = 最左侧像素。
  */
typedef struct {
    const uint8_t  width;   /*!< 字符宽度（像素） */
    uint8_t        height;  /*!< 字符高度（像素） */
    const uint16_t *data;   /*!< 指向打包位图数组的指针 */
} FontDef;

/* ===================== 导出的字体实例 ===================== */

extern FontDef Font_7x10;   /*!< 小字体：7 像素宽，10 像素高 */
extern FontDef Font_11x18;  /*!< 中字体：11 像素宽，18 像素高 */
extern FontDef Font_16x26;  /*!< 大字体：16 像素宽，26 像素高 */

#endif /* __FONTS_H__ */
