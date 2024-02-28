#include "heartguard.hpp"

using namespace std;

static pid_t heartguard = 0;
static bool run = true;
static thread mainThread;
static bool shutdownheartguard = false;
static unique_ptr<ADS1115> hgads1115;
static unique_ptr<MAX30102> hgmax30102;

static void sighandlerShutdown(int sig) {
  if (this_thread::get_id() != mainThread.get_id()) {
    shutdownheartguard = true;
    raise(sig);  // raise the signal to the main thread
  } else {
    if (run) fprintf(stderr, "Caught signal %i\nShutting down...\n", sig);
    run = false;
  }
}

static void sighandlerRedirect(int) { run = false; }

int main(int argc, char* argv[]) {
  try {
    // Set stdout to be unbuffered.
    // This has previously been done using stdbuf, but this does not work for a
    // 64-bit program on a 32-bit system.

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    mainThread = thread([]() {
      while (run) this_thread::yield();
    });

    // parse command-line arguments
    bool watchdog = true;
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--no-watchdog") == 0) {
        watchdog = false;
      } else {
        fprintf(stderr, "Usage: %s [--no-watchdog]\n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }

    // avoid duplicated instances
    int fd = open("/tmp/heartguard", O_CREAT, 0600);
    if (fd == -1 || flock(fd, LOCK_EX | LOCK_NB) == -1) {
      fprintf(stderr, "There is already an instance of this process!\n");
      exit(EXIT_FAILURE);
    }

    // the watchdog
    if (watchdog) {
      for (;;) {
        heartguard = fork();
        if (heartguard == -1)
          exit(EXIT_FAILURE);
        else if (heartguard == 0)
          break;

        int status;
        struct sigaction sa;
        sa.sa_handler = sighandlerRedirect;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGTERM, &sa, NULL) == -1) {
          perror("Error: cannot handle SIGTERM");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
          perror("Error: cannot handle SIGINT");
        }
        if (waitpid(heartguard, &status, 0) != heartguard) {
          exit(EXIT_FAILURE);
        }
        sa.sa_handler = SIG_DFL;
        if (sigaction(SIGTERM, &sa, NULL) == -1) {
          perror("Error: cannot handle SIGTERM");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
          perror("Error: cannot handle SIGINT");
        }

        // detect requested or normal exit
        bool normalExit =
            !run || (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);

        // quit
        exit(WIFEXITED(status) && normalExit ? WEXITSTATUS(status)
                                             : EXIT_FAILURE);
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

    if (shutdownheartguard)
      static_cast<void>(!system("sudo systemctl poweroff &"));

  } catch (const exception& e) {
    cerr << "Exception: " << e.what() << endl;
  } catch (...) {
    std::cerr << "Caught unknown exception\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
