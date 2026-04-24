/*!
 *  @file VEML7700_HAL.h
 *
 *  STM32 HAL Driver for VEML7700 Lux sensor
 *
 *  Ported from Adafruit_VEML7700 Arduino library.
 *  Uses STM32 HAL I2C APIs directly.
 */

#ifndef _VEML7700_HAL_H
#define _VEML7700_HAL_H

#include "main.h"  // Contains stm32xxxx_hal.h and I2C_HandleTypeDef

#define VEML7700_I2CADDR_DEFAULT 0x10 ///< Default I2C address

#define VEML7700_ALS_CONFIG 0x00        ///< Light configuration register
#define VEML7700_ALS_THREHOLD_HIGH 0x01 ///< Light high threshold for irq
#define VEML7700_ALS_THREHOLD_LOW 0x02  ///< Light low threshold for irq
#define VEML7700_ALS_POWER_SAVE 0x03    ///< Power save register
#define VEML7700_ALS_DATA 0x04          ///< The light data output
#define VEML7700_WHITE_DATA 0x05        ///< The white light data output
#define VEML7700_INTERRUPTSTATUS 0x06   ///< What IRQ (if any)

#define VEML7700_INTERRUPT_HIGH 0x4000 ///< Interrupt status for high threshold
#define VEML7700_INTERRUPT_LOW 0x8000  ///< Interrupt status for low threshold

#define VEML7700_GAIN_1 0x00   ///< ALS gain 1x
#define VEML7700_GAIN_2 0x01   ///< ALS gain 2x
#define VEML7700_GAIN_1_8 0x02 ///< ALS gain 1/8x
#define VEML7700_GAIN_1_4 0x03 ///< ALS gain 1/4x

#define VEML7700_IT_100MS 0x00 ///< ALS integration time 100ms
#define VEML7700_IT_200MS 0x01 ///< ALS integration time 200ms
#define VEML7700_IT_400MS 0x02 ///< ALS integration time 400ms
#define VEML7700_IT_800MS 0x03 ///< ALS integration time 800ms
#define VEML7700_IT_50MS 0x08  ///< ALS integration time 50ms
#define VEML7700_IT_25MS 0x0C  ///< ALS integration time 25ms

#define VEML7700_PERS_1 0x00 ///< ALS irq persistance 1 sample
#define VEML7700_PERS_2 0x01 ///< ALS irq persistance 2 samples
#define VEML7700_PERS_4 0x02 ///< ALS irq persistance 4 samples
#define VEML7700_PERS_8 0x03 ///< ALS irq persistance 8 samples

#define VEML7700_POWERSAVE_MODE1 0x00 ///< Power saving mode 1
#define VEML7700_POWERSAVE_MODE2 0x01 ///< Power saving mode 2
#define VEML7700_POWERSAVE_MODE3 0x02 ///< Power saving mode 3
#define VEML7700_POWERSAVE_MODE4 0x03 ///< Power saving mode 4

/** Options for lux reading method */
typedef enum {
  VEML_LUX_NORMAL,
  VEML_LUX_CORRECTED,
  VEML_LUX_AUTO,
  VEML_LUX_NORMAL_NOWAIT,
  VEML_LUX_CORRECTED_NOWAIT
} luxMethod;

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            VEML7700 Light Sensor on STM32 HAL
 */
class VEML7700_HAL {
public:
  VEML7700_HAL();
  bool begin(I2C_HandleTypeDef *hi2c, uint8_t addr = VEML7700_I2CADDR_DEFAULT);

  void enable(bool enable);
  bool enabled(void);

  void interruptEnable(bool enable);
  bool interruptEnabled(void);
  void setPersistence(uint8_t pers);
  uint8_t getPersistence(void);
  void setIntegrationTime(uint8_t it, bool wait = true);
  uint8_t getIntegrationTime(void);
  int getIntegrationTimeValue(void);
  void setGain(uint8_t gain);
  uint8_t getGain(void);
  float getGainValue(void);
  void powerSaveEnable(bool enable);
  bool powerSaveEnabled(void);
  void setPowerSaveMode(uint8_t mode);
  uint8_t getPowerSaveMode(void);

  void setLowThreshold(uint16_t value);
  uint16_t getLowThreshold(void);
  void setHighThreshold(uint16_t value);
  uint16_t getHighThreshold(void);
  uint16_t interruptStatus(void);

  uint16_t readALS(bool wait = false);
  uint16_t readWhite(bool wait = false);
  float readLux(luxMethod method = VEML_LUX_NORMAL);

private:
  static const float MAX_RES;
  static const float GAIN_MAX;
  static const float IT_MAX;
  float getResolution(void);
  float computeLux(uint16_t rawALS, bool corrected = false);
  float autoLux(void);
  void readWait(void);
  uint32_t lastRead;

  I2C_HandleTypeDef *_hi2c;
  uint16_t _addr;  // 8-bit I2C address (7-bit << 1)

  bool writeRegister(uint8_t reg, uint16_t value);
  uint16_t readRegister(uint8_t reg);
};

#endif
