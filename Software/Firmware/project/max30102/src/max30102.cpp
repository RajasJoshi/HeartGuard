#include "max30102.hpp"

MAX30102::MAX30102() {
  // Constructor
}

/**
 * Initializes sensor.
 * Returns negative number on failure.
 * Returns sensor revision on success.
 */
int MAX30102::begin(uint8_t i2cAddr) {
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

  int result = gpioSetMode(INTERRUPT_PIN, PI_INPUT);
  if (result < 0) {
    throw std::runtime_error("Failed to set GPIO mode, error " +
                             std::to_string(result));
  }
  result =
      gpioSetISRFuncEx(INTERRUPT_PIN, RISING_EDGE, 1000, gpioISR, (void*)this);
  if (result < 0) {
    throw std::runtime_error("Failed to set GPIO ISR function, error " +
                             std::to_string(result));
  }

  _i2c = i2cOpen(1, i2cAddr, 0);
  if (_i2c < 0) {
    // Failed to open the I2C bus
    return -1;
  }

  _i2caddr = i2cAddr;

  // Check if part id matches.
  if (readPartID() != MAX30102_EXPECTEDPARTID) {
    return -3;
  }

  return i2cReadByteData(_i2c, REG_REVISIONID);
}

void MAX30102::enableAFULL(void) {
  bitMask(REG_INTENABLE1, MASK_INT_A_FULL, INT_A_FULL_ENABLE);
}
void MAX30102::disableAFULL(void) {
  bitMask(REG_INTENABLE1, MASK_INT_A_FULL, INT_A_FULL_DISABLE);
}

void MAX30102::enableDATARDY(void) {
  bitMask(REG_INTENABLE1, MASK_INT_DATA_RDY, INT_DATA_RDY_ENABLE);
}
void MAX30102::disableDATARDY(void) {
  bitMask(REG_INTENABLE1, MASK_INT_DATA_RDY, INT_DATA_RDY_DISABLE);
}

// Mode configuration //

/**
 * Pull sensor out of low power mode.
 */
void MAX30102::wakeUp(void) { bitMask(REG_MODECONFIG, MASK_SHUTDOWN, WAKEUP); }

/**
 * Put sensor into low power mode.
 * During this mode the sensor will continue to respond to I2C commands
 * but will not update or take new readings, such as temperature.
 */
void MAX30102::shutDown(void) {
  bitMask(REG_MODECONFIG, MASK_SHUTDOWN, SHUTDOWN);
}

/**
 * All configuration, threshold, and data registers are reset
 * to their power-on state through a power-on reset.
 * The reset bit is cleared back to zero after reset finishes.
 */
void MAX30102::softReset(void) {
  bitMask(REG_MODECONFIG, MASK_RESET, RESET);

  // Poll for bit to clear, reset is then complete
  // Timeout after 100ms
  auto startTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point endTime;
  do {
    uint8_t response = i2cReadByteData(_i2c, REG_MODECONFIG);
    if ((response & RESET) == 0) break;  // Done reset!
    usleep(1);                           // Prevent over burden the I2C bus
    endTime = std::chrono::system_clock::now();
  } while ((std::chrono::duration_cast<std::chrono::milliseconds>(endTime -
                                                                  startTime)
                .count()) < 100);
}

/**
 * Sets which LEDs are used for sampling.
 * - Red only
 * - Red+IR only
 * - Custom
 */
void MAX30102::setLEDMode(uint8_t mode) {
  bitMask(REG_MODECONFIG, MASK_LEDMODE, mode);
}

/**
 * Sets ADC Range.
 * Available ADC Range: 2048, 4096, 8192, 16384
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
  i2cWriteByteData(_i2c, REG_LED1_PULSEAMP, amplitude);
}

/**
 * Sets IR LED Pulse Amplitude.
 */
void MAX30102::setPulseAmplitudeIR(uint8_t amplitude) {
  i2cWriteByteData(_i2c, REG_LED2_PULSEAMP, amplitude);
}

void MAX30102::setPulseAmplitudeProximity(uint8_t amplitude) {
  i2cWriteByteData(_i2c, REG_LED_PROX_AMP, amplitude);
}

/**
 * Set the IR ADC count that will trigger the beginning of particle-sensing
 * mode. The threshMSB signifies only the 8 most significant-bits of the ADC
 * count.
 */
void MAX30102::setProximityThreshold(uint8_t threshMSB) {
  i2cWriteByteData(_i2c, REG_PROXINTTHRESH, threshMSB);
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
  i2cWriteByteData(_i2c, REG_MULTILEDCONFIG1, 0);
  i2cWriteByteData(_i2c, REG_MULTILEDCONFIG2, 0);
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
  i2cWriteByteData(_i2c, REG_FIFOWRITEPTR, 0);
  i2cWriteByteData(_i2c, REG_FIFOOVERFLOW, 0);
  i2cWriteByteData(_i2c, REG_FIFOREADPTR, 0);
}

/**
 * Enable roll over if FIFO over flows.
 */
void MAX30102::enableFIFORollover(void) {
  bitMask(REG_FIFOCONFIG, MASK_ROLLOVER, ROLLOVER_ENABLE);
}

/**
 * Read the FIFO Write Pointer.
 */
uint8_t MAX30102::getWritePointer(void) {
  return (i2cReadByteData(_i2c, REG_FIFOWRITEPTR));
}

/**
 * Read the FIFO Read Pointer.
 */
