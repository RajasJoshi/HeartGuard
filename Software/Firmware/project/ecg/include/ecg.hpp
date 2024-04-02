#ifndef ECG_HPP
#define ECG_HPP

#include "ads1115.hpp"

class ECG {
 public:
  ECG();                                          // Constructor
  ~ECG();                                         // Destructor
  void start(std::unique_ptr<ADS1115>& ads_ptr);  // Start the ECG sensor
  void stop(void);                                // Stop the ECG sensor

  // Add your class methods and member variables here
  void GetECGData(void);

 private:
  // Add your private member variables here
};

#endif  // ECG_HPP