#include <csignal>
#include <iostream>
#include <atomic>
#include <mutex>
#include <pthread.h>
#include <condition_variable>

using namespace std;

enum State { INIT, A, B, C, D, E };

extern "C" void DUMP_GLOB(void);

static int resets_cnt = 0;
static std::mutex stateMutex;
static std::condition_variable resetCondition;
static bool resetPending = false;  // Flag to indicate pending reset

struct programstate {
  bool has_reached_A;
  bool has_reached_B;
};

static struct programstate GLOBAL_STATE = {
    .has_reached_A = false,
    .has_reached_B = false,
};

class MealyMachine {
public:
  MealyMachine() : currentState(INIT) {}

  void processInput(char input) {
    std::unique_lock<std::mutex> lock(stateMutex);
    resetCondition.wait(lock, []{ return !resetPending; });  // Wait until no reset is pending

    // Process input based on the current state
    switch (currentState) {
      case INIT:
        handleInitState(input);
        break;
      case A:
        handleAState(input);
        break;
      case B:
        handleBState(input);
        break;
      case C:
        handleCState(input);
        break;
      case D:
        handleDState(input);
        break;
      case E:
        handleEState(input);
        break;
      default:
        reset();
        break;
    }
  }

  void resetHard() {
    currentState = INIT;
    GLOBAL_STATE.has_reached_A = false;
    GLOBAL_STATE.has_reached_B = false;
  }

private:
  State currentState;

  void handleInitState(char input) {
    switch (input) {
      case 'a':
        GLOBAL_STATE.has_reached_A = true;
        currentState = A;
        cout << "1" << endl;
        break;
      case 'b':
        GLOBAL_STATE.has_reached_B = true;
        currentState = B;
        cout << "2" << endl;
        break;
      default:
        reset();
        break;
    }
  }

  void handleAState(char input) {
    switch (input) {
      case 'c':
        currentState = C;
        cout << "3" << endl;
        break;
      case 'd':
        currentState = D;
        cout << "4" << endl;
        break;
      default:
        reset();
        break;
    }
  }

  void handleBState(char input) {
    switch (input) {
      case 'e':
        currentState = E;
        cout << "5" << endl;
        break;
      default:
        reset();
        break;
    }
  }

  void handleCState(char input) {
    switch (input) {
      case 'f':
        GLOBAL_STATE.has_reached_A = true;
        currentState = A;
        cout << "6" << endl;
        break;
      default:
        reset();
        break;
    }
  }

  void handleDState(char input) {
    switch (input) {
      case 'g':
        GLOBAL_STATE.has_reached_B = true;
        currentState = B;
        cout << "7" << endl;
        break;
      default:
        reset();
        break;
    }
  }

  void handleEState(char input) {
    switch (input) {
      case 'h':
        currentState = C;
        cout << "8" << endl;
        break;
      default:
        reset();
        break;
    }
  }

  void reset() {
    cout << "X" << endl;
    resetHard();
  }
};

static MealyMachine machine;

// Signal-handling thread function
void* signalHandlerThread(void* arg) {
    sigset_t* set = static_cast<sigset_t*>(arg);
    int sig;

    while (true) {
        // Wait for the signal (SIGUSR1)
        if (sigwait(set, &sig) == 0 && sig == SIGUSR1) {
            std::lock_guard<std::mutex> lock(stateMutex);
            resetPending = true;

            DUMP_GLOB();
            machine.resetHard();

            resetPending = false;
            resetCondition.notify_all();  // Wake up main thread for processing
            cout << "OK" << endl << std::flush;
        }
    }
    return nullptr;
}

int main() {
    // Mask SIGUSR1 in all threads
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    // Start the signal-handling thread
    pthread_t sigThread;
    pthread_create(&sigThread, nullptr, signalHandlerThread, &set);

    // Main input loop
    char input;
    while (cin >> input) {
        machine.processInput(input);
    }

    pthread_join(sigThread, nullptr);
    return 0;
}

