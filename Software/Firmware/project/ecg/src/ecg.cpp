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
  std::cout << "about to setup socket" << std::endl;

  setupSocket();

  std::cout << "socket setup" << std::endl;

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

      if  (socket_connected) {
          std::string message = "raw" + std::to_string(value) + ",filtered" + std::to_string(fs) + ",heartrate" + std::to_string(heart_rate) + ",";
          if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
              std::cerr << "Error sending data" << std::endl;
              socket_connected = false; // Handle connection issue
              // Potentially attempt to re-establish connection
          }
      }

      

      if (count%int(10*SAMPLING_RATE) == 0){
        std::cout << "recalculating mean, stdev, and threshold" << std::endl;
        recalculate_mean();
        recalculate_stdev();
        recalculate_threshold();
        empty_values();
        std::cout << "mean: " << mean << " stdev: " << stdev << " threshold: " << threshold << std::endl;
      }

      // if (count%int(15*SAMPLING_RATE) == 0){
      //   calculate_hrv();
      //   empty_values();
      // }

      count = count%10000 + 1;
    }

    }
}

void ECG::setupSocket() {
    // Create the server socket
    std::cout << "1";
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "2";

    // Configure address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port);
    std::cout << "3";

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        exit(EXIT_FAILURE); 
    }
    std::cout << "4";
    // Start listening
    if (listen(server_socket, SOMAXCONN) < 0) {
        std::cerr << "Error in listen()" << std::endl;
        exit(EXIT_FAILURE); 
    }
    std::cout << "5";
    // Accept a connection
    socklen_t addrlen = sizeof(address);
    client_socket = accept(server_socket, (struct sockaddr*)&address, &addrlen);
    if (client_socket < 0) {
        std::cerr << "Error accepting client" << std::endl;
    } else {
        std::cout << "Client connected" << std::endl;
        socket_connected = true;
    }
    std::cout << "6";
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
