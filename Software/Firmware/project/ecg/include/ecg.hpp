#ifndef ECG_HPP
#define ECG_HPP

#include "ads1115.hpp"
#include <queue>
#include <algorithm>
#include "Iir.h"
#include <fstream>


class ECG {
  public:
    ECG();                                          // Constructor
    ~ECG();                                         // Destructor
    void start(std::unique_ptr<ADS1115>& ads_ptr);  // Start the ECG sensor
    void stop(void);                                // Stop the ECG sensor


    // Add your class methods and member variables here
    void GetECGData(void);

    void display_buffer(void) {
      for (int i = 0; i < BUFFER_SIZE; i++) {
        int index = (headIndex + i) % BUFFER_SIZE;
        std::cout << circularBuffer[index] << " ";
      }
      std::cout << std::endl;
    }



  private:
    // Add your private member variables here
    static const int SAMPLING_RATE = 860.0f; // Hz
    static const int BUFFER_SIZE = SAMPLING_RATE*4; // 4 seconds of data at 860 Hz
    float circularBuffer[BUFFER_SIZE];
    int headIndex = 0;  // Index where the next value will be written
    bool bufferFull;

      // infinite impulse response library filter params
    static const int filter_order = 4;                     // 4th order filter
    static constexpr float cutoff_frequency = 50.0f;           // Hz (EMG / Power line removal)
    static constexpr float highpass_cutoff_frequency = 0.1f;   // Hz (baseline wander removal)
    static constexpr float lowpass_cutoff_frequency = 100.0f;  // Hz (for noise removal)

    // biological data
    std::vector<int> detected_peaks;    // Store indices of detected R peaks
    float mean = 0.0f;                     // Mean of the ECG signal
    float stdev = 0.0f;                      // Standard deviation of the ECG signal
    float threshold = mean + 3*stdev;     // Threshold for peak detection
    float RR_interval = 0.0f;           // stores the most recent R_R interval
    float heart_rate;                   // stores the most recent heart rate  
    std::vector<float> RR_intervals;    // Stores each RR interval for HRV calculation
    std::vector<float> HRV;             // Stores the hrv scores for calcualtion every 15 seconds
    
    void recalculate_mean() {
      float sum = 0.0f;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        sum += circularBuffer[i];
      }
      mean = sum / BUFFER_SIZE;
    }

    void recalculate_stdev() {
      float sum = 0.0f;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        sum += (circularBuffer[i] - mean) * (circularBuffer[i] - mean);
      }
      stdev = std::sqrt(sum / BUFFER_SIZE);
    }

    void recalculate_threshold() {
      threshold = mean + 2*stdev;
    }

    void calculate_RR_interval_hr(float SAMPLING_RATE) {
      if (detected_peaks.empty()) {
          heart_rate = 0.0f;
        }
      if (detected_peaks.size() > 1){
      float average_time_between_peaks = 0.0f;
      for (int i = 1; i <= static_cast<int>(detected_peaks.size()); i++) {
          int time_between_peaks = (detected_peaks[i] - detected_peaks[i - 1] + BUFFER_SIZE) % BUFFER_SIZE;
          average_time_between_peaks += time_between_peaks;
      }
      average_time_between_peaks /= (detected_peaks.size() - 1);
      RR_interval = average_time_between_peaks / SAMPLING_RATE;
      RR_intervals.push_back(RR_interval);
      heart_rate = 60.0f / (RR_interval);
      std::cout << "Heart Rate: " << heart_rate << " (bpm)" << std::endl;

     }
    } 

    float ECG_filtering(Iir::RBJ::IIRNotch& notch_filter,
                        Iir::Butterworth::LowPass<filter_order>& lowpass_filter,
                        Iir::Butterworth::HighPass<filter_order>& highpass_filter,
                            float sample,float SAMPLING_RATE) {

      float filtered_sample_hp = highpass_filter.filter(sample);
      filtered_sample_hp = lowpass_filter.filter(filtered_sample_hp);
      circularBuffer[headIndex] = notch_filter.filter(filtered_sample_hp);

      if (circularBuffer[headIndex] > threshold && (circularBuffer[headIndex] > circularBuffer[(headIndex -1) % BUFFER_SIZE]) ) { 
        if (detected_peaks.empty() || ((headIndex - detected_peaks.back() + BUFFER_SIZE) % BUFFER_SIZE > (0.2 * SAMPLING_RATE))) { // 300 max HR so ~0.2s between peaks minimum#
             detected_peaks.push_back(headIndex);
            if (detected_peaks.size() > 2){
                detected_peaks.erase(detected_peaks.begin());
                calculate_RR_interval_hr(SAMPLING_RATE);
            }
        }
      }
      int index = headIndex;
      headIndex = (headIndex + 1) % BUFFER_SIZE;
      return circularBuffer[index];
    }

void calculate_hrv() {
    if (!(RR_intervals.size() > 1)){
        std::cout << "Not enough RR intervals to calculate HRV" << std::endl;
    }
    std::cout << "RR intervals: " << RR_intervals.size() << std::endl;
    float sum_sq_diffs = 0.0f;
    for (int i = 1; i < static_cast<int>(RR_intervals.size()); i++) {
        float diff = RR_intervals[i] - RR_intervals[i - 1];
        sum_sq_diffs += diff * diff;
    }

    float mean_sq_diffs = sum_sq_diffs / (RR_intervals.size() - 1);
    float rmssd = std::sqrt(mean_sq_diffs);
    std::cout << "RMSSD: " << rmssd << std::endl;
    HRV.push_back(rmssd);
    // float ln_rmssd = std::log(rmssd);

    // float scaled_hrv = (ln_rmssd / 6.5f) * 100.0f; // max ln_rmssd is usually ~6.5 so scale for a score from 0-100 (from a database)
    // scaled_hrv = std::clamp(scaled_hrv, 0.0f, 100.0f); // Enforce bounds
}

void empty_values() {
    RR_intervals.clear();
}

  
};



#endif  // ECG_HPP