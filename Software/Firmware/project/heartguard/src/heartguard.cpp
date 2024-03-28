#include "heartguard.hpp"

// Global variables
static std::atomic<bool> run{true};  ///< Controls the main execution loop.
static std::atomic<bool> enable_max30102{
    false};  ///< Enables the MAX30102 sensor.
static std::atomic<bool> gpio_pins_ready{
    false};  ///< Indicates if the GPIO pins are ready.
static std::unique_ptr<std::thread> mainThread;  ///< Main thread.
static std::unique_ptr<ADS1115> hgads1115;       ///< ADS1115 sensor.
static std::unique_ptr<MAX30102> hgmax30102;     ///< MAX30102 sensor.
static std::unique_ptr<std::thread>
    ads1115Thread;  ///< Thread for ADS1115 sensor.
static std::unique_ptr<std::thread>
    max30102Thread;                 ///< Thread for MAX30102 sensor.
static std::condition_variable cv;  ///< Condition variable for main thread.
static std::condition_variable gpio_cv;  ///< Condition variable for GPIO pins.
static std::mutex cv_m;                  ///< Mutex for main thread.
static std::mutex gpio_m;                ///< Mutex for GPIO pins.

/**
 * @brief Signal handler for shutdown.
 *
 * @param sig Signal number.
 */
static void sighandlerShutdown(int sig) {
  std::cerr << "Caught signal " << sig << "\nShutting down...\n";
  run = false;
  cv.notify_all();
}

/**
 * @brief GPIO alert function.
 *
 * @param gpio GPIO pin number.
 * @param level Level of the GPIO pin.
 * @param tick Time at which the alert occurred.
 */
void gpioAlert(int gpio, int level, uint32_t tick) {
  if (gpio == 27 && level == PI_LOW) {
    // If pin 27 goes low, check the state of pin 22
    int pin2_state = gpioRead(22);
    if (pin2_state == PI_LOW) {
      // If both pins are low, signal the condition variable
      std::lock_guard<std::mutex> lk(gpio_m);
      gpio_pins_ready = true;
      gpio_cv.notify_all();
    }
  }
}

/**
 * @brief Main function.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Exit status.
 */

int main(int argc, char* argv[]) {
  try {
    // Set stdout to be unbuffered.
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-h" || arg == "--help") {
        std::cout << "Usage: " << argv[0] << " [options]\n";
        // Add more usage information here
        return EXIT_SUCCESS;
      } else if (arg == "-v" || arg == "--version") {
        std::cout << "Version 1.0.0\n";
        return EXIT_SUCCESS;
      } else if (arg == "-m" || arg == "--max30102") {
        enable_max30102 = true;
      } else {
        std::cerr << "Unknown argument: " << arg << "\n";
        return EXIT_FAILURE;
      }
    }

    // register signal handler for ctrl+c and termination signal
    struct sigaction sa;
    sa.sa_handler = sighandlerShutdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
      perror("Error: cannot handle SIGTERM");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
      perror("Error: cannot handle SIGINT");
    }

    // Initialize the GPIO library
    int result = gpioInitialise();
    if (result < 0) {
      std::cerr << "Failed to initialize GPIO, error " << result << std::endl;
      return EXIT_FAILURE;
    }

    mainThread = std::make_unique<std::thread>([]() {
      std::unique_lock<std::mutex> lk(cv_m);
      cv.wait(lk, [] { return !run; });
    });

    // Set the callback function for GPIO pin 27
    result = gpioSetMode(27, PI_INPUT);
    if (result < 0) {
      throw std::runtime_error("Failed to set GPIO mode, error " +
                               std::to_string(result));
    }
    result = gpioSetPullUpDown(27, PI_PUD_UP);  // Set pull up resistor
    if (result < 0) {
      throw std::runtime_error("Failed to set GPIO pull configuration, error " +
                               std::to_string(result));
    }
    result = gpioSetAlertFunc(27, gpioAlert);
    if (result < 0) {
      throw std::runtime_error("Failed to set GPIO alert function, error " +
                               std::to_string(result));
    }

    // Create the ads1115 thread
    ads1115Thread = std::make_unique<std::thread>([]() {
      try {
        // Wait for the GPIO pins to be ready before starting the ads1115 thread
        std::unique_lock<std::mutex> lk(gpio_m);
        gpio_cv.wait(lk, [] { return gpio_pins_ready.load(); });

        hgads1115 = std::make_unique<ADS1115>();
        hgads1115->start();
      } catch (const std::exception& e) {
        std::cerr << "Exception in ads1115Thread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in ads1115Thread\n";
      }
    });

    // Create the max30102 thread
    if (enable_max30102) {
      max30102Thread = std::make_unique<std::thread>([]() {
        try {
          hgmax30102 = std::make_unique<MAX30102>();
          int result = hgmax30102->begin();
          if (result < 0) {
            throw std::runtime_error(
                "Failed to start I2C (Error: " + std::to_string(result) + ").");
          }
          std::cout << "Device found (revision: " << result << ")!"
                    << std::endl;

          hgmax30102->setup();
          hgmax30102->setPulseAmplitudeRed(0x0A);
        } catch (const std::exception& e) {
          std::cerr << "Exception in max30102Thread: " << e.what() << std::endl;
        } catch (...) {
          std::cerr << "Caught unknown exception in max30102Thread\n";
        }
      });
    }

    ads1115Thread->join();
    if (max30102Thread) {
      max30102Thread->join();
    }
    mainThread->join();  // Wait for the main thread to finish

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  } catch (...) {
    // If an exception is thrown, join the threads before rethrowing the
    // exception
    if (ads1115Thread && ads1115Thread->joinable()) {
      ads1115Thread->join();
    }
    if (max30102Thread && max30102Thread->joinable()) {
      max30102Thread->join();
    }
    if (mainThread && mainThread->joinable()) {
      mainThread->join();
    }
    throw;
  }

  return EXIT_SUCCESS;
}