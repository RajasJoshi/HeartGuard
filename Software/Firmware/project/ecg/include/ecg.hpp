#ifndef ECG_HPP
#define ECG_HPP

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <fstream>
#include <queue>

#include "Iir.h"
#include "ads1115.hpp"

class ECG {
 public:
  ECG();                                          // Constructor
  ~ECG();                                         // Destructor
  void start(std::unique_ptr<ADS1115>& ads_ptr);  // Start the ECG sensor
  void stop(void);                                // Stop the ECG sensor
  void display_buffer(void);                      // Display the buffer
  boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>>
      ecgtcpqueue;

 private:
  // Add your private member variables here
  static const int SAMPLING_RATE = 860.0f;  // Hz
  static const int BUFFER_SIZE =
      SAMPLING_RATE * 4;  // 4 seconds of data at 860 Hz
  float circularBuffer[BUFFER_SIZE];
  int headIndex = 0;  // Index where the next value will be written
  bool bufferFull;

  // infinite impulse response library filter params
  static const int filter_order = 4;  // 4th order filter
  static constexpr float cutoff_frequency =
      50.0f;  // Hz (EMG / Power line removal)
  static constexpr float highpass_cutoff_frequency =
      0.1f;  // Hz (baseline wander removal)
  static constexpr float lowpass_cutoff_frequency =
      100.0f;  // Hz (for noise removal)

  // biological data
  std::vector<int> detected_peaks;     // Store indices of detected R peaks
  float mean = 0.0f;                   // Mean of the ECG signal
  float stdev = 0.0f;                  // Standard deviation of the ECG signal
  float threshold = mean + 3 * stdev;  // Threshold for peak detection
  float RR_interval = 0.0f;            // stores the most recent R_R interval
  float heart_rate;                    // stores the most recent heart rate
  std::vector<float>
      RR_intervals;  // Stores each RR interval for HRV calculation
  std::vector<float>
      HRV;  // Stores the hrv scores for calcualtion every 15 seconds

  void recalculate_mean(void);
  void recalculate_stdev(void);
  void recalculate_threshold(void);
  void calculate_RR_interval_hr(float SAMPLING_RATE);
  float ECG_filtering(Iir::RBJ::IIRNotch& notch_filter,
                      Iir::Butterworth::LowPass<filter_order>& lowpass_filter,
                      Iir::Butterworth::HighPass<filter_order>& highpass_filter,
                      float sample, float SAMPLING_RATE);
  void calculate_hrv(void);
  void empty_values(void);
};

#endif  // ECG_HPP