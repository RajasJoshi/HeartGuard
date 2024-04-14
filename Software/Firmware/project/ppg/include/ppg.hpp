#ifndef PPG_HPP
#define PPG_HPP

#include <algorithm>
#include <boost/lockfree/spsc_queue.hpp>
#include <fstream>
#include <queue>

#include "max30102.hpp"

class PPG {
 public:
  PPG();                                                // Constructor
  ~PPG();                                               // Destructor
  void start(std::unique_ptr<MAX30102>& max30102_ptr);  // Start the PPG sensor
  void stop(void);                                      // Stop the PPG sensor
  boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>>
      ppgtcpqueue;

 private:
  bool running = false;  // Indicates if the PPG sensor is running
  float PPG_filtering();
};

#endif  // PPG_HPP