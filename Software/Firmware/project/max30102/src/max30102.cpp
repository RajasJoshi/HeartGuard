#include "max30102.hpp"

/**
 * @brief Construct a new MAX30102::MAX30102 object
 *
 */
MAX30102::MAX30102() {
  // Constructor
}

/**
 * @brief Construct a new MAX30102::MAX30102 object
 *
 */
MAX30102::~MAX30102() {
  // Destructor
  stop();
}

/**
 * @brief Worker thread that listens for data ready events.
 *
 */
int MAX30102::start() {
  // Open the I2C bus
  char gpioFilename[20];
  snprintf(gpioFilename, 19, "/dev/i2c-%d", settings.i2c_bus);
  fdDRDY = open(gpioFilename, O_RDWR);
  if (fdDRDY < 0) {
    char i2copen[] = "Could not open I2C.\n";
  }

  if (ioctl(fdDRDY, I2C_SLAVE, settings.address) < 0) {
    return -2;
  }

  _i2c = fdDRDY;
  _i2caddr = settings.address;

  // Check if part id matches.
  if (readPartID() != MAX30102_EXPECTEDPARTID) {
    return -3;
  }

  chipDRDY = gpiod_chip_open_by_number(settings.drdy_chip);
  lineDRDY = gpiod_chip_get_line(chipDRDY, settings.drdy_gpio);

  int ret = gpiod_line_request_falling_edge_events(lineDRDY, "Consumer");
  if (ret < 0) {
#ifdef DEBUG
    perror("Request event notification failed.\n");
#endif
    throw "Could not request event for IRQ.";
  }

  running = true;

  thr = std::thread(&MAX30102::worker, this);

  return i2c_smbus_read_byte_data(_i2c, REG_REVISIONID);
}

// INterrupt configuration //

/**
 * Enable the A_FULL interrupt.
 */
void MAX30102::enableAFULL(void) {
  bitMask(REG_INTENABLE1, MASK_INT_A_FULL, INT_A_FULL_ENABLE);
}

/**
 * Disable the A_FULL interrupt.
 */
void MAX30102::disableAFULL(void) {
  bitMask(REG_INTENABLE1, MASK_INT_A_FULL, INT_A_FULL_DISABLE);
}

/**
 * Enable the PPG_RDY interrupt.
 */
void MAX30102::enableDATARDY(void) {
  bitMask(REG_INTENABLE1, MASK_INT_DATA_RDY, INT_DATA_RDY_ENABLE);
}

/**
 * Disable the PPG_RDY interrupt.
 */
void MAX30102::disableDATARDY(void) {
  bitMask(REG_INTENABLE1, MASK_INT_DATA_RDY, INT_DATA_RDY_DISABLE);
}

void MAX30102::enableALCOVF(void) {
  bitMask(REG_INTENABLE1, MASK_INT_ALC_OVF, INT_ALC_OVF_ENABLE);
}
void MAX30102::disableALCOVF(void) {
  bitMask(REG_INTENABLE1, MASK_INT_ALC_OVF, INT_ALC_OVF_DISABLE);
}

// Mode configuration //

/**
 * @brief Put the MAX30102 into Standby Mode.
 */
void MAX30102::shutDown(void) {
  bitMask(REG_MODECONFIG, MASK_SHUTDOWN, SHUTDOWN);
}

/**
 * @brief Wake the MAX30102 from Standby Mode.
 */
void MAX30102::softReset(void) {
  bitMask(REG_MODECONFIG, MASK_RESET, RESET);

  // Timeout after 100ms
  auto startTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point endTime;
  do {
    uint8_t response = i2c_smbus_read_byte_data(_i2c, REG_MODECONFIG);
    if ((response & RESET) == 0) break;  // Done reset!
    endTime = std::chrono::system_clock::now();
  } while ((std::chrono::duration_cast<std::chrono::milliseconds>(endTime -
                                                                  startTime)
                .count()) < 100);
}

/**
 * @brief Put the MAX30102 into Operating Mode.
 * @param mode The mode to set.
 */
void MAX30102::setLEDMode(uint8_t mode) {
  bitMask(REG_MODECONFIG, MASK_LEDMODE, mode);
}

/**
 * @brief Set the ADC Range.
 * @param adcRange The ADC Range to set.
 */
void MAX30102::setADCRange(uint8_t adcRange) {
  bitMask(REG_PARTICLECONFIG, MASK_ADCRANGE, adcRange);
}

/**
 * Sets Sample Rate.
 * Available Sample Rates: 50, 100, 200, 400, 800, 1000, 1600, 3200
 */
void MAX30102::setSampleRate(uint8_t sampleRate) {
  bitMask(REG_PARTICLECONFIG, MASK_SAMPLERATE, sampleRate);
}

/**
 * Sets Pulse Width.
 * Available Pulse Width: 69, 188, 215, 411
 */
void MAX30102::setPulseWidth(uint8_t pulseWidth) {
  bitMask(REG_PARTICLECONFIG, MASK_PULSEWIDTH, pulseWidth);
}

/**
 * Sets Red LED Pulse Amplitude.
 */
void MAX30102::setPulseAmplitudeRed(uint8_t amplitude) {
  i2c_smbus_write_byte_data(_i2c, REG_LED1_PULSEAMP, amplitude);
}

/**
 * Sets IR LED Pulse Amplitude.
 */
void MAX30102::setPulseAmplitudeIR(uint8_t amplitude) {
  i2c_smbus_write_byte_data(_i2c, REG_LED2_PULSEAMP, amplitude);
}

void MAX30102::setPulseAmplitudeProximity(uint8_t amplitude) {
  i2c_smbus_write_byte_data(_i2c, REG_LED_PROX_AMP, amplitude);
}

/**
 * Given a slot number assign a thing to it.
 * Devices are SLOT_RED_LED or SLOT_RED_PILOT (proximity)
 * Assigning a SLOT_RED_LED will pulse LED
 * Assigning a SLOT_RED_PILOT will ??
 */
void MAX30102::enableSlot(uint8_t slotNumber, uint8_t device) {
  switch (slotNumber) {
    case (1):
      bitMask(REG_MULTILEDCONFIG1, MASK_SLOT1, device);
      break;
    case (2):
      bitMask(REG_MULTILEDCONFIG1, MASK_SLOT2, device << 4);
      break;
    case (3):
      bitMask(REG_MULTILEDCONFIG2, MASK_SLOT3, device);
      break;
    case (4):
      bitMask(REG_MULTILEDCONFIG2, MASK_SLOT4, device << 4);
      break;
    default:
      // Shouldn't be here!
      break;
  }
}

/**
 * Clears all slot assignments.
 */
void MAX30102::disableSlots(void) {
  i2c_smbus_write_byte_data(_i2c, REG_MULTILEDCONFIG1, 0);
  i2c_smbus_write_byte_data(_i2c, REG_MULTILEDCONFIG2, 0);
}

// FIFO Configuration //

/**
 * Sets sample average.
 */
void MAX30102::setFIFOAverage(uint8_t numberOfSamples) {
  bitMask(REG_FIFOCONFIG, MASK_SAMPLEAVG, numberOfSamples);
}

/**
 * Resets all points to start in a known state.
 * Recommended to clear FIFO before beginning a read.
 */
void MAX30102::clearFIFO(void) {
  i2c_smbus_write_byte_data(_i2c, REG_FIFOWRITEPTR, 0);
  i2c_smbus_write_byte_data(_i2c, REG_FIFOOVERFLOW, 0);
  i2c_smbus_write_byte_data(_i2c, REG_FIFOREADPTR, 0);
}

/**
 * Enable roll over if FIFO over flows.
 */
void MAX30102::enableFIFORollover(void) {
  bitMask(REG_FIFOCONFIG, MASK_ROLLOVER, ROLLOVER_ENABLE);
}

/**
 * Disable roll over if FIFO over flows.
 */
void MAX30102::disableFIFORollover(void) {
  bitMask(REG_FIFOCONFIG, MASK_ROLLOVER, ROLLOVER_DISABLE);
}

/**
 * Read the FIFO Write Pointer.
 */
uint8_t MAX30102::getWritePointer(void) {
  return (i2c_smbus_read_byte_data(_i2c, REG_FIFOWRITEPTR));
}

/**
 * Read the FIFO Read Pointer.
 */
uint8_t MAX30102::getReadPointer(void) {
  return (i2c_smbus_read_byte_data(_i2c, REG_FIFOREADPTR));
}

// Device ID and Revision //

uint8_t MAX30102::readPartID() {
  return i2c_smbus_read_byte_data(_i2c, REG_PARTID);
}

// Setup the Sensor
void MAX30102::setup() {
  // Reset all configuration, threshold, and data registers to POR values
  softReset();

  // The chip will average multiple samples of same type together if you wish
  setFIFOAverage(settings.numberOfSamples);

  // Allow FIFO to wrap/roll over

  enableFIFORollover();

  // Set interrupt mode into FIFO almost full flag
  enableAFULL();
  enableALCOVF();
  enableDATARDY();

  // Mode Configuration //
  setLEDMode(settings.ledMode);

  activeLEDs = 2;  // used to control how many bytes to read from FIFO buffer

  // Particle Sensing Configuration //
  setADCRange(settings.adcRange);
  setSampleRate(settings.sampleRate);
  setPulseWidth(settings.pulseWidth);  // 18 bit resolution

  // LED Pulse Amplitude Configuration //
  setPulseAmplitudeRed(settings.redPulseAmplitude);
  setPulseAmplitudeIR(settings.redPulseAmplitude);
  setPulseAmplitudeProximity(settings.redPulseAmplitude);

  // Multi-LED Mode Configuration //
  enableSlot(1, SLOT_RED_LED);
  enableSlot(2, SLOT_IR_LED);

  // Reset the FIFO before we begin checking the sensor.
  clearFIFO();
}

