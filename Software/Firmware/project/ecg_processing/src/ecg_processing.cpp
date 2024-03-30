#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include "Iir.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "ads1115.hpp"

// ECG processing constants
const int SAMPLING_RATE = 860; // Hz 

// infinite impulse response library filter params
const int filter_order = 4;                     // 4th order filter
const float cutoff_frequency = 50.0f;           // Hz (EMG / Power line removal)
const float highpass_cutoff_frequency = 0.1f;   // Hz (baseline wander removal)
const float lowpass_cutoff_frequency = 100.0f;  // Hz (noise removal)
float threshold = 0.5f; // threshold for peak detection
int detected_peaks_old_size = 1;

// circlular buffer
const int BUFFER_SIZE = 4*SAMPLING_RATE;    // stores 4 seconds of data (assuming min HR 30 this should always have 2 beats)
std::array<float, BUFFER_SIZE> buffer;
buffer.fill(0.0f); 
int head = 0;                               // most recent sample in the buffer (index)
float raw_sample = 0.0f;                    // raw ECG sample
bool variable_threshold = false;            // if true, threshold is calculated every 4 seconds, else it is fixed at 0.5


// biological data
std::vector<int> detected_peaks;    // Store indices of detected R peaks


class ADS1115Reader : public ADS1115 {
	virtual void hasSample(float v) {
		float raw_sample = v;
	}
};
float ECG_filtering(Iir::RBJ::IIRNotch& notch_filter,
                        Iir::Butterworth::LowPass<filter_order>& lowpass_filter,
                        Iir::Butterworth::HighPass<filter_order>& highpass_filter,
                            float sample) {

    float filtered_sample_hp = highpass_filter.filter(sample);
    filtered_sample_hp = lowpass_filter.filter(filtered_sample_hp);
    return notch_filter.filter(filtered_sample_hp);
}

float calculate_heart_rate(const std::vector<int>& peak_indices) {
    if (peak_indices.empty()) {
        return 0.0f; // if no peaks detected, return 0 heart rate
    }
    float average_time_between_peaks = 0.0f;
    for (int i = 1; i < static_cast<int>(peak_indices.size()); i++) {
        int time_between_peaks = peak_indices[i] - peak_indices[i - 1];
        average_time_between_peaks += time_between_peaks;
    }
    average_time_between_peaks /= (peak_indices.size() - 1);

    float heart_rate = 60.0f / (average_time_between_peaks / SAMPLING_RATE);
    return heart_rate;
}


void display_buffer(const std::vector<float>& buffer, int head) {
  for (int i = 0; i < static_cast<int>(buffer.size()); i++) {
    int index = (head + i) % BUFFER_SIZE;
    std::cout << buffer[index] << " ";
  }
  std::cout << std::endl;
}


int main() {
    // ADS1115 setup
    ADS1115Reader ads1115;
    ADS1115settings ads1115settings;
	ads1115settings.samplingRate = ADS1115settings::FS860HZ; // 860 Hz
    ads1115.start(s);

    // Notch filter for removing EMG / power line noise
    Iir::RBJ::IIRNotch notch_filter;
    notch_filter.setup(SAMPLING_RATE, cutoff_frequency);

    // Butterworth high-pass filter for baseline wander removal
    Iir::Butterworth::HighPass<filter_order> highpass_filter;
    highpass_filter.setup(SAMPLING_RATE, highpass_cutoff_frequency);

    // Butterworth low-pass filter for noise removal
    Iir::Butterworth::LowPass<filter_order> lowpass_filter;
    lowpass_filter.setup(SAMPLING_RATE, lowpass_cutoff_frequency);

    float filtered_sample = ECG_filtering(notch_filter, lowpass_filter, highpass_filter, raw_sample);

    buffer[head] = filtered_sample;

    if (filtered_sample > threshold && 
        (detected_peaks.empty() || (head - detected_peaks.back()) > int(0.27 * SAMPLING_RATE ))) { // 220 max HR so ~0.27s between peaks minimum
        detected_peaks.push_back(head);
    }

    if (static_cast<int>(detected_peaks.size()) > detected_peaks_old_size) {
        float heart_rate = calculate_heart_rate(detected_peaks);
        std::cout << "Heart Rate (bpm): " << heart_rate << std::endl;
        detected_peaks_old_size = int(detected_peaks.size());
    }
    
    if ((i + 1) % (4 * SAMPLING_RATE) == 0) {
        detected_peaks.clear();
        detected_peaks_old_size = 1;

        if (variable_threshold) {
            float max_value = *std::max_element(buffer.begin(), buffer.end());
            float min_value = *std::min_element(buffer.begin(), buffer.end());
            threshold = max_value - min_value / 2 + min_value;
        }
    }


    head = (head + 1) % BUFFER_SIZE;
    ads1115.stop();
	return 0;
}
