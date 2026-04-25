#ifndef __GUI_HPP__
#define __GUI_HPP__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void gui_init(void);
void gui_draw_background(void);
void gui_update_temp(double temp);
void gui_update_hum(double hum);
void gui_update_lux(double lux);
void gui_update_smoke(double ppm);
void gui_update_gas(double ppm);
void gui_update_time(uint8_t hour, uint8_t minute, uint8_t second);

float veml7700_read_lux(void);
float smoke_detector_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __GUI_HPP__ */