uint8_t MAX30102::getReadPointer(void) {
  return (i2cReadByteData(_i2c, REG_FIFOREADPTR));
}

// Device ID and Revision //

uint8_t MAX30102::readPartID() { return i2cReadByteData(_i2c, REG_PARTID); }

void MAX30102::readRevisionID() {
  revisionID = i2cReadByteData(_i2c, REG_REVISIONID);
}

uint8_t MAX30102::getRevisionID() { return revisionID; }

// Setup the Sensor
void MAX30102::setup(uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode,
                     int sampleRate, int pulseWidth, int adcRange) {
  // Reset all configuration, threshold, and data registers to POR values
  softReset();

  // FIFO Configuration //
  enableAFULL();    // Almost Full Flag
  enableDATARDY();  // New FIFO Data Ready

  // The chip will average multiple samples of same type together if you wish
  if (sampleAverage == 1)
    setFIFOAverage(SAMPLEAVG_1);
  else if (sampleAverage == 2)
    setFIFOAverage(SAMPLEAVG_2);
  else if (sampleAverage == 4)
    setFIFOAverage(SAMPLEAVG_4);
  else if (sampleAverage == 8)
    setFIFOAverage(SAMPLEAVG_8);
  else if (sampleAverage == 16)
    setFIFOAverage(SAMPLEAVG_16);
  else if (sampleAverage == 32)
    setFIFOAverage(SAMPLEAVG_32);
  else
    setFIFOAverage(SAMPLEAVG_4);

  // Allow FIFO to wrap/roll over
  enableFIFORollover();

  // Mode Configuration //
  if (ledMode == 3)
    setLEDMode(LEDMODE_MULTILED);  // Watch all three led channels [TODO] there
                                   // are only 2!
  else if (ledMode == 2)
    setLEDMode(LEDMODE_REDIRONLY);
  else
    setLEDMode(LEDMODE_REDONLY);
  activeLEDs =
      ledMode;  // used to control how many bytes to read from FIFO buffer

  // Particle Sensing Configuration //
  if (adcRange < 4096)
    setADCRange(ADCRANGE_2048);
  else if (adcRange < 8192)
    setADCRange(ADCRANGE_4096);
  else if (adcRange < 16384)
    setADCRange(ADCRANGE_8192);
  else if (adcRange == 16384)
    setADCRange(ADCRANGE_16384);
  else
    setADCRange(ADCRANGE_2048);

  if (sampleRate < 100)
    setSampleRate(SAMPLERATE_50);
  else if (sampleRate < 200)
    setSampleRate(SAMPLERATE_100);
  else if (sampleRate < 400)
    setSampleRate(SAMPLERATE_200);
  else if (sampleRate < 800)
    setSampleRate(SAMPLERATE_400);
  else if (sampleRate < 1000)
    setSampleRate(SAMPLERATE_800);
  else if (sampleRate < 1600)
    setSampleRate(SAMPLERATE_1000);
  else if (sampleRate < 3200)
    setSampleRate(SAMPLERATE_1600);
  else if (sampleRate == 3200)
    setSampleRate(SAMPLERATE_3200);
  else
    setSampleRate(SAMPLERATE_50);

  if (pulseWidth < 118)
    setPulseWidth(PULSEWIDTH_69);  // 15 bit resolution
  else if (pulseWidth < 215)
    setPulseWidth(PULSEWIDTH_118);  // 16 bit resolution
  else if (pulseWidth < 411)
    setPulseWidth(PULSEWIDTH_215);  // 17 bit resolution
  else if (pulseWidth == 411)
    setPulseWidth(PULSEWIDTH_411);  // 18 bit resolution
  else
    setPulseWidth(PULSEWIDTH_69);

  // LED Pulse Amplitude Configuration //
  setPulseAmplitudeRed(powerLevel);
  setPulseAmplitudeIR(powerLevel);
  setPulseAmplitudeProximity(powerLevel);

  // Multi-LED Mode Configuration //
  // Enable the readings of the three LEDs [TODO] only 2!
  enableSlot(1, SLOT_RED_LED);
  if (ledMode > 1) enableSlot(2, SLOT_IR_LED);

  // Reset the FIFO before we begin checking the sensor.
  clearFIFO();
}

// Data Collection //

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
        // If toGet is 32, this is bad because we read 6 bytes (RED+IR * 3 = 6)
        // at a time. 32 % 6 = 2 left over. We don't want to request 32 bytes,
        // we want to request 30. 32 % 9 (Red+IR+GREEN) = 5 left over. We want
        // to request 27.

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
  uint8_t originalContents = i2cReadByteData(_i2c, reg);

  // Zero-out portions of the register based on mask
  originalContents = originalContents & mask;

  // Change contents of register
  i2cWriteByteData(_i2c, reg, originalContents | thing);
}

std::vector<uint8_t> MAX30102::readMany(uint8_t address, uint8_t length) {
  char* rawRead = new char[length];
  int bytesRead = i2cReadI2CBlockData(_i2c, address, rawRead, length);
  std::vector<uint8_t> result;
  if (bytesRead >= 0) {
    for (int i = 0; i < bytesRead; i++) {
      result.push_back(static_cast<uint8_t>(rawRead[i]));
    }
  }
  delete[] rawRead;
  return result;
}

MAX30102::~MAX30102() {
  // Destructor
  i2cClose(_i2c);
  gpioTerminate();
}
