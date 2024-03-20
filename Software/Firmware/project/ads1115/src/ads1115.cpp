#include "ads1115.hpp"

/**
 * @brief Starts the ADS1115 sensor.
 */
void ADS1115::start() {
  ADS1115settings settings;
  settings.samplingRate = ADS1115settings::FS860HZ;
  ads1115settings = settings;

  if (settings.initPIGPIO) {
    int cfg = gpioCfgGetInternals();
    cfg |= PI_CFG_NOSIGHANDLER;
    gpioCfgSetInternals(cfg);
    int r = gpioInitialise();
    if (r < 0) {
      std::string msg = "Cannot init pigpio.";
#ifdef DEBUG
      std::cerr << msg << '\n';
#endif
      throw std::runtime_error(msg);
    }
  }

#ifdef DEBUG
  std::cerr << "Init .\n";
#endif

  // Enable RDY
  i2c_writeWord(reg_lo_thres, 0x0000);
  i2c_writeWord(reg_hi_thres, 0x8000);

  unsigned r = (0b10000000 << 8);  // kick it all off
  r = r | (1 << 2) | (1 << 3);     // data ready active high & latching
  r = r | (settings.samplingRate << 5);
  r = r | (settings.pgaGain << 9);
  r = r | (settings.channel << 12) | 1 << 14;  // unipolar
  i2c_writeWord(reg_config, r);

#ifdef DEBUG
  std::cerr << "Receiving data.\n";
#endif

  int result = gpioSetMode(settings.drdy_gpio, PI_INPUT);
  if (result < 0) {
    throw std::runtime_error("Failed to set GPIO mode, error " +
                             std::to_string(result));
  }
  result = gpioSetISRFuncEx(settings.drdy_gpio, RISING_EDGE, ISR_TIMEOUT,
                            gpioISR, (void *)this);
  if (result < 0) {
    throw std::runtime_error("Failed to set GPIO ISR function, error " +
                             std::to_string(result));
  }
}

/**
 * @brief Sets the channel for the ADS1115 sensor.
 *
 * @param channel The channel to set.
 */
void ADS1115::setChannel(ADS1115settings::Input channel) {
  unsigned r = i2c_readWord(reg_config);
  r = r & (~(3 << 12));
  r = r | (channel << 12);
  i2c_writeWord(reg_config, r);
  ads1115settings.channel = channel;
}

/**
 * @brief Handles the event when data is ready.
 */
void ADS1115::dataReady() {
  float v =
      (float)i2c_readConversion() / (float)0x7fff * fullScaleVoltage() * 2;
  hasSample(v);
}

/**
 * @brief Stops the ADS1115 sensor.
 */
void ADS1115::stop() {
  int result = gpioSetISRFuncEx(ads1115settings.drdy_gpio, RISING_EDGE, -1,
                                nullptr, static_cast<void *>(this));
  if (result < 0) {
    throw std::runtime_error("Failed to set GPIO ISR function, error " +
                             std::to_string(result));
  }

  if (ads1115settings.initPIGPIO) {
    gpioTerminate();
  }
}

/**
 * @brief Writes a word to the I2C bus.
 *
 * @param reg The register to write to.
 * @param data The data to write.
 */
void ADS1115::i2c_writeWord(uint8_t reg, unsigned data) {
  int fd = i2cOpen(ads1115settings.i2c_bus, ads1115settings.address, 0);
  if (fd < 0) {
#ifdef DEBUG
    std::cerr << "Could not open " << std::hex << ads1115settings.address
              << ".\n";
#endif
    throw std::runtime_error("Could not open i2c.");
  }
  char tmp[2];
  tmp[0] = (char)((data & 0xff00) >> 8);
  tmp[1] = (char)(data & 0x00ff);
  int r = i2cWriteI2CBlockData(fd, reg, tmp, 2);
  if (r < 0) {
#ifdef DEBUG
    std::cerr << "Could not write word from " << std::hex
              << ads1115settings.address << ". ret=" << r << ".\n";
#endif
    throw std::runtime_error("Could not read from i2c.");
  }
  i2cClose(fd);
}

/**
 * @brief Reads a word from the I2C bus.
 *
 * @param reg The register to read from.
 * @return unsigned The data read.
 */
unsigned ADS1115::i2c_readWord(uint8_t reg) {
  int fd = i2cOpen(ads1115settings.i2c_bus, ads1115settings.address, 0);
  if (fd < 0) {
#ifdef DEBUG
    std::cerr << "Could not open " << std::hex << ads1115settings.address
              << ".\n";
#endif
    throw std::runtime_error("Could not open i2c.");
  }
  int r;
  char tmp[2];
  r = i2cReadI2CBlockData(fd, reg, tmp, 2);
  if (fd < 0) {
#ifdef DEBUG
    std::cerr << "Could not read word from " << std::hex
              << ads1115settings.address << ". ret=" << r << ".\n";
#endif
    throw std::runtime_error("Could not read from i2c.");
  }
  i2cClose(fd);
  return (((unsigned)(tmp[0])) << 8) | ((unsigned)(tmp[1]));
}

/**
 * @brief Reads the conversion result from the I2C bus.
 *
 * @return int The conversion result.
 */
int ADS1115::i2c_readConversion() {
  const int reg = 0;
  int fd = i2cOpen(ads1115settings.i2c_bus, ads1115settings.address, 0);
  if (fd < 0) {
#ifdef DEBUG
    std::cerr << "Could not open " << std::hex << ads1115settings.address
              << ".\n";
#endif
    throw std::runtime_error("Could not open i2c.");
  }
  int r;
  char tmp[2];
  r = i2cReadI2CBlockData(fd, reg, tmp, 2);
  if (fd < 0) {
#ifdef DEBUG
    std::cerr << "Could not read ADC value. ret=" << r << ".\n";
#endif
    throw std::runtime_error("Could not read from i2c.");
  }
  i2cClose(fd);
  //        return (((int)(tmp[0])) << 8) | ((int)(tmp[1]));
  return ((int)(tmp[0]) << 8) | (int)(tmp[1]);
}

/**
 * @brief Handles the event when a sample is available.
 *
 * @param v The sample value.
 */
void ADS1115::hasSample(float v) {
  // Implement the function here
  std::cout << v << std::endl;
}