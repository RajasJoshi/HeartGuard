#include "heartguard.hpp"

static std::atomic<bool> run(true);
static std::thread mainThread;
static std::unique_ptr<ADS1115> hgads1115;
static std::unique_ptr<MAX30102> hgmax30102;
static std::condition_variable cv;
static std::mutex cv_m;

static void sighandlerShutdown(int sig) {
  std::cerr << "Caught signal " << sig << "\nShutting down...\n";
  run = false;
  cv.notify_all();
}

int main(int argc, char* argv[]) {
  try {
    // Set stdout to be unbuffered.
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    mainThread = std::thread([]() {
      std::unique_lock<std::mutex> lk(cv_m);
      cv.wait(lk, [] { return !run; });
    });

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

    std::thread max30102Thread = std::thread([]() {
      hgmax30102 = std::make_unique<MAX30102>();
      int result = hgmax30102->begin();
      if (result < 0) {
        std::cout << "Failed to start I2C (Error: " << result << ")."
                  << std::endl;
        return (-1 * result);
      }
      std::cout << "Device found (revision: " << result << ")!" << std::endl;

      hgmax30102->setup();
      hgmax30102->setPulseAmplitudeRed(0x0A);
      while (1) {
        std::cout << "IR: " << hgmax30102->getIR();
        std::cout << ", RED: " << hgmax30102->getRed();
        std::cout << std::endl;
        usleep(500);
      }
    });

    std::thread ads1115Thread = std::thread([]() {
      hgads1115 = std::make_unique<ADS1115>();
      hgads1115->start();
    });

    ads1115Thread.join();
    max30102Thread.join();
    mainThread.join();  // Wait for the main thread to finish

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Caught unknown exception\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}