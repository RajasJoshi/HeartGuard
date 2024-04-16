#include "ecg.hpp"

/**
 * @brief Starts the ECG sensor.
 */
ECG::ECG() {}

/**
 * @brief Starts the ECG sensor.
 * @param ads_ptr Pointer to the ADS1115 object.
 */
void ECG::start(std::unique_ptr<ADS1115>& ads_ptr,
                std::unique_ptr<PPG>& ppg_ptr) {
  running = true;  // Add this line
  const ADS1115settings& settings = ads_ptr->getADS1115settings();
  const float SAMPLING_RATE = settings.getSamplingRate();
  int count = 0;

  // Notch filter for removing EMG / power line noise
  Iir::RBJ::IIRNotch notch_filter;
  notch_filter.setup(SAMPLING_RATE, cutoff_frequency);

  // Butterworth high-pass filter for baseline wander removal
  Iir::Butterworth::HighPass<filter_order> highpass_filter;
  highpass_filter.setup(SAMPLING_RATE, highpass_cutoff_frequency);

  // Butterworth low-pass filter for noise removal
  Iir::Butterworth::LowPass<filter_order> lowpass_filter;
  lowpass_filter.setup(SAMPLING_RATE, lowpass_cutoff_frequency);

  while (running) {
    float value;
    if (!ads_ptr->ads115queue.pop(value)) {
      std::this_thread::yield();
    } else {
      float fs = ECG_filtering(notch_filter, lowpass_filter, highpass_filter,
                               value, SAMPLING_RATE);
      std::string message = "ecg," + std::to_string(value) + "," +
                            std::to_string(fs) + "," +
                            std::to_string(heart_rate) + "," +
                            std::to_string(ppg_ptr->latestIRBPM) + "," +
                            std::to_string(ppg_ptr->latestRedSpO2) + "," +
                            std::to_string(ppg_ptr->latestIRValue) + "," +
                            std::to_string(ppg_ptr->latestRedValue) + "#";
      while (!ecgtcpqueue.push(message)) {
        std::this_thread::yield();  // Yield if queue is full}
      }

      if (count % int(10 * SAMPLING_RATE) == 0) {
        recalculate_mean();
        recalculate_stdev();
        recalculate_threshold();
        empty_values();
      }

      count = count % 10000 + 1;
    }
  }
}

/**
 * @brief Recalculates the mean of the ECG signal.
 */
void ECG::recalculate_mean() {
  float sum = 0.0f;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    sum += circularBuffer[i];
  }
  mean = sum / BUFFER_SIZE;
}

/**
 * @brief Recalculates the standard deviation of the ECG signal.
 */
void ECG::recalculate_stdev() {
  float sum = 0.0f;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    sum += (circularBuffer[i] - mean) * (circularBuffer[i] - mean);
  }
  stdev = std::sqrt(sum / BUFFER_SIZE);
}

/**
 * @brief Recalculates the threshold for peak detection.
 */
void ECG::recalculate_threshold() { threshold = mean + 2 * stdev; }

/**
 * @brief Calculates the RR interval and heart rate.
 * @param SAMPLING_RATE The sampling rate of the ECG signal.
 */
void ECG::calculate_RR_interval_hr(float SAMPLING_RATE) {
  if (detected_peaks.empty()) {
    heart_rate = 0.0f;
  }
  if (detected_peaks.size() > 1) {
    float average_time_between_peaks = 0.0f;
    for (int i = 1; i <= static_cast<int>(detected_peaks.size()); i++) {
      int time_between_peaks =
          (detected_peaks[i] - detected_peaks[i - 1] + BUFFER_SIZE) %
          BUFFER_SIZE;
      average_time_between_peaks += time_between_peaks;
    }
    average_time_between_peaks /= float(detected_peaks.size() - 1);
    RR_interval = average_time_between_peaks / SAMPLING_RATE;
    RR_intervals.push_back(RR_interval);
    heart_rate = 60.0f / (RR_interval);
  }
}

/**
 * @brief Filters the ECG signal.
 * @param notch_filter The notch filter.
 * @param lowpass_filter The low-pass filter.
 * @param highpass_filter The high-pass filter.
 * @param sample The ECG sample.
 * @param SAMPLING_RATE The sampling rate of the ECG signal.
 * @return The filtered ECG sample.
 */
float ECG::ECG_filtering(
    Iir::RBJ::IIRNotch& notch_filter,
    Iir::Butterworth::LowPass<filter_order>& lowpass_filter,
    Iir::Butterworth::HighPass<filter_order>& highpass_filter, float sample,
    float SAMPLING_RATE) {
  float filtered_sample_hp = highpass_filter.filter(sample);
  filtered_sample_hp = lowpass_filter.filter(filtered_sample_hp);
  circularBuffer[headIndex] = notch_filter.filter(filtered_sample_hp);

  if (circularBuffer[headIndex] > threshold &&
      (circularBuffer[headIndex] >
       circularBuffer[(headIndex - 1) % BUFFER_SIZE])) {
    if (detected_peaks.empty() ||
        ((headIndex - detected_peaks.back() + BUFFER_SIZE) % BUFFER_SIZE >
         (0.2 *
          SAMPLING_RATE))) {  // 300 max HR so ~0.2s between peaks minimum#
      detected_peaks.push_back(headIndex);
      if (detected_peaks.size() > 2) {
        detected_peaks.erase(detected_peaks.begin());
        calculate_RR_interval_hr(SAMPLING_RATE);
      }
    }
  }
  int index = headIndex;
  headIndex = (headIndex + 1) % BUFFER_SIZE;
  return circularBuffer[index];
}

/**
 * @brief Calculates the heart rate variability.
 */
void ECG::calculate_hrv() {
  if (!(RR_intervals.size() > 1)) {
    std::cout << "Not enough RR intervals to calculate HRV" << std::endl;
  }
  float sum_sq_diffs = 0.0f;
  for (int i = 1; i < static_cast<int>(RR_intervals.size()); i++) {
    float diff = RR_intervals[i] - RR_intervals[i - 1];
    sum_sq_diffs += diff * diff;
  }

  float mean_sq_diffs = sum_sq_diffs / (RR_intervals.size() - 1);
  float rmssd = std::sqrt(mean_sq_diffs);
  HRV.push_back(rmssd);
}

/**
 * @brief Empties the values in the ECG buffer.
 */
void ECG::empty_values() { RR_intervals.clear(); }

/**
 * @brief Displays the buffer.
 */
void ECG::display_buffer(void) {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int index = (headIndex + i) % BUFFER_SIZE;
  }
}

/**
 * @brief Stops the ECG sensor.
 */
void ECG::stop() {
  running = false;
  std::cout << "Stopping ECG sensor..." << std::endl;
}

/**
 * @brief Destructor for the ECG sensor.
 */
ECG::~ECG() {
  stop();
  std::cout << "ECG sensor stopped" << std::endl;
}
