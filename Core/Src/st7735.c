/**
  ******************************************************************************
  * @file    st7735.c
  * @brief   ST7735 128x128 TFT LCD 驱动实现。
  *          支持阻塞式 SPI 写入和异步 DMA 传输，
  *          使用乒乓行缓冲区进行填充和图像操作。
  * @author  Health Monitor Project Team
  * @date    2026
  * @copyright Based on public-domain Adafruit ST7735 code, adapted for STM32 HAL.
  ******************************************************************************
  */

#include "main.h"
#include "st7735.h"
#include "string.h"

/* ===================== DMA 状态 ===================== */

/** @brief DMA 忙标志：1 = 传输进行中，0 = 空闲。 */
static volatile uint8_t st7735_dma_busy = 0;

/** @brief DMA 传输的乒乓行缓冲区（2 x 宽度 x 2 字节 RGB565）。 */
static uint8_t st7735_buf[2][ST7735_WIDTH * 2];

/** @brief DMA 操作模式枚举。 */
typedef enum {
    ST7735_DMA_IDLE = 0, /*!< 无 DMA 活动 */
    ST7735_DMA_FILL,     /*!< 异步矩形填充 */
    ST7735_DMA_IMAGE     /*!< 异步图像绘制 */
} ST7735_DMA_Mode;

/** @brief 内部 DMA 状态跟踪结构体。 */
static struct {
    uint16_t        w;            /*!< 行宽度（像素） */
    uint16_t        rows_total;   /*!< 待传输的总行数 */
    uint16_t        rows_done;    /*!< 已传输的行数 */
    uint8_t         active_buf;   /*!< 当前活动的乒乓缓冲区索引 */
    const uint16_t *image_src;    /*!< 源图像指针（IMAGE 模式） */
    uint16_t        image_stride; /*!< 图像行步幅（像素） */
    ST7735_DMA_Mode mode;         /*!< 当前 DMA 模式 */
} st7735_dma_state;

/** @brief 命令延时标志：与参数计数进行或运算，表示后置延时。 */
#define DELAY 0x80

/* ===================== 初始化命令序列 ===================== */

/**
  * @brief  初始化命令列表第 1 部分（适用于所有变体）。
  * @note   数组格式：[命令数量, 命令, 参数标志, 参数..., ...]。
  *         如果参数标志设置了 DELAY 位，则下一字节为延时毫秒数（255 = 500 毫秒）。
  */
static const uint8_t init_cmds1[] = {
    15,                       /*!< 列表中有 15 条命令 */
    ST7735_SWRESET,   DELAY,  /*!< 1: 软件复位 */
      150,                    /*!<     150 毫秒延时 */
    ST7735_SLPOUT ,   DELAY,  /*!< 2: 退出睡眠模式 */
      255,                    /*!<     500 毫秒延时 */
    ST7735_FRMCTR1, 3      ,  /*!< 3: 帧率控制（正常模式） */
      0x01, 0x2C, 0x2D,       /*!<     速率 = fosc/(1x2+40)*(LINE+2C+2D) */
    ST7735_FRMCTR2, 3      ,  /*!< 4: 帧率控制（空闲模式） */
      0x01, 0x2C, 0x2D,
    ST7735_FRMCTR3, 6      ,  /*!< 5: 帧率控制（部分模式） */
      0x01, 0x2C, 0x2D,       /*!<     点反转 */
      0x01, 0x2C, 0x2D,       /*!<     行反转 */
    ST7735_INVCTR , 1      ,  /*!< 6: 显示反转控制 */
      0x07,                   /*!<     不反转 */
    ST7735_PWCTR1 , 3      ,  /*!< 7: 电源控制 1 */
      0xA2,
      0x02,                   /*!<     -4.6V */
      0x84,                   /*!<     自动模式 */
    ST7735_PWCTR2 , 1      ,  /*!< 8: 电源控制 2 */
      0xC5,                   /*!<     VGH25=2.4C VGSEL=-10 VGH=3*AVDD */
    ST7735_PWCTR3 , 2      ,  /*!< 9: 电源控制 3（正常） */
      0x0A,                   /*!<     运放电流小 */
      0x00,                   /*!<     升压频率 */
    ST7735_PWCTR4 , 2      ,  /*!< 10: 电源控制 4（空闲） */
      0x8A,                   /*!<     BCLK/2，运放小电流与中低电流 */
      0x2A,
    ST7735_PWCTR5 , 2      ,  /*!< 11: 电源控制 5（部分） */
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  /*!< 12: VCOM 控制 */
      0x0E,
    ST7735_INVOFF , 0      ,  /*!< 13: 关闭反转 */
    ST7735_MADCTL , 1      ,  /*!< 14: 存储器访问控制（旋转） */
      ST7735_ROTATION,        /*!<     行/列地址，自下而上刷新 */
    ST7735_COLMOD , 1      ,  /*!< 15: 色彩模式 */
      0x05                    /*!<     16 位色彩（RGB565） */
};

/**
  * @brief  初始化命令列表第 2 部分：128x128 的列/行地址设置。
  */
static const uint8_t init_cmds2[] = {
    2,                        /*!< 2 条命令 */
    ST7735_CASET  , 4      ,  /*!< 1: 列地址设置 */
      0x00, 0x00,             /*!<     XSTART = 0 */
      0x00, 0x7F,             /*!<     XEND = 127 */
    ST7735_RASET  , 4      ,  /*!< 2: 行地址设置 */
      0x00, 0x00,             /*!<     YSTART = 0 */
      0x00, 0x7F              /*!<     YEND = 127 */
};

/**
  * @brief  初始化命令列表第 3 部分：伽马校正 + 显示开启。
  */
static const uint8_t init_cmds3[] = {
    4,                        /*!< 4 条命令 */
    ST7735_GMCTRP1, 16      , /*!< 1: 伽马正极性调整 */
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , /*!< 2: 伽马负极性调整 */
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2d,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, /*!< 3: 正常显示开启 */
      10,                     /*!<     10 毫秒延时 */
    ST7735_DISPON ,    DELAY, /*!< 4: 主屏幕开启 */
      100                     /*!<     100 毫秒延时 */
};

/* ===================== 底层 SPI 辅助函数 ===================== */

/**
  * @brief  拉低片选（低电平有效）。
  */
static void ST7735_Select(void)
{
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  释放片选（高电平无效）。
  */
void ST7735_Unselect(void)
{
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  SPI 发送完成回调（由 HAL 在每行 DMA 传输后调用）。
  * @note   切换乒乓缓冲区并启动下一行，直到全部完成。
  * @param  hspi  SPI 句柄
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI1) return;

    st7735_dma_state.rows_done++;

    if (st7735_dma_state.rows_done < st7735_dma_state.rows_total) {
        uint8_t next_buf = st7735_dma_state.active_buf ^ 1;

        if (st7735_dma_state.mode == ST7735_DMA_IMAGE) {
            uint16_t next_row = st7735_dma_state.rows_done;
            const uint8_t *src = (const uint8_t *)(st7735_dma_state.image_src
                                                   + next_row * st7735_dma_state.image_stride);
            memcpy(st7735_buf[next_buf], src, st7735_dma_state.w * 2);
        }
        /* FILL 模式：缓冲区已包含相同颜色，无需重新填充。 */

        st7735_dma_state.active_buf = next_buf;
        HAL_SPI_Transmit_DMA(&ST7735_SPI_PORT, st7735_buf[next_buf], st7735_dma_state.w * 2);
    } else {
        ST7735_Unselect();
        st7735_dma_busy = 0;
    }
}

/**
  * @brief  硬件复位时序：拉低 RST 5 毫秒，然后释放。
  */
static void ST7735_Reset(void)
{
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
}

/**
  * @brief  向 ST7735 写入 8 位命令（DC = 低电平）。
  * @param  cmd  命令字节
  */
static void ST7735_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

/**
  * @brief  向 ST7735 写入数据字节（DC = 高电平）。
  * @param  buff       数据缓冲区指针
  * @param  buff_size  要发送的字节数
  */
static void ST7735_WriteData(uint8_t *buff, size_t buff_size)
{
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);
}

/**
  * @brief  执行紧凑命令列表（初始化序列）。
  * @note   格式：首字节 = 命令数量，然后每条命令：
  *         [命令字节, 带延时标志的参数计数, 参数..., 可选延时毫秒数]。
  * @param  addr  命令列表数组指针
  */