// Data Collection //

/**
 * Report the most recent Red value.
 */
uint32_t MAX30102::getRed(void) { return (sense.red[sense.head]); }

/**
 * Report the most recent IR value.
 */
uint32_t MAX30102::getIR(void) { return (sense.IR[sense.head]); }

/**
 * Report the next Red value in FIFO.
 */
uint32_t MAX30102::getFIFORed(void) { return (sense.red[sense.tail]); }

/**
 * Report the next IR value in FIFO.
 */
uint32_t MAX30102::getFIFOIR(void) { return (sense.IR[sense.tail]); }

uint16_t MAX30102::check(void) {
  uint8_t readPointer = getReadPointer();
  uint8_t writePointer = getWritePointer();

  int numberOfSamples = 0;

  // Do we have new data?
  if (readPointer != writePointer) {
    // Calculate the number of readings we need to get from sensor
    numberOfSamples = writePointer - readPointer;
    // std::cout << "There are " << (int)numberOfSamples << " available
    // samples." << std::endl;
    if (numberOfSamples < 0) numberOfSamples += 32;

    // We know have the number of readings, now calculate bytes to read.
    // For this example we are just doing Red and IR (3 bytes each)
    int bytesLeftToRead = numberOfSamples * activeLEDs * 3;

    // We may need to read as many as 288 bytes so we read in blocks no larger
    // than I2C_BUFFER_LENGTH
    while (bytesLeftToRead > 0) {
      int toGet = bytesLeftToRead;
      if (toGet > I2C_BUFFER_LENGTH) {
        // If toGet is 32, this is bad because we read 6 bytes (RED+IR * 3 =
        // 6) at a time. 32 % 6 = 2 left over. We don't want to request 32
        // bytes, we want to request 30. 32 % 9 (Red+IR+GREEN) = 5 left over.
        // We want to request 27.

        // Trim toGet to be a multiple of the samples we need to read.
        toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % (activeLEDs * 3));
      }

      bytesLeftToRead -= toGet;

      // Request toGet number of bytes from sensor
      std::vector<uint8_t> dataReceived = readMany(REG_FIFODATA, toGet);
      int index = 0;

      while (toGet > 0) {
        sense.head++;  // Advance the head of the storage struct
        sense.head %= STORAGE_SIZE;

        uint8_t temp[sizeof(
            uint32_t)];  // Array of 4 bytes that we will convert into long
        uint32_t tempLong;

        // Burst read three bytes - RED
        temp[3] = 0;
        temp[2] = dataReceived[index++];
        temp[1] = dataReceived[index++];
        temp[0] = dataReceived[index++];

        // Convert array to long
        std::memcpy(&tempLong, temp, sizeof(tempLong));

        // Zero out all but 18 bits
        tempLong &= 0x3FFFF;

        // Store this reading into the sense array
        sense.red[sense.head] = tempLong;

        if (activeLEDs > 1) {
          // Burst read three more bytes - IR
          temp[3] = 0;
          temp[2] = dataReceived[index++];
          temp[1] = dataReceived[index++];
          temp[0] = dataReceived[index++];

          // Convert array to long
          std::memcpy(&tempLong, temp, sizeof(tempLong));

          // Zero out all 18 bits
          tempLong &= 0x3FFFF;

          sense.IR[sense.head] = tempLong;
        }

        toGet -= activeLEDs * 3;
      }
    }
  }
  return (numberOfSamples);
}

/**
 * Set certain thing in register.
 */
void MAX30102::bitMask(uint8_t reg, uint8_t mask, uint8_t thing) {
  // Read register
  uint8_t originalContents = i2c_smbus_read_byte_data(_i2c, reg);

  // Zero-out portions of the register based on mask
  originalContents = originalContents & mask;

  // Change contents of register
  i2c_smbus_write_byte_data(_i2c, reg, originalContents | thing);
}

/**
 * Read multiple bytes from register.
 */
std::vector<uint8_t> MAX30102::readMany(uint8_t address, uint8_t length) {
  ioctl(_i2c, I2C_SLAVE, _i2caddr);
  uint8_t *rawRead = new uint8_t[length];
  i2c_smbus_read_i2c_block_data(_i2c, address, length, rawRead);
  std::vector<uint8_t> result;
  for (uint8_t i = 0; i < length; i++) {
    result.push_back(rawRead[i]);
  }
  return result;
}

/**
 * @brief Handles the event when data is ready.
 */
void MAX30102::dataReady() { check(); }

void MAX30102::stop() {
  if (!running) return;
  running = false;
  thr.join();
  gpiod_line_release(lineDRDY);
  gpiod_chip_close(chipDRDY);
  close(fdDRDY);
}
