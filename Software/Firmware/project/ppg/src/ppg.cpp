#include "ppg.hpp"

/**
 * @brief Starts the PPG sensor.
 */
PPG::PPG() {}

/**
 * @brief Starts the PPG sensor.
 * @param ads_ptr Pointer to the ADS1115 object.
 */
void PPG::start(std::unique_ptr<MAX30102>& max30102_ptr) {
  running = true;  // Add this line
  FloatPair data;

  while (running) {
    if (!max30102_ptr->max30102queue.pop(data)) {
      std::this_thread::yield();
    } else {  // Process the received data
      float receivedVal1 = data.value1;
      float receivedVal2 = data.value2;
      std::cout << "Received values: " << receivedVal1 << ", " << receivedVal2
                << std::endl;
    }
  }
}

float PPG::PPG_filtering() {}

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
