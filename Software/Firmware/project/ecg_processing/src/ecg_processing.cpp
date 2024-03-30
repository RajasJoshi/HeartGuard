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


// circlular buffer
const int BUFFER_SIZE = 4*SAMPLING_RATE;    // stores 4 seconds of data (assuming min HR 30 this should always have 2 beats)
std::array<float, BUFFER_SIZE> buffer;
buffer.fill(0.0f); 
int head = 0;                               // most recent sample in the buffer (index)
float raw_sample = 0.0f;                    // raw ECG sample
bool variable_threshold = false;            // if true, threshold is calculated every 4 seconds, else it is fixed at 0.5


// biological data
float threshold = 0.5f;             // threshold for peak detection
int detected_peaks_old_size = 1;
std::vector<int> detected_peaks;    // Store indices of detected R peaks
float RR_interval = 0.0f;           // stores the most recent R_R interval
std::vector<float> RR_intervals;    // Stores each RR interval for HRV calculation
const int HRV_WINDOW = 30;          // settable second window for HRV calculation (15 = minimum for RMSSD)
std::vector<float> HRV;             // Stores the hrv scores for calcualtion every specified window


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

float calculate_RR_interval(const std::vector<int>& peak_indices) {
    if (peak_indices.size() < 1) {
        std::cout << "No peaks detected" << std::endl;
        return 0.0f; // if no peaks detected, return 0 heart rate
    }

    float last_peak = peak_indices.back();
    float second_last_peak = peak_indices[peak_indices.size() - 2];
    return (last_peak - second_last_peak) * 1000 / SAMPLING_RATE; // convert to ms
}

float calculate_hrv(const std::vector<float>& RR_intervals) {
    if (!(RR_intervals.size() > 1)){
        std::cout << "Not enough RR intervals to calculate HRV" << std::endl;
        return 0.0;
    }
    std::cout << "RR intervals: " << RR_intervals.size() << std::endl;
    float sum_sq_diffs = 0.0f;
    for (int i = 1; i < static_cast<int>(RR_intervals.size()); i++) {
        float diff = RR_intervals[i] - RR_intervals[i - 1];
        sum_sq_diffs += diff * diff;
    }

    float mean_sq_diffs = sum_sq_diffs / (RR_intervals.size() - 1);
    float rmssd = std::sqrt(mean_sq_diffs);
    float ln_rmssd = std::log(rmssd);

    float scaled_hrv = (ln_rmssd / 6.5f) * 100.0f; // max ln_rmssd is usually ~6.5 so scale for a score from 0-100 (from a database)
    scaled_hrv = std::clamp(scaled_hrv, 0.0f, 100.0f); // Enforce bounds
    return rmssd;
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
        RR_interval = calculate_RR_interval(detected_peaks);
        RR_intervals.push_back(RR_interval);
        // std::cout << "RR interval: " << RR_interval << std::endl;
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

    if ((i + 1) % (HRV_WINDOW * SAMPLING_RATE) == 0) {
        float hrv = calculate_hrv(RR_intervals);
        RR_intervals.clear();
        HRV.push_back(hrv);
        std::cout << "HRV: " << hrv << std::endl;
    }


    head = (head + 1) % BUFFER_SIZE;
    ads1115.stop();
	return 0;
}

/*
 for rmssd values
    13–48 ms —  healthy adults aged 38–42 years
    35–107 ms — elite athletes
    53.5–82 ms — healthy men
    40.5–71 ms — men
    29–65 ms — women
    23–72 ms — men
    22–79 ms — women
    */