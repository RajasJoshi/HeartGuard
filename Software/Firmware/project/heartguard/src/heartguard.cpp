#include "heartguard.hpp"

static bool run = true;
static std::thread mainThread;
static std::unique_ptr<ADS1115> hgads1115;
static std::unique_ptr<MAX30102> hgmax30102;

static void sighandlerShutdown(int sig) {
  if (std::this_thread::get_id() != mainThread.get_id()) {
    run = false;
  } else {
    if (run) std::cerr << "Caught signal " << sig << "\nShutting down...\n";
    run = false;
  }
}

int main(int argc, char* argv[]) {
  try {
    // Set stdout to be unbuffered.
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    mainThread = std::thread([]() {
      while (run) std::this_thread::yield();
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
      hgmax30102->start();
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