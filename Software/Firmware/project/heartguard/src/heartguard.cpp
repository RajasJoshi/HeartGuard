#include "heartguard.hpp"

using namespace std;

static bool run = true;
static thread mainThread;
static unique_ptr<ADS1115> hgads1115;
static unique_ptr<MAX30102> hgmax30102;

static void sighandlerShutdown(int sig) {
  if (this_thread::get_id() != mainThread.get_id()) {
    run = false;
  } else {
    if (run) fprintf(stderr, "Caught signal %i\nShutting down...\n", sig);
    run = false;
  }
}

int main(int argc, char* argv[]) {
  try {
    // Set stdout to be unbuffered.
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    mainThread = thread([]() {
      while (run) this_thread::yield();
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

    thread max30102Thread = thread([]() {
      hgmax30102 = make_unique<MAX30102>();
      hgmax30102->start();
    });

    thread ads1115Thread = thread([]() {
      hgads1115 = make_unique<ADS1115>();
      hgads1115->start();
    });

    ads1115Thread.join();
    max30102Thread.join();
    mainThread.join();  // Wait for the main thread to finish

  } catch (const exception& e) {
    cerr << "Exception: " << e.what() << endl;
  } catch (...) {
    std::cerr << "Caught unknown exception\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// int main(int argc, char* argv[]) {
//   fprintf(stderr, "Press any key to stop.\n");
//   ADS1115 ads1115rpi;

//   ads1115rpi.start();
//   fprintf(stderr, "fs = %d\n",
//           ads1115rpi.getADS1115settings().getSamplingRate());
//   getchar();
//   ads1115rpi.stop();
//   return 0;
// }
