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
int head = 0;   // most recent sample in the buffer (index)
float raw_sample = 0.0f; // raw ECG sample


// biological data



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

    // display buffer
    display_buffer(buffer, head);
    

    head = (head + 1) % BUFFER_SIZE;
    ads1115.stop();
	return 0;
}
