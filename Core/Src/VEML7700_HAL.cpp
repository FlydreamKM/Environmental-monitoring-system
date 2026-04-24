/*!
 *  @file VEML7700_HAL.cpp
 *
 *  STM32 HAL Driver for VEML7700 Lux sensor
 *
 *  Ported from Adafruit_VEML7700 Arduino library.
 */

#include "VEML7700_HAL.h"

const float VEML7700_HAL::MAX_RES = 0.0036f;
const float VEML7700_HAL::GAIN_MAX = 2.0f;
const float VEML7700_HAL::IT_MAX = 800.0f;

/*!
 *    @brief  Instantiates a new VEML7700_HAL class
 */
VEML7700_HAL::VEML7700_HAL(void) {}

/*!
 *    @brief  Sets up the hardware for talking to the VEML7700
 *    @param  hi2c Pointer to HAL I2C handle (e.g. &hi2c1)
 *    @param  addr 7-bit I2C address, default 0x10
 *    @return True if initialization was successful, otherwise false.
 */
bool VEML7700_HAL::begin(I2C_HandleTypeDef *hi2c, uint8_t addr) {
  _hi2c = hi2c;
  _addr = (uint16_t)(addr << 1);  // HAL expects left-aligned address

  // Check if device is present on bus
  if (HAL_I2C_IsDeviceReady(_hi2c, _addr, 3, 100) != HAL_OK) {
    return false;
  }

  // Default init sequence (matches original Arduino driver)
  enable(false);
  interruptEnable(false);
  setPersistence(VEML7700_PERS_1);
  setGain(VEML7700_GAIN_1_8);
  setIntegrationTime(VEML7700_IT_100MS, false);
  powerSaveEnable(false);
  enable(true);

  lastRead = HAL_GetTick();
  return true;
}

/*!
 *    @brief  Write a 16-bit little-endian register value via HAL I2C
 */
bool VEML7700_HAL::writeRegister(uint8_t reg, uint16_t value) {
  uint8_t buf[2];
  buf[0] = (uint8_t)(value & 0xFF);       // LSB
  buf[1] = (uint8_t)((value >> 8) & 0xFF); // MSB
  return (HAL_I2C_Mem_Write(_hi2c, _addr, reg,
                            I2C_MEMADD_SIZE_8BIT, buf, 2, 100) == HAL_OK);
}

/*!
 *    @brief  Read a 16-bit little-endian register value via HAL I2C
 */
uint16_t VEML7700_HAL::readRegister(uint8_t reg) {
  uint8_t buf[2];
  if (HAL_I2C_Mem_Read(_hi2c, _addr, reg,
                       I2C_MEMADD_SIZE_8BIT, buf, 2, 100) != HAL_OK) {
    return 0;
  }
  return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

/*!
 *    @brief Enable or disable the sensor
 *    @param enable The flag to enable/disable
 */
void VEML7700_HAL::enable(bool enable) {
  uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
  if (enable) {
    cfg &= ~(uint16_t)0x0001; // bit 0 = 0 (active)
  } else {
    cfg |= 0x0001;            // bit 0 = 1 (shutdown)
  }
  writeRegister(VEML7700_ALS_CONFIG, cfg);

  // From app note:
  // When activating, wait 2.5 ms before first measurement.
  if (enable) {
    HAL_Delay(5); // doubling 2.5ms spec to be sure
  }
}

/*!
 *    @brief Ask if the sensor is enabled
 *    @returns True if enabled, false otherwise
 */
bool VEML7700_HAL::enabled(void) {
  return (readRegister(VEML7700_ALS_CONFIG) & 0x0001) == 0;
}

/*!
 *    @brief Enable or disable the interrupt
 *    @param enable The flag to enable/disable
 */
void VEML7700_HAL::interruptEnable(bool enable) {
  uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
  if (enable) {
    cfg |= 0x0002;  // bit 1 = 1
  } else {
    cfg &= ~(uint16_t)0x0002; // bit 1 = 0
  }
  writeRegister(VEML7700_ALS_CONFIG, cfg);
}

/*!
 *    @brief Ask if the interrupt is enabled
 *    @returns True if enabled, false otherwise
 */
bool VEML7700_HAL::interruptEnabled(void) {
  return (readRegister(VEML7700_ALS_CONFIG) & 0x0002) != 0;
}

/*!
 *    @brief Set the ALS IRQ persistance setting
 *    @param pers Persistance constant
 */
void VEML7700_HAL::setPersistence(uint8_t pers) {
  uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
  cfg &= ~(uint16_t)(0x0003 << 4);      // clear bits 4-5
  cfg |= (uint16_t)((pers & 0x03) << 4); // set bits 4-5
  writeRegister(VEML7700_ALS_CONFIG, cfg);
}

/*!
 *    @brief Get the ALS IRQ persistance setting
 *    @returns Persistance constant
 */
uint8_t VEML7700_HAL::getPersistence(void) {
  return (uint8_t)((readRegister(VEML7700_ALS_CONFIG) >> 4) & 0x03);
}

/*!
 *    @brief Set ALS integration time
 *    @param it Integration time constant
 *    @param wait Waits to insure old integration time cycle has completed.
 */
void VEML7700_HAL::setIntegrationTime(uint8_t it, bool wait) {
  int flushDelay = wait ? getIntegrationTimeValue() : 0;

  uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
  cfg &= ~(uint16_t)(0x000F << 6);      // clear bits 6-9
  cfg |= (uint16_t)((it & 0x0F) << 6);   // set bits 6-9
  writeRegister(VEML7700_ALS_CONFIG, cfg);

  HAL_Delay(flushDelay);
  lastRead = HAL_GetTick();
}

/*!
 *    @brief Get ALS integration time setting
 *    @returns IT index
 */
uint8_t VEML7700_HAL::getIntegrationTime(void) {
  return (uint8_t)((readRegister(VEML7700_ALS_CONFIG) >> 6) & 0x0F);
}

/*!
 *    @brief Get ALS integration time value in milliseconds
 *    @returns Integration time in ms
 */
int VEML7700_HAL::getIntegrationTimeValue(void) {
  switch (getIntegrationTime()) {
  case VEML7700_IT_25MS:
    return 25;
  case VEML7700_IT_50MS:
    return 50;
  case VEML7700_IT_100MS:
    return 100;
  case VEML7700_IT_200MS:
    return 200;
  case VEML7700_IT_400MS:
    return 400;
  case VEML7700_IT_800MS:
    return 800;
  default:
    return -1;
  }
}

/*!
 *    @brief Set ALS gain
 *    @param gain Gain constant
 */
void VEML7700_HAL::setGain(uint8_t gain) {
  uint16_t cfg = readRegister(VEML7700_ALS_CONFIG);
  cfg &= ~(uint16_t)(0x0003 << 11);       // clear bits 11-12
  cfg |= (uint16_t)((gain & 0x03) << 11);  // set bits 11-12
  writeRegister(VEML7700_ALS_CONFIG, cfg);
  lastRead = HAL_GetTick(); // reset
}

/*!
 *    @brief Get ALS gain setting
 *    @returns Gain index
 */
uint8_t VEML7700_HAL::getGain(void) {
  return (uint8_t)((readRegister(VEML7700_ALS_CONFIG) >> 11) & 0x03);
}

/*!
 *    @brief Get ALS gain value
 *    @returns Actual gain value as float
 */
float VEML7700_HAL::getGainValue(void) {
  switch (getGain()) {
  case VEML7700_GAIN_1_8:
    return 0.125f;
  case VEML7700_GAIN_1_4:
    return 0.25f;
  case VEML7700_GAIN_1:
    return 1.0f;
  case VEML7700_GAIN_2:
    return 2.0f;
  default:
    return -1.0f;
  }
}

/*!
 *    @brief Enable power save mode
 *    @param enable True if power save should be enabled
 */
void VEML7700_HAL::powerSaveEnable(bool enable) {
  uint16_t psm = readRegister(VEML7700_ALS_POWER_SAVE);
  if (enable) {
    psm |= 0x0001;  // bit 0 = 1
  } else {
    psm &= ~(uint16_t)0x0001; // bit 0 = 0
  }
  writeRegister(VEML7700_ALS_POWER_SAVE, psm);
}

/*!
 *    @brief Check if power save mode is enabled
 *    @returns True if power save is enabled
 */
bool VEML7700_HAL::powerSaveEnabled(void) {
  return (readRegister(VEML7700_ALS_POWER_SAVE) & 0x0001) != 0;
}

/*!
 *    @brief Assign the power save register data
 *    @param mode Power save mode constant
 */
void VEML7700_HAL::setPowerSaveMode(uint8_t mode) {
  uint16_t psm = readRegister(VEML7700_ALS_POWER_SAVE);
  psm &= ~(uint16_t)(0x0003 << 1);      // clear bits 1-2
  psm |= (uint16_t)((mode & 0x03) << 1); // set bits 1-2
  writeRegister(VEML7700_ALS_POWER_SAVE, psm);
}

/*!
 *    @brief  Retrieve the power save register data
 *    @return Power save mode constant
 */
uint8_t VEML7700_HAL::getPowerSaveMode(void) {
  return (uint8_t)((readRegister(VEML7700_ALS_POWER_SAVE) >> 1) & 0x03);
}

/*!
 *    @brief Assign the low threshold register data
 *    @param value The 16-bit data to write
 */
void VEML7700_HAL::setLowThreshold(uint16_t value) {
  writeRegister(VEML7700_ALS_THREHOLD_LOW, value);
}

/*!
 *    @brief  Retrieve the low threshold register data
 *    @return 16-bit data
 */
uint16_t VEML7700_HAL::getLowThreshold(void) {
  return readRegister(VEML7700_ALS_THREHOLD_LOW);
}

/*!
 *    @brief Assign the high threshold register data
 *    @param value The 16-bit data to write
 */
void VEML7700_HAL::setHighThreshold(uint16_t value) {
  writeRegister(VEML7700_ALS_THREHOLD_HIGH, value);
}

/*!
 *    @brief  Retrieve the high threshold register data
 *    @return 16-bit data
 */
uint16_t VEML7700_HAL::getHighThreshold(void) {
  return readRegister(VEML7700_ALS_THREHOLD_HIGH);
}

/*!
 *    @brief  Retrieve the interrupt status register data
 *    @return 16-bit interrupt status
 */
uint16_t VEML7700_HAL::interruptStatus(void) {
  return readRegister(VEML7700_INTERRUPTSTATUS);
}

/*!
 *    @brief Read the raw ALS data
 *    @param wait If true, wait based on integration time before reading
 *    @returns 16-bit data value from the ALS register
 */
uint16_t VEML7700_HAL::readALS(bool wait) {
  if (wait)
    readWait();
  lastRead = HAL_GetTick();
  return readRegister(VEML7700_ALS_DATA);
}

/*!
 *    @brief Read the raw white light data
 *    @param wait If true, wait based on integration time before reading
 *    @returns 16-bit data value from the WHITE register
 */
uint16_t VEML7700_HAL::readWhite(bool wait) {
  if (wait)
    readWait();
  lastRead = HAL_GetTick();
  return readRegister(VEML7700_WHITE_DATA);
}

/*!
 *    @brief Read the calibrated lux value.
 *    @param method Lux computation method to use.
 *    @returns Floating point Lux data
 */
float VEML7700_HAL::readLux(luxMethod method) {
  bool wait = true;
  switch (method) {
  case VEML_LUX_NORMAL_NOWAIT:
    wait = false;
    /* fall through */
  case VEML_LUX_NORMAL:
    return computeLux(readALS(wait));
  case VEML_LUX_CORRECTED_NOWAIT:
    wait = false;
    /* fall through */
  case VEML_LUX_CORRECTED:
    return computeLux(readALS(wait), true);
  case VEML_LUX_AUTO:
    return autoLux();
  default:
    return -1.0f;
  }
}

/*!
 *    @brief Determines resolution for current gain and integration time
 */
float VEML7700_HAL::getResolution(void) {
  return MAX_RES * (IT_MAX / (float)getIntegrationTimeValue()) *
         (GAIN_MAX / getGainValue());
}

/*!
 *    @brief Compute lux from ALS reading.
 *    @param rawALS raw ALS register value
 *    @param corrected if true, apply non-linear correction
 *    @return lux value
 */
float VEML7700_HAL::computeLux(uint16_t rawALS, bool corrected) {
  float lux = getResolution() * (float)rawALS;
  if (corrected)
    lux = (((6.0135e-13f * lux - 9.3924e-9f) * lux + 8.1488e-5f) * lux +
           1.0023f) *
          lux;
  return lux;
}

/*!
 *    @brief Wait for integration time to elapse since last read.
 *    Based on app note: wait at least programmed integration time.
 */
void VEML7700_HAL::readWait(void) {
  uint32_t timeToWait = 2 * (uint32_t)getIntegrationTimeValue();
  uint32_t timeWaited = HAL_GetTick() - lastRead;

  if (timeWaited < timeToWait)
    HAL_Delay(timeToWait - timeWaited);
}

/*!
 *  @brief Implementation of App Note "Designing the VEML7700 Into an
 *  Application", Vishay Document Number: 84323, Fig. 24 Flow Chart.
 *  Automatically adjusts gain and integration time as needed.
 */
float VEML7700_HAL::autoLux(void) {
  const uint8_t gains[] = {VEML7700_GAIN_1_8, VEML7700_GAIN_1_4,
                           VEML7700_GAIN_1, VEML7700_GAIN_2};
  const uint8_t intTimes[] = {VEML7700_IT_25MS,  VEML7700_IT_50MS,
                              VEML7700_IT_100MS, VEML7700_IT_200MS,
                              VEML7700_IT_400MS, VEML7700_IT_800MS};

  uint8_t gainIndex = 0;      // start with ALS gain = 1/8
  uint8_t itIndex = 2;        // start with ALS integration time = 100ms
  bool useCorrection = false; // flag for non-linear correction

  setGain(gains[gainIndex]);
  setIntegrationTime(intTimes[itIndex]);

  uint16_t ALS = readALS(true);

  if (ALS <= 100) {
    // increase first gain and then integration time as needed
    while ((ALS <= 100) && !((gainIndex == 3) && (itIndex == 5))) {
      if (gainIndex < 3) {
        setGain(gains[++gainIndex]);
      } else if (itIndex < 5) {
        setIntegrationTime(intTimes[++itIndex]);
      }
      ALS = readALS(true);
    }
  } else {
    // decrease integration time as needed
    useCorrection = true;
    while ((ALS > 10000) && (itIndex > 0)) {
      setIntegrationTime(intTimes[--itIndex]);
      ALS = readALS(true);
    }
  }

  return computeLux(ALS, useCorrection);
}
