/**
  ******************************************************************************
  * @file    gui.cpp
  * @brief   健康监测显示器 GUI 实现。
  *          - 16x16 中文字模位图，用于传感器标签
  *          - 左侧卡片区域本地双缓冲 (100x24 RGB565)
  *          - ST7735 PushBuffer 刷新
  *          - VEML7700 与烟雾检测器初始化及封装函数
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#include "gui.hpp"
#include "main.h"
#include "GuiLite.h"
#include "st7735.h"
#include "fonts.h"
#include "VEML7700_HAL.h"
#include "SmokeDetector_HAL.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c2; /*!< 供 VEML7700 与 SHT40 使用的 I2C2 句柄 */
extern I2C_HandleTypeDef hi2c1; /*!< 供 MAX30105 使用的 I2C1 句柄 */

/* ===================== 1. 16x16 中文字模位图 ===================== */

/**
  * @brief  中文字模位图（1 bpp，MSB 在前，每个字 32 字节）。
  * @note   直接嵌入以避免依赖外部字体文件。
  *         每个字形：16 行 × 2 字节/行 = 32 字节。
  */

static const uint8_t CHINESE_HUAN[32] = {
    0x7D, 0xFE, 0x10, 0x30, 0x10, 0x20, 0x10, 0x20,
    0x7C, 0x68, 0x10, 0x68, 0x10, 0xE4, 0x10, 0xA6,
    0x1D, 0x22, 0x73, 0x20, 0x42, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_JING[32] = {
    0x20, 0x60, 0x23, 0xFC, 0x20, 0x10, 0x20, 0x90,
    0x7F, 0xFE, 0x20, 0x00, 0x23, 0xFC, 0x22, 0x04,
    0x23, 0xFC, 0x22, 0x04, 0x3B, 0xFC, 0x40, 0xA0,
    0x01, 0x22, 0x0E, 0x3E, 0x08, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_JIAN[32] = {
    0x02, 0x20, 0x32, 0x60, 0x32, 0x60, 0x32, 0x7E,
    0x32, 0xD0, 0x32, 0x98, 0x32, 0x8C, 0x02, 0x00,
    0x00, 0x00, 0x3F, 0xFC, 0x32, 0x44, 0x32, 0x44,
    0x32, 0x44, 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_CE[32] = {
    0x00, 0x06, 0x77, 0xC6, 0x14, 0x56, 0x04, 0x56,
    0x45, 0x56, 0x65, 0x56, 0x25, 0x56, 0x05, 0x56,
    0x15, 0x56, 0x15, 0x56, 0x23, 0x96, 0x22, 0x86,
    0x46, 0x46, 0x4C, 0x4E, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_WEN[32] = {
    0x00, 0x00, 0x33, 0xF8, 0x12, 0x08, 0x03, 0xF8,
    0x42, 0x08, 0x32, 0x08, 0x03, 0xF8, 0x00, 0x00,
    0x17, 0xFC, 0x34, 0xA4, 0x24, 0xA4, 0x24, 0xA4,
    0x64, 0xA4, 0x5F, 0xFE, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_DU[32] = {
    0x00, 0x80, 0x00, 0x80, 0x3F, 0xFC, 0x22, 0x10,
    0x22, 0x10, 0x2F, 0xFC, 0x22, 0x10, 0x23, 0xF0,
    0x20, 0x00, 0x2F, 0xF8, 0x23, 0x10, 0x21, 0xA0,
    0x60, 0xC0, 0x47, 0x3E, 0x08, 0x02, 0x00, 0x00
};

static const uint8_t CHINESE_SHI[32] = {
    0x00, 0x00, 0x23, 0xFC, 0x1A, 0x04, 0x03, 0xFC,
    0x02, 0x04, 0x62, 0x04, 0x33, 0xFC, 0x00, 0x00,
    0x04, 0x92, 0x14, 0x92, 0x22, 0x94, 0x22, 0x9C,
    0x60, 0x90, 0x4F, 0xFE, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_GUANG[32] = {
    0x01, 0x80, 0x01, 0x80, 0x11, 0x8C, 0x19, 0x98,
    0x0D, 0xB0, 0x01, 0x80, 0x7F, 0xFE, 0x02, 0x40,
    0x02, 0x40, 0x06, 0x40, 0x04, 0x40, 0x0C, 0x42,
    0x18, 0x46, 0x70, 0x7C, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_ZHAO[32] = {
    0x00, 0x00, 0x3D, 0xFC, 0x24, 0x44, 0x24, 0x44,
    0x24, 0xC4, 0x3D, 0x9C, 0x24, 0xFC, 0x24, 0x84,
    0x24, 0x84, 0x3C, 0xFC, 0x00, 0x00, 0x24, 0x4C,
    0x26, 0x64, 0x62, 0x26, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_YAN[32] = {
    0x10, 0x00, 0x11, 0xFE, 0x11, 0x26, 0x15, 0x26,
    0x5D, 0x26, 0x59, 0xFE, 0x51, 0x26, 0x51, 0x36,
    0x11, 0x56, 0x19, 0x4E, 0x1D, 0x86, 0x25, 0x06,
    0x21, 0xFE, 0x41, 0x06, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_WU[32] = {
    0x3F, 0xFC, 0x00, 0x80, 0x7F, 0xFE, 0x60, 0x82,
    0x6C, 0xB2, 0x1E, 0xF8, 0x04, 0x00, 0x1F, 0xF0,
    0x33, 0x60, 0x07, 0xE0, 0x79, 0x1E, 0x1F, 0xF8,
    0x02, 0x18, 0x3C, 0x70, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_NONG[32] = {
    0x20, 0x60, 0x30, 0x40, 0x17, 0xFC, 0x04, 0x44,
    0x44, 0xC4, 0x70, 0xC0, 0x11, 0xC4, 0x01, 0x2C,
    0x03, 0x38, 0x17, 0x30, 0x2D, 0x18, 0x21, 0x48,
    0x61, 0xC6, 0x41, 0x82, 0x01, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_QI[32] = {
    0x0C, 0x00, 0x08, 0x00, 0x1F, 0xFC, 0x30, 0x00,
    0x2F, 0xF8, 0x60, 0x00, 0x00, 0x00, 0x3F, 0xF0,
    0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x1A,
    0x00, 0x0A, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t CHINESE_TI[32] = {
    0x00, 0x40, 0x18, 0x40, 0x10, 0x40, 0x17, 0xFE,
    0x30, 0xE0, 0x30, 0xD0, 0x51, 0xD0, 0x51, 0x58,
    0x13, 0x48, 0x16, 0x46, 0x1D, 0xFA, 0x10, 0x40,
    0x10, 0x40, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00
};

/**
  * @brief  将 3 字节 UTF-8 中文字符映射到对应的字模指针。
  * @param  utf8  指向 3 字节 UTF-8 序列的指针。
  * @retval 指向 32 字节字模的指针，未找到则返回 NULL。
  */
static const uint8_t *get_chinese_bitmap(const char *utf8)
{
    unsigned int code = ((unsigned char)utf8[0] << 16)
                      | ((unsigned char)utf8[1] << 8)
                      |  (unsigned char)utf8[2];
    switch (code) {
        case 0xE78EAF: return CHINESE_HUAN; /*!< 环 */
        case 0xE5A283: return CHINESE_JING; /*!< 境 */
        case 0xE79B91: return CHINESE_JIAN; /*!< 监 */
        case 0xE6B58B: return CHINESE_CE;   /*!< 测 */
        case 0xE6B8A9: return CHINESE_WEN;  /*!< 温 */
        case 0xE5BAA6: return CHINESE_DU;   /*!< 度 */
        case 0xE6B9BF: return CHINESE_SHI;  /*!< 湿 */
        case 0xE58589: return CHINESE_GUANG;/*!< 光 */
        case 0xE785A7: return CHINESE_ZHAO; /*!< 照 */
        case 0xE7839F: return CHINESE_YAN;  /*!< 烟 */
        case 0xE99BBE: return CHINESE_WU;   /*!< 雾 */
        case 0xE6B5B3: return CHINESE_NONG; /*!< 浓 */
        case 0xE6B094: return CHINESE_QI;   /*!< 气 */
        case 0xE4BD93: return CHINESE_TI;   /*!< 体 */
        default:       return NULL;
    }
}

/* ===================== 2. 本地双缓冲 (100x24 RGB565) ===================== */

#define LEFT_W  100   /*!< 左侧传感器卡片区域宽度 */
#define CARD_H  24    /*!< 每张传感器卡片行高度 */

static uint16_t gui_backbuf[LEFT_W * CARD_H]; /*!< 卡片合成用后备缓冲 */

/**
  * @brief  将后备缓冲清除为纯色。
  */
static void buf_clear(uint16_t color)
{
    for (int i = 0; i < LEFT_W * CARD_H; i++) {
        gui_backbuf[i] = color;
    }
}

/**
  * @brief  在后缓冲中绘制单个像素，带裁剪。
  */
static void buf_pixel(int x, int y, uint16_t color)
{
    if (x >= 0 && x < LEFT_W && y >= 0 && y < CARD_H) {
        gui_backbuf[y * LEFT_W + x] = color;
    }
}

/**
  * @brief  使用字体在后缓冲中绘制 ASCII 字符。
  */
static void buf_draw_char(int x, int y, char ch, FontDef font, uint16_t fg, uint16_t bg)
{
    if (ch < 32 || ch > 126) ch = '?';
    int idx = (ch - 32) * font.height;
    for (int row = 0; row < font.height; row++) {
        uint16_t line = font.data[idx + row];
        for (int col = 0; col < font.width; col++) {
            buf_pixel(x + col, y + row, (line << col) & 0x8000 ? fg : bg);
        }
    }
}

/**
  * @brief  在后缓冲中绘制 ASCII 字符串。
  */
static void buf_draw_string(int x, int y, const char *str, FontDef font, uint16_t fg, uint16_t bg)
{
    while (*str) {
        buf_draw_char(x, y, *str, font, fg, bg);
        x += font.width;
        str++;
    }
}

/**
  * @brief  从字模位图在后缓冲中绘制 16x16 中文字符。
  */
static void buf_draw_chinese(int x, int y, const uint8_t *bitmap, uint16_t fg, uint16_t bg)
{
    if (!bitmap) return;
    for (int row = 0; row < 16; row++) {
        uint8_t high = bitmap[row * 2];
        uint8_t low  = bitmap[row * 2 + 1];
        for (int col = 0; col < 16; col++) {
            uint8_t bit = (col < 8) ? ((high >> (7 - col)) & 1)
                                    : ((low  >> (7 - (col - 8))) & 1);
            buf_pixel(x + col, y + row, bit ? fg : bg);
        }
    }
}

/* ===================== 3. 颜色定义 ===================== */

#define COL_BLACK    0x0000
#define COL_WHITE    0xFFFF
#define COL_TEMP_BG  ST7735_COLOR565(0, 150, 200)   /*!< 温度卡片：青蓝色 */
#define COL_HUM_BG   ST7735_COLOR565(0, 180, 120)   /*!< 湿度卡片：绿色 */
#define COL_LUX_BG   ST7735_COLOR565(255, 160, 0)   /*!< 光照卡片：橙色 */
#define COL_SMOKE_BG ST7735_COLOR565(150, 100, 180) /*!< 烟雾卡片：紫色 */
#define COL_GAS_BG   ST7735_COLOR565(220, 80, 80)   /*!< 气体卡片：红色 */

/* ===================== 4. 传感器实例 ===================== */

static VEML7700_HAL  veml7700;      /*!< 环境光传感器实例 */
static SmokeDetectorHAL smokeDetector; /*!< 烟雾检测器封装实例 */

/**
  * @brief  兼容 C 的 VEML7700 光照度读取封装（NOWAIT 模式）。
  */
extern "C" float veml7700_read_lux(void)
{
    return veml7700.readLux(VEML_LUX_NORMAL_NOWAIT);
}

/**
  * @brief  兼容 C 的 MAX30105 烟雾浓度读取封装。
  */
extern "C" float smoke_detector_read(void)
{
    return smokeDetector.getSmokeConcentration();
}

/* ===================== 5. 左侧卡片刷新 ===================== */

/**
  * @brief  将传感器卡片合成到后缓冲并推送至 ST7735。
  * @param  screen_y   屏幕上的垂直位置 (0, 24, 48, 72, 96)
  * @param  bg_color   卡片背景色
  * @param  label_bmp  指向 16x16 中文标签字模的指针
  * @param  value_str  ASCII 数值字符串（例如 "25.3C"）
  */
static void gui_flush_left_card(int screen_y, uint16_t bg_color,
                                const uint8_t *label_bmp, const char *value_str)
{
    buf_clear(bg_color);
    /* 中文标签 16x16，在 24 像素内垂直居中：(24-16)/2 = 4 */
    buf_draw_chinese(4, 4, label_bmp, COL_WHITE, bg_color);
    /* 数值 11x18，垂直居中：(24-18)/2 = 3 */
    buf_draw_string(22, 3, value_str, Font_11x18, COL_WHITE, bg_color);
    ST7735_PushBuffer(0, screen_y, LEFT_W, CARD_H, gui_backbuf);
}

/* ===================== 6. 公共 API 实现 ===================== */

extern "C" void gui_init(void)
{
    /* 向 GuiLite 主题引擎注册字体。 */
    c_theme::add_font(FONT_DEFAULT, &Font_11x18);
    c_theme::add_font(FONT_CUSTOM1, &Font_16x26);
    c_theme::add_font(FONT_CUSTOM2, &Font_7x10);

    c_theme::add_color(COLOR_WND_FONT,   GL_RGB(255, 255, 255));
    c_theme::add_color(COLOR_WND_NORMAL, GL_RGB(0, 120, 215));

    /* 在 I2C2 上初始化 VEML7700（默认地址 0x10）。 */
    veml7700.begin(&hi2c2, VEML7700_I2CADDR_DEFAULT);

    /* 在 I2C1 上初始化 MAX30105 烟雾检测器（地址 0x57）。 */
    smokeDetector.begin();
    smokeDetector.calibrateBaseline(100); /*!< 100 个采样点，约 500 毫秒 */
}

extern "C" void gui_draw_background(void)
{
    /* 全屏黑色。 */
    ST7735_FillScreenFast(ST7735_BLACK);

    /* 左列：五个传感器卡片 (100x24)。 */
    ST7735_FillRectangleFast(0, 0,   LEFT_W, CARD_H, COL_TEMP_BG);
    ST7735_FillRectangleFast(0, 24,  LEFT_W, CARD_H, COL_HUM_BG);
    ST7735_FillRectangleFast(0, 48,  LEFT_W, CARD_H, COL_LUX_BG);
    ST7735_FillRectangleFast(0, 72,  LEFT_W, CARD_H, COL_SMOKE_BG);
    ST7735_FillRectangleFast(0, 96,  LEFT_W, CARD_H, COL_GAS_BG);

    /* 右列：时间显示区域黑色背景。 */
    ST7735_FillRectangleFast(LEFT_W, 0, 128 - LEFT_W, 128, ST7735_BLACK);
}

extern "C" void gui_update_temp(double temp)
{
    static double last = -999.0;
    int now  = (int)(temp * 10);
    int prev = (int)(last * 10);
    if (now == prev) return; /*!< 无可见变化时跳过。 */

    int whole = (int)temp;
    int frac  = (int)((temp >= 0 ? temp : -temp) * 10) % 10;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%dC", whole, frac);

    gui_flush_left_card(0, COL_TEMP_BG, CHINESE_WEN, buf);
    last = temp;
}

extern "C" void gui_update_hum(double hum)
{
    static double last = -999.0;
    int now  = (int)(hum * 10);
    int prev = (int)(last * 10);
    if (now == prev) return;

    int whole = (int)hum;
    int frac  = (int)((hum >= 0 ? hum : -hum) * 10) % 10;
    char buf[20];
    snprintf(buf, sizeof(buf), "%d.%d%%", whole, frac);

    gui_flush_left_card(24, COL_HUM_BG, CHINESE_SHI, buf);
    last = hum;
}

extern "C" void gui_update_lux(double lux)
{
    static double last_lux = -1.0;
    if (lux < 0) lux = 0;
    if (lux > 999999) lux = 999999;

    int use_klx = (lux >= 1000.0);
    int last_use_klx = (last_lux >= 1000.0);

    if (use_klx == last_use_klx) {
        if (use_klx) {
            if ((int)(lux / 100.0) == (int)(last_lux / 100.0)) return;
        } else {
            if ((int)lux == (int)last_lux) return;
        }
    }

    char buf[16];
    if (use_klx) {
        double klx = lux / 1000.0;
        if (klx < 10.0) {
            int val = (int)(klx * 10.0 + 0.5);
            snprintf(buf, sizeof(buf), "%d.%dk", val / 10, val % 10);
        } else {
            snprintf(buf, sizeof(buf), "%dk", (int)(klx + 0.5));
        }
    } else {
        snprintf(buf, sizeof(buf), "%dlx", (int)lux);
    }

    gui_flush_left_card(48, COL_LUX_BG, CHINESE_GUANG, buf);
    last_lux = lux;
}

extern "C" void gui_update_smoke(double ppm)
{
    static int last = -1;
    int now = (int)(ppm * 10);
    if (now == last) return;

    char buf[20];
    if (ppm < 0) {
        snprintf(buf, sizeof(buf), "--");
    } else {
        int val = (int)(ppm * 10.0 + 0.5);
        snprintf(buf, sizeof(buf), "%d.%dppm", val / 10, val % 10);
    }

    gui_flush_left_card(72, COL_SMOKE_BG, CHINESE_YAN, buf);
    last = now;
}

extern "C" void gui_update_gas(double ppm)
{
    static int last = -1;
    int now = (int)(ppm * 10);
    if (now == last) return;

    char buf[20];
    if (ppm < 0) {
        snprintf(buf, sizeof(buf), "--");
    } else {
        int val = (int)(ppm * 10.0 + 0.5);
        snprintf(buf, sizeof(buf), "%d.%dppm", val / 10, val % 10);
    }

    gui_flush_left_card(96, COL_GAS_BG, CHINESE_QI, buf);
    last = now;
}

/* ===================== 7. RTC 时间显示（右侧面板） ===================== */

extern "C" void gui_update_time(uint8_t hour, uint8_t minute, uint8_t second)
{
    static uint8_t last_h = 0xFF, last_m = 0xFF, last_s = 0xFF;
    char buf[8];

    #define TIME_X  (LEFT_W + 3)  /*!< 右侧面板 11x18 居中 X 偏移 */

    if (hour != last_h) {
        snprintf(buf, sizeof(buf), "%02d", hour);
        ST7735_WriteString(TIME_X, 8, buf, Font_11x18, ST7735_WHITE, ST7735_BLACK);
        last_h = hour;
    }

    if (minute != last_m) {
        snprintf(buf, sizeof(buf), "%02d", minute);
        ST7735_WriteString(TIME_X, 36, buf, Font_11x18, ST7735_WHITE, ST7735_BLACK);
        last_m = minute;
    }

    if (second != last_s) {
        snprintf(buf, sizeof(buf), "%02d", second);
        ST7735_WriteString(TIME_X, 64, buf, Font_11x18, ST7735_WHITE, ST7735_BLACK);
        last_s = second;
    }
}
