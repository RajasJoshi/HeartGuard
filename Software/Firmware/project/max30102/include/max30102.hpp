#ifndef MAX30102_HPP
#define MAX30102_HPP

/**
 * @file max30102.hpp
 * @brief MAX30102 class for reading data from the MAX30102 sensor.
 */

// Include any necessary headers here
#include <fcntl.h>
#include <pigpio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

// Define any constants here
#define MAX30102_ADDRESS 0x57
#define I2C_BUFFER_LENGTH 32
#define INTERRUPT_PIN 17  // Change this to the GPIO pin number you are using

// Status Registers
static const uint8_t REG_INTSTAT1 = 0x00;
static const uint8_t REG_INTSTAT2 = 0x01;
static const uint8_t REG_INTENABLE1 = 0x02;
static const uint8_t REG_INTENABLE2 = 0x03;

// FIFO Registers
static const uint8_t REG_FIFOWRITEPTR = 0x04;
static const uint8_t REG_FIFOOVERFLOW = 0x05;
static const uint8_t REG_FIFOREADPTR = 0x06;
static const uint8_t REG_FIFODATA = 0x07;

// Configuration Registers
static const uint8_t REG_FIFOCONFIG = 0x08;
static const uint8_t REG_MODECONFIG = 0x09;
static const uint8_t REG_PARTICLECONFIG = 0x0A;
static const uint8_t REG_LED1_PULSEAMP = 0x0C;
static const uint8_t REG_LED2_PULSEAMP = 0x0D;
static const uint8_t REG_LED_PROX_AMP = 0x10;
static const uint8_t REG_MULTILEDCONFIG1 = 0x11;
static const uint8_t REG_MULTILEDCONFIG2 = 0x12;

// Die Temperature Registers
static const uint8_t REG_DIETEMPINT = 0x1F;
static const uint8_t REG_DIETEMPFRAC = 0x20;
static const uint8_t REG_DIETEMPCONFIG = 0x21;

// Proximity Function Registers
static const uint8_t REG_PROXINTTHRESH = 0x30;

// IDs
static const uint8_t REG_REVISIONID = 0xFE;
static const uint8_t REG_PARTID = 0xFF;
static const uint8_t MAX30102_EXPECTEDPARTID = 0x15;

// Interrupt Configuration
static const uint8_t MASK_INT_A_FULL = (uint8_t)~0b10000000;
static const uint8_t INT_A_FULL_ENABLE = 0x80;
static const uint8_t INT_A_FULL_DISABLE = 0x00;

static const uint8_t MASK_INT_DATA_RDY = (uint8_t)~0b01000000;
static const uint8_t INT_DATA_RDY_ENABLE = 0x40;
static const uint8_t INT_DATA_RDY_DISABLE = 0x00;

static const uint8_t MASK_INT_ALC_OVF = (uint8_t)~0b00100000;
static const uint8_t INT_ALC_OVF_ENABLE = 0x20;
static const uint8_t INT_ALC_OVF_DISABLE = 0x00;

static const uint8_t MASK_INT_PROX_INT = (uint8_t)~0b00010000;
static const uint8_t INT_PROX_INT_ENABLE = 0x10;
static const uint8_t INT_PROX_INT_DISABLE = 0x00;

static const uint8_t MASK_INT_DIE_TEMP_RDY = (uint8_t)~0b00000010;
static const uint8_t INT_DIE_TEMP_RDY_ENABLE = 0x02;
static const uint8_t INT_DIE_TEMP_RDY_DISABLE = 0x00;

static const uint8_t MASK_SAMPLEAVG = (uint8_t)~0b11100000;
static const uint8_t SAMPLEAVG_1 = 0x00;
static const uint8_t SAMPLEAVG_2 = 0x20;
static const uint8_t SAMPLEAVG_4 = 0x40;
static const uint8_t SAMPLEAVG_8 = 0x60;
static const uint8_t SAMPLEAVG_16 = 0x80;
static const uint8_t SAMPLEAVG_32 = 0xA0;

static const uint8_t MASK_ROLLOVER = 0xEF;
static const uint8_t ROLLOVER_ENABLE = 0x10;
static const uint8_t ROLLOVER_DISABLE = 0x00;

static const uint8_t MASK_A_FULL = 0xF0;

// Mode configuration commands
static const uint8_t MASK_SHUTDOWN = 0x7f;
static const uint8_t SHUTDOWN = 0x80;
static const uint8_t WAKEUP = 0x00;
static const uint8_t MASK_RESET = 0xBF;
static const uint8_t RESET = 0X40;
/// IR led mode
static const uint8_t MASK_LEDMODE = 0xF8;
static const uint8_t LEDMODE_REDONLY = 0x02;
static const uint8_t LEDMODE_REDIRONLY = 0x03;
static const uint8_t LEDMODE_MULTILED = 0x07;

// Particle sensing configuration commands
static const uint8_t MASK_ADCRANGE = 0x9F;
static const uint8_t ADCRANGE_2048 = 0x00;
static const uint8_t ADCRANGE_4096 = 0x20;
static const uint8_t ADCRANGE_8192 = 0x40;
static const uint8_t ADCRANGE_16384 = 0x60;

static const uint8_t MASK_SAMPLERATE = 0xE3;
static const uint8_t SAMPLERATE_50 = 0x00;
static const uint8_t SAMPLERATE_100 = 0x04;
static const uint8_t SAMPLERATE_200 = 0x08;
static const uint8_t SAMPLERATE_400 = 0x0C;
static const uint8_t SAMPLERATE_800 = 0x10;
static const uint8_t SAMPLERATE_1000 = 0x14;
static const uint8_t SAMPLERATE_1600 = 0x18;
static const uint8_t SAMPLERATE_3200 = 0x1C;

static const uint8_t MASK_PULSEWIDTH = 0xFC;
static const uint8_t PULSEWIDTH_69 = 0x00;
static const uint8_t PULSEWIDTH_118 = 0x01;
static const uint8_t PULSEWIDTH_215 = 0x02;
static const uint8_t PULSEWIDTH_411 = 0x03;

// Multi-LED Mode Configuration
static const uint8_t MASK_SLOT1 = 0xF8;
static const uint8_t MASK_SLOT2 = 0x8F;
static const uint8_t MASK_SLOT3 = 0xF8;
static const uint8_t MASK_SLOT4 = 0x8F;

static const uint8_t SLOT_NONE = 0x00;
static const uint8_t SLOT_RED_LED = 0x01;
static const uint8_t SLOT_IR_LED = 0x02;
static const uint8_t SLOT_NONE_PILOT = 0x04;
static const uint8_t SLOT_RED_PILOT = 0x05;
static const uint8_t SLOT_IR_PILOT = 0x06;

/**
 * @brief MAX30102 class for reading data from the MAX30102 sensor.
 */
class MAX30102 {
 public:
  MAX30102(void);

  int begin(uint8_t i2cAddr = MAX30102_ADDRESS);

  bool safeCheck(uint8_t maxTimeToCheck);  // Given a max amount of time, checks
                                           // for new data.

  // Configuration
  void wakeUp();
  void shutDown();
  void softReset();

  void setLEDMode(uint8_t mode);

  void setADCRange(uint8_t adcRange);
  void setSampleRate(uint8_t sampleRate);
  void setPulseWidth(uint8_t pulseWidth);

  void setPulseAmplitudeRed(uint8_t value);
  void setPulseAmplitudeIR(uint8_t value);
  void setPulseAmplitudeProximity(uint8_t value);

  void setProximityThreshold(uint8_t thresMSB);

  // Multi-LED configuration mode
  void enableSlot(uint8_t slotNumber, uint8_t device);
  void disableSlots(void);

  // Data Collection

  // Interrupts
  uint8_t getINT1(void);
  uint8_t getINT2(void);
  void enableAFULL(void);
  void disableAFULL(void);
  void enableDATARDY(void);
  void disableDATARDY(void);

  // FIFO Configurations
  void setFIFOAverage(uint8_t samples);
  void enableFIFORollover();
  void disableFIFORollover();
  void setFIFOAlmostFull(uint8_t samples);

  // FIFO Reading
  uint16_t check(void);
  uint8_t available(void);
  void nextSample(void);
  uint32_t getFIFORed(void);
  uint32_t getFIFOIR(void);

  uint8_t getWritePointer(void);
  uint8_t getReadPointer(void);
  void clearFIFO(void);

  // Detecting ID/Revision
  uint8_t getRevisionID();
  uint8_t readPartID();

  void setup(uint8_t powerLevel = 0x1F, uint8_t sampleAverage = 4,
             uint8_t ledMode = 2, int sampleRate = 400, int pulseWidth = 411,
             int adcRange = 4096);

 private:
  int _i2c;
  uint8_t _i2caddr;

  uint8_t activeLEDs;

  uint8_t revisionID;

  void readRevisionID();

  void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);

  std::vector<uint8_t> readMany(uint8_t address, uint8_t length);

  static void gpioISR(int, int, uint32_t, void* userdata) {
    ((MAX30102*)userdata)->check();
    std::cout << "IR: " << sense.IR[sense.head];
    std::cout << ", RED: " << sense.red[sense.head];
    std::cout << std::endl;
  }

#define STORAGE_SIZE 4
  typedef struct Record {
    uint32_t red[STORAGE_SIZE];
    uint32_t IR[STORAGE_SIZE];
    uint8_t head;
    uint8_t tail;
  } sense_struct;
  sense_struct sense;
};

#endif  // MAX30102_HPP