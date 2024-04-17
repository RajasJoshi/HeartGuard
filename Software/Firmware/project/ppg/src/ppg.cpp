#include "ppg.hpp"

/**
 * @brief Starts the PPG sensor.
 */
PPG::PPG() {}

/**
 * @brief Starts the PPG sensor.
 * @param max30102_ptr Pointer to the MAX30102 object.
 */
void PPG::start(std::unique_ptr<MAX30102>& max30102_ptr) {
  if (running) return;
  running = true;

  irLastValue = -999;
  // Init local maxima/minima for peak detection
  localMaxima = -9999;
  localMinima = 9999;
  // Get current time.
  auto timeCurrent = std::chrono::system_clock::now();
  // Init last heartbeat times.
  timeLastIRHeartBeat = timeCurrent;

  std::thread t1(&PPG::loopThread, this,
                 std::ref(max30102_ptr));  // Pass by reference
  t1.detach();
}

void PPG::loopThread(
    std::unique_ptr<MAX30102>& ppgmax30102_ptr) {  // Receive by reference
  while (running) {
    PPG_filtering(ppgmax30102_ptr);  // Pass by reference again

    std::this_thread::sleep_for(
        std::chrono::milliseconds(1000));  // sleep for 100 milliseconds
  }
}

LowPassFilter lpf(0.08, M_PI);
HighPassFilter hpf(0.08, M_PI);
/**
 * @brief Filters the PPG data.
 * @param ppgmax30102_ptr Pointer to the MAX30102 object.
 */
void PPG::PPG_filtering(std::unique_ptr<MAX30102>& ppgmax30102_ptr) {
  auto timeCurrent = std::chrono::system_clock::now();
  latestIRValue = ppgmax30102_ptr->getIR();
  latestRedValue = ppgmax30102_ptr->getRed();

  // Let's get the number of miliseconds passed since we last ran the loop.
  int loopDelta = std::chrono::duration_cast<std::chrono::milliseconds>(
                      timeCurrent - timeLastLoopRan)
                      .count();
  // We're finished with timeLastLoopRan, so let's update it's value for next
  // time.
  timeLastLoopRan = timeCurrent;

  // Check whether finger is on sensor->
  if (latestIRValue < 100000) {
    // Finger is not on sensor->
    // Clear all calculations and exit out of loop.
    resetCalculations();
    return;
  }

  // Calculate the IR heart rate //

  int32_t filteredIRValue = static_cast<int32_t>(latestIRValue);
  int32_t filteredRedValue = static_cast<int32_t>(latestRedValue);
  filteredIRValue = lpf.update(filteredIRValue);
  filteredIRValue = hpf.update(filteredIRValue);
  filteredRedValue = lpf.update(filteredRedValue);
  filteredRedValue = hpf.update(filteredRedValue);

  int timeSinceLastIRHeartBeat =
      std::chrono::duration_cast<std::chrono::milliseconds>(timeCurrent -
                                                            timeLastIRHeartBeat)
          .count();

  if (peakDetect(filteredIRValue)) {
    int _irBPM = 60000 / timeSinceLastIRHeartBeat;
    latestIRBPM = _irBPM;
    bpmBuffer[nextBPMBufferIndex++] = _irBPM;
    if (nextBPMBufferIndex >= BPM_BUFFER_SIZE) nextBPMBufferIndex = 0;
    // Calculate SpO2
    float ratioOfRatios =
        (filteredRedValue / filteredIRValue) / 0.1047;  // Calibration may vary

    int spo2 = 104 - 17 * ratioOfRatios;
    latestRedSpO2 = spo2;
    spo2Buffer[nextspo2BufferIndex++] = spo2;
    if (nextspo2BufferIndex >= BPM_BUFFER_SIZE) nextspo2BufferIndex = 0;

    // Update timeLastIRHeartBeat for next time.
    timeLastIRHeartBeat = timeCurrent;
  }

  // We're finished with irLastValue, so let's update their value for next time.
  irLastValue = filteredIRValue;
}

/**
 * @brief Detects peaks in heart data.
 * @param data The data to be analyzed.
 * @return True if the data is a peak, false otherwise.
 */
bool PPG::peakDetect(int32_t data) {
  if (irLastValue == -999) {
    // This is first time peakDetect is called.
    return false;
  }
  if (crest && trough && data > irLastValue) {
    dataBeenIncreasing++;
    if (dataBeenIncreasing >= 2) {
      // This is a beat.
      // Add local maxima & minima to past
      pastMaximas[nextPastPeaksIndex] = localMaxima;
      pastMinimas[nextPastPeaksIndex] = localMinima;
      nextPastPeaksIndex++;
      if (nextPastPeaksIndex >= PPG::PAST_PEAKS_SIZE) {
        nextPastPeaksIndex = 0;
      }
      // Reset values
      crest = trough = false;
      dataBeenIncreasing = 0;
      localMaxima = -9999;
      localMinima = 9999;

      return true;
    }
  }

  if (data > localMaxima) {
    localMaxima = data;
    if (data > 100) {
      crest = true;
    }
  }
  if (crest && data < localMinima) {
    localMinima = data;
    if (crest && data < -100) {
      trough = true;
    }
  }
  return false;
}

/**
 * @brief Resets the calculations for the PPG sensor.
 */
void PPG::resetCalculations() {
  // Clear stored heart rates.
  latestIRBPM = 0;

  // Reset peak detection.
  crest = trough = false;
  nextPastPeaksIndex = 0;
  dataBeenIncreasing = 0;
  localMaxima = -9999;
  localMinima = 9999;

  // Reset bpm buffer
  for (int i = 0; i < BPM_BUFFER_SIZE; i++) {
    bpmBuffer[i] = 0;
  }

  for (int i = 0; i < SPO2_BUFFER_SIZE; i++) {
    spo2Buffer[i] = 0;
  }

  // Reset last values.
  irLastValue = -999;

  // Get current time.
  auto timeCurrent = std::chrono::system_clock::now();
  // Reset last heartbeat times.
  timeLastIRHeartBeat = timeCurrent;
}

/**
 * @brief Stops the PPG sensor.
 */
void PPG::stop() {
  running = false;
  std::cout << "Stopping PPG sensor..." << std::endl;
}

/**
 * @brief Destructor for the PPG sensor.
 */
PPG::~PPG() {
  stop();
  std::cout << "PPG sensor stopped" << std::endl;
}
