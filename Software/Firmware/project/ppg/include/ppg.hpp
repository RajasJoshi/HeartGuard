#ifndef PPG_HPP
#define PPG_HPP

#include <algorithm>
#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <queue>
#include <thread>

#include "DigitalFilters.h"
#include "max30102.hpp"

class PPG {
 public:
  PPG(void);                                            // Constructor
  ~PPG(void);                                           // Destructor
  void start(std::unique_ptr<MAX30102>& max30102_ptr);  // Start the PPG sensor
  void stop(void);                                      // Stop the PPG sensor

  const int static BPM_BUFFER_SIZE = 100;
  int32_t bpmBuffer[BPM_BUFFER_SIZE];
  int nextBPMBufferIndex = 0;

  const int static SPO2_BUFFER_SIZE = 100;
  int32_t spo2Buffer[SPO2_BUFFER_SIZE];
  int nextspo2BufferIndex = 0;
  int latestIRBPM;
  int averageIRBPM;
  int latestRedSpO2;
  int averageRedSpO2;
  int latestIRValue;
  int latestRedValue;

 private:
  bool running = false;  // Indicates if the PPG sensor is running

  std::chrono::time_point<std::chrono::system_clock> timeLastLoopRan;
  // IR Data
  std::chrono::time_point<std::chrono::system_clock> timeLastIRHeartBeat;
  int32_t irLastValue;

  // For Peak Detection
  int32_t localMaxima;
  int32_t localMinima;
  const static int8_t PAST_PEAKS_SIZE = 2;
  int32_t pastMaximas[PAST_PEAKS_SIZE];
  int32_t pastMinimas[PAST_PEAKS_SIZE];
  bool crest = false;
  bool trough = false;
  uint8_t dataBeenIncreasing = 0;
  uint8_t nextPastPeaksIndex = 0;

  void PPG_filtering(std::unique_ptr<MAX30102>& ppgmax30102_ptr);
  void resetCalculations();
  bool peakDetect(int32_t data);
  void loopThread(std::unique_ptr<MAX30102>& ppgmax30102_ptr);
};

#endif  // PPG_HPP