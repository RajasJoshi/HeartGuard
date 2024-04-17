#include "heartguard.hpp"

// Global variables
static std::atomic<bool> run{true};  ///< Controls the main execution loop.

static std::atomic<bool> gpio_pins_ready{
    false};  ///< Indicates if the GPIO pins are ready.
static std::unique_ptr<ADS1115> hgads1115;       ///< ADS1115 sensor.
static std::unique_ptr<MAX30102> hgmax30102;     ///< MAX30102 sensor.
static std::unique_ptr<PPG> hgppg;               ///< PPG processing>
static std::unique_ptr<ECG> hgecg;               ///< ECG sensor.
static std::unique_ptr<PPG> hgppg;               ///< GPIO pins.
static std::unique_ptr<TcpServer> hgtcpserver;   ///< TCP server.
static std::unique_ptr<std::thread> mainThread;  ///< Main thread.
static std::unique_ptr<std::thread>
    ads1115Thread;  ///< Thread for ADS1115 sensor.
static std::unique_ptr<std::thread>
    max30102Thread;                             ///< Thread for MAX30102 sensor.
static std::unique_ptr<std::thread> ecgThread;  ///< Thread for ECG sensor.
static std::unique_ptr<std::thread> ppgThread;  ///< Thread for PPG sensor.
static std::unique_ptr<std::thread>
    tcpServerThread;                ///< Thread for TCP server.
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

    mainThread = std::make_unique<std::thread>([]() {
      std::unique_lock<std::mutex> lk(cv_m);
      cv.wait(lk, [] { return !run; });
    });

    // Create the ads1115 thread
    ads1115Thread = std::make_unique<std::thread>([]() {
      try {
        hgads1115 = std::make_unique<ADS1115>();
        hgads1115->start();
      } catch (const std::exception& e) {
        std::cerr << "Exception in ads1115Thread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in ads1115Thread\n";
      }
    });

    ecgThread = std::make_unique<std::thread>([]() {
      try {
        hgecg = std::make_unique<ECG>();
        hgecg->start(hgads1115,hgppg);
      } catch (const std::exception& e) {
        std::cerr << "Exception in ecgThread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in ecgThread\n";
      }
    });

    ppgThread = std::make_unique<std::thread>([]() {
      try {
        hgppg = std::make_unique<PPG>(hgmax30102);
        cout << "Starting..." << endl;
        hgppg.begin();
        cout << "Began heart rate calculation..." << endl;
        while (1) {
          cout << "IR Heart Rate- Latest:" << hgppg.getLatestIRHeartRate() << ", SAFE:" << hgppg.getSafeIRHeartRate();
          cout << ", Temperature: " << hgppg.getLatestTemperatureF() << endl;
        }
      } catch (const std::exception& e) {
        std::cerr << "Exception in ppgThread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in ppgThread\n";
      }
    });

    // Create the max30102 thread
    max30102Thread = std::make_unique<std::thread>([]() {
      try {
        hgmax30102 = std::make_unique<MAX30102>();
        int result = hgmax30102->start();
        if (result < 0) {
          throw std::runtime_error(
              "Failed to start I2C (Error: " + std::to_string(result) + ").");
        }
        std::cout << "Device found (revision: " << result << ")!" << std::endl;

        hgmax30102->setup();
        hgmax30102->setPulseAmplitudeRed(0x0A);
      } catch (const std::exception& e) {
        std::cerr << "Exception in max30102Thread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in max30102Thread\n";
      }
    });

    // Create the ppg thread
    ppgThread = std::make_unique<std::thread>([]() {
      try {
        hgppg = std::make_unique<PPG>();
        hgppg->start(hgmax30102);
        std::cout << "Began heart rate calculation..." << std::endl;
      } catch (const std::exception& e) {
        std::cerr << "Exception in ppgThread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in ppgThread\n";
      }
    });

    // Create the tcp server thread
    tcpServerThread = std::make_unique<std::thread>([]() {
      try {
        hgtcpserver = std::make_unique<TcpServer>();
        hgtcpserver->start(hgecg);

      } catch (const std::exception& e) {
        std::cerr << "Exception in tcpServerThread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Caught unknown exception in tcpServerThread\n";
      }
    });

    if (ads1115Thread) {
      ads1115Thread->join();
    }
    if (ecgThread) {
      ecgThread->join();
    }
    if (tcpServerThread) {
      tcpServerThread->join();
    }
    if (max30102Thread) {
      max30102Thread->join();
    }
    if (ppgThread) {
      ppgThread->join();
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
    if (ecgThread && ecgThread->joinable()) {
      ecgThread->join();
    }
    if (tcpServerThread && tcpServerThread->joinable()) {
      tcpServerThread->join();
    }
    if (max30102Thread && max30102Thread->joinable()) {
      max30102Thread->join();
    }
    if (ppgThread && ppgThread->joinable()) {
      ppgThread->join();
    }
    if (mainThread && mainThread->joinable()) {
      mainThread->join();
    }
    throw;
  }

  return EXIT_SUCCESS;
}