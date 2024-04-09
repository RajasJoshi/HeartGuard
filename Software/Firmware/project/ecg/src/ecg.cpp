#include "ecg.hpp"

/**
 * @brief Starts the ECG sensor.
 */
ECG::ECG() {

}

void ECG::start(std::unique_ptr<ADS1115>& ads_ptr) {

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


  std::cout << "ECG sensor started" << std::endl;
  std::cout << "ECG sensor sampling rate: " << SAMPLING_RATE << " Hz" << std::endl;
  float value;

  while (true) {
    if (!ads_ptr->ads115queue.pop(value)) {
      std::this_thread::yield();
    } else {
      float fs = ECG_filtering(notch_filter, lowpass_filter, highpass_filter, value, SAMPLING_RATE);
      
      std::ofstream file("ecg_data.csv", std::ios::app);
      file << "raw, " << value << ", filtered, " << fs << std::endl;
      file.close();

      if (count%int(SAMPLING_RATE) == 0){
        calculate_heart_rate(SAMPLING_RATE);
      }
      if (count%int(2*SAMPLING_RATE) == 0){
        recalculate_mean();
        recalculate_std();
        recalculate_threshold();
      }
      if (count%int(4*SAMPLING_RATE) == 0){
        calculate_RR_interval(SAMPLING_RATE);
      }

      if (count%int(15*SAMPLING_RATE) == 0){
        calculate_hrv();
        empty_values();
      }

      count = count%10000 + 1;
    }

    }
}

void ECG::stop() {
  // Add your code here
}

void ECG::GetECGData() {
  // Add your code here
}

/**
 * @brief Stops the ECG sensor.
 */
ECG::~ECG() {
  // Add your code here
}
