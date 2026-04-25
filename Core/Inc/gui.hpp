/**
  ******************************************************************************
  * @file    gui.hpp
  * @brief   健康监测显示器显示层 C 兼容 GUI API 头文件。
  *          声明初始化、背景绘制、传感器数值更新函数，
  *          以及 main.c 使用的光照/烟雾读取封装。
  * @author  Health Monitor Project Team
  * @date    2026
  ******************************************************************************
  */

#ifndef __GUI_HPP__
#define __GUI_HPP__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== GUI 生命周期 ===================== */

/**
  * @brief  初始化 GUI：注册字体/颜色，初始化 VEML7700 与 MAX30105。
  */
void gui_init(void);

/**
  * @brief  绘制静态背景：左侧传感器卡片 + 右侧黑色面板。
  */
void gui_draw_background(void);

/* ===================== 传感器数值更新 ===================== */

/**
  * @brief  更新左侧卡片温度显示（仅在数值变化时刷新）。
  * @param  temp  摄氏温度。
  */
void gui_update_temp(double temp);

/**
  * @brief  更新左侧卡片湿度显示（仅在数值变化时刷新）。
  * @param  hum  相对湿度（%RH）。
  */
void gui_update_hum(double hum);

/**
  * @brief  更新左侧卡片照度显示（仅在数值变化时刷新）。
  * @param  lux  环境光照度（lux，支持自动切换 klx）。
  */
void gui_update_lux(double lux);

/**
  * @brief  更新左侧卡片烟雾浓度显示（仅在数值变化时刷新）。
  * @param  ppm  烟雾浓度（ppm）；负值显示 "--"。
  */
void gui_update_smoke(double ppm);

/**
  * @brief  更新左侧卡片可燃气体显示（仅在数值变化时刷新）。
  * @param  ppm  气体浓度（ppm）；负值显示 "--"。
  */
void gui_update_gas(double ppm);

/**
  * @brief  更新右侧面板 RTC 时间显示（仅刷新变化字段）。
  * @param  hour    小时 (0..23)
  * @param  minute  分钟 (0..59)
  * @param  second  秒 (0..59)
  */
void gui_update_time(uint8_t hour, uint8_t minute, uint8_t second);

/* ===================== 传感器读取封装 ===================== */

/**
  * @brief  读取 VEML7700 光照度（NOWAIT 模式，在主循环中调用）。
  * @retval 环境光照度（lux）。
  */
float veml7700_read_lux(void);

/**
  * @brief  读取 MAX30105 烟雾浓度封装。
  * @retval 烟雾浓度（ppm）。
  */
float smoke_detector_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __GUI_HPP__ */