static void ST7735_ExecuteCommandList(const uint8_t *addr)
{
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while (numCommands--) {
        uint8_t cmd = *addr++;
        ST7735_WriteCommand(cmd);

        numArgs = *addr++;
        ms = numArgs & DELAY;
        numArgs &= ~DELAY;
        if (numArgs) {
            ST7735_WriteData((uint8_t *)addr, numArgs);
            addr += numArgs;
        }

        if (ms) {
            ms = *addr++;
            if (ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

/**
  * @brief  设置 ST7735 地址窗口，用于后续的 RAM 写入。
  * @param  x0  列起始
  * @param  y0  行起始
  * @param  x1  列结束（包含）
  * @param  y1  行结束（包含）
  */
static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    /* 列地址设置 */
    ST7735_WriteCommand(ST7735_CASET);
    uint8_t data[] = { 0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
    ST7735_WriteData(data, sizeof(data));

    /* 行地址设置 */
    ST7735_WriteCommand(ST7735_RASET);
    data[1] = y0 + ST7735_YSTART;
    data[3] = y1 + ST7735_YSTART;
    ST7735_WriteData(data, sizeof(data));

    /* 开始 RAM 写入 */
    ST7735_WriteCommand(ST7735_RAMWR);
}

/* ===================== 公共 API 实现 ===================== */

void ST7735_Init(void)
{
    ST7735_Select();
    ST7735_Reset();
    ST7735_ExecuteCommandList(init_cmds1);
    ST7735_ExecuteCommandList(init_cmds2);
    ST7735_ExecuteCommandList(init_cmds3);
    ST7735_Unselect();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x + 1, y + 1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7735_WriteData(data, sizeof(data));
    ST7735_Unselect();
}

/**
  * @brief  使用指定字体绘制单个 ASCII 字符（阻塞式）。
  * @param  x       起始 X 坐标
  * @param  y       起始 Y 坐标
  * @param  ch      要绘制的字符（32..126）
  * @param  font    字体定义
  * @param  color   前景 RGB565 颜色
  * @param  bgcolor 背景 RGB565 颜色
  */
static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font,
                             uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;

    ST7735_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

    for (i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for (j = 0; j < font.width; j++) {
            if ((b << j) & 0x8000) {
                uint8_t data[] = { color >> 8, color & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            }
        }
    }
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font,
                        uint16_t color, uint16_t bgcolor)
{
    ST7735_Select();

    while (*str) {
        if (x + font.width >= ST7735_WIDTH) {
            x = 0;
            y += font.height;
            if (y + font.height >= ST7735_HEIGHT) {
                break;
            }
            if (*str == ' ') {
                /* 跳过新行开头的空格。 */
                str++;
                continue;
            }
        }

        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ST7735_Unselect();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    /* 裁剪 */
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if ((x + w - 1) >= ST7735_WIDTH)  w = ST7735_WIDTH - x;
    if ((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    for (y = h; y > 0; y--) {
        for (x = w; x > 0; x--) {
            HAL_SPI_Transmit(&ST7735_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    ST7735_Unselect();
}

bool ST7735_IsBusy(void)
{
    return st7735_dma_busy;
}

void ST7735_FillRectangleAsync(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if ((x + w - 1) >= ST7735_WIDTH)  w = ST7735_WIDTH - x;
    if ((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;
    if (st7735_dma_busy) return;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (uint16_t i = 0; i < w; i++) {
        st7735_buf[0][i * 2]     = hi;
        st7735_buf[0][i * 2 + 1] = lo;
        st7735_buf[1][i * 2]     = hi;
        st7735_buf[1][i * 2 + 1] = lo;
    }

    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);

    st7735_dma_state.w            = w;
    st7735_dma_state.rows_total   = h;
    st7735_dma_state.rows_done    = 0;
    st7735_dma_state.active_buf   = 0;
    st7735_dma_state.image_src    = NULL;
    st7735_dma_state.image_stride = 0;
    st7735_dma_state.mode         = ST7735_DMA_FILL;
    st7735_dma_busy               = 1;

    HAL_SPI_Transmit_DMA(&ST7735_SPI_PORT, st7735_buf[0], w * 2);
}

void ST7735_FillScreenAsync(uint16_t color)
{
    ST7735_FillRectangleAsync(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_DrawImageAsync(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if ((x + w - 1) >= ST7735_WIDTH)  return;
    if ((y + h - 1) >= ST7735_HEIGHT) return;
    if (st7735_dma_busy) return;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);

    memcpy(st7735_buf[0], (const uint8_t *)data, w * 2);
    if (h > 1) {
        memcpy(st7735_buf[1], (const uint8_t *)data + w * 2, w * 2);
    }

    st7735_dma_state.w            = w;
    st7735_dma_state.rows_total   = h;
    st7735_dma_state.rows_done    = 0;
    st7735_dma_state.active_buf   = 0;
    st7735_dma_state.image_src    = data;
    st7735_dma_state.image_stride = w;
    st7735_dma_state.mode         = ST7735_DMA_IMAGE;
    st7735_dma_busy               = 1;

    HAL_SPI_Transmit_DMA(&ST7735_SPI_PORT, st7735_buf[0], w * 2);
}

void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ST7735_FillRectangleAsync(x, y, w, h, color);
    while (st7735_dma_busy);
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_FillScreenAsync(color);
    while (st7735_dma_busy);
}

void ST7735_FillScreenFast(uint16_t color)
{
    ST7735_FillScreenAsync(color);
    while (st7735_dma_busy);
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    ST7735_DrawImageAsync(x, y, w, h, data);
    while (st7735_dma_busy);
}

void ST7735_PushBuffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *buffer)
{
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if ((x + w - 1) >= ST7735_WIDTH)  w = ST7735_WIDTH - x;
    if ((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, (uint8_t *)buffer, w * h * 2, HAL_MAX_DELAY);
    ST7735_Unselect();
}

void ST7735_InvertColors(bool invert)
{
    ST7735_Select();
    ST7735_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
    ST7735_Unselect();
}

void ST7735_SetGamma(GammaDef gamma)
{
    ST7735_Select();
    ST7735_WriteCommand(ST7735_GAMSET);
    ST7735_WriteData((uint8_t *)&gamma, sizeof(gamma));
    ST7735_Unselect();
}
