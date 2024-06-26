#include "ads1115.hpp"

/**
 * @brief Starts the ADS1115 sensor.
 */
void ADS1115::start() {
  ADS1115settings settings;
  settings.samplingRate = ADS1115settings::FS860HZ;
  ads1115settings = settings;

  char gpioFilename[20];
  snprintf(gpioFilename, 19, "/dev/i2c-%d", settings.i2c_bus);
  fdDRDY = open(gpioFilename, O_RDWR);
  if (fdDRDY < 0) {
    char i2copen[] = "Could not open I2C.\n";

#ifdef DEBUG
    fprintf(stderr, i2open);
#endif
    throw i2copen;
  }

  if (ioctl(fdDRDY, I2C_SLAVE, settings.address) < 0) {
    char i2cslave[] = "Could not access I2C adress.\n";

#ifdef DEBUG
    fprintf(stderr, i2cslave);
#endif
    throw i2cslave;
  }

#ifdef DEBUG
  fprintf(stderr, "Init.\n");
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

  chipDRDY = gpiod_chip_open_by_number(settings.drdy_chip);
  lineDRDY = gpiod_chip_get_line(chipDRDY, settings.drdy_gpio);

  int ret = gpiod_line_request_rising_edge_events(lineDRDY, "Consumer");
  if (ret < 0) {
#ifdef DEBUG
    perror("Request event notification failed.\n");
#endif
    throw "Could not request event for IRQ.";
  }

  running = true;

  thr = std::thread(&ADS1115::worker, this);
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
  if (!running) return;
  running = false;
  thr.join();
  gpiod_line_release(lineDRDY);
  gpiod_chip_close(chipDRDY);
  close(fdDRDY);
}

/**
 * @brief Writes a word to the I2C bus.
 *
 * @param reg The register to write to.
 * @param data The data to write.
 */
void ADS1115::i2c_writeWord(uint8_t reg, unsigned data) {
  uint8_t tmp[3];
  tmp[0] = reg;
  tmp[1] = (char)((data & 0xff00) >> 8);
  tmp[2] = (char)(data & 0x00ff);
  long int r = write(fdDRDY, &tmp, 3);
  if (r < 0) {
#ifdef DEBUG
    fprintf(stderr, "Could not write word from %02x.ret=%d.\n",
            ads1115settings.address, r);
#endif
    throw "Could not write to i2c.";
  }
}

/**
 * @brief Reads a word from the I2C bus.
 *
 * @param reg The register to read from.
 * @return unsigned The data read.
 */
unsigned ADS1115::i2c_readWord(uint8_t reg) {
  uint8_t tmp[2];
  tmp[0] = reg;
  write(fdDRDY, &tmp, 1);
  long int r = read(fdDRDY, tmp, 2);
  if (r < 0) {
#ifdef DEBUG
    std::cerr << "Could not read word from " << std::hex
              << ads1115settings.address << ". ret=" << r << ".\n";
#endif
    throw std::runtime_error("Could not read from i2c.");
  }
  return (((unsigned)(tmp[0])) << 8) | ((unsigned)(tmp[1]));
}

/**
 * @brief Reads the conversion result from the I2C bus.
 *
 * @return int The conversion result.
 */
int ADS1115::i2c_readConversion() {
  const int reg = 0;
  char tmp[3];
  tmp[0] = reg;
  write(fdDRDY, &tmp, 1);
  long int r = read(fdDRDY, tmp, 2);
  if (r < 0) {
#ifdef DEBUG
    std::cerr << "Could not read ADC value. ret=" << r << ".\n";
#endif
    throw std::runtime_error("Could not read from i2c.");
  }
  return ((int)(tmp[0]) << 8) | (int)(tmp[1]);
}

/**
 * @brief Handles the event when a sample is available.
 *
 * @param v The sample value.
 */
void ADS1115::hasSample(float v) {
  while (!ads115queue.push(v)) {
    std::this_thread::yield();  // Yield if queue is full
  }
}
