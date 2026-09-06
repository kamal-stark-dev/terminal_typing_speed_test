#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#define TEST_SECONDS 60.0
#define BAR_WIDTH 30

#define RESET            "\033[0m"
#define BOLD             "\033[1m"
#define DIM              "\033[2m"
#define REVERSE          "\033[7m"

#define TITLE_COLOR      "\033[1;38;5;220m"
#define HUD_COLOR        "\033[38;5;252m"
#define INFO_COLOR       "\033[38;5;252m"
#define PROMPT_COLOR     "\033[1;38;5;214m"
#define ERROR_COLOR      "\033[1;38;5;196m"
#define SUCCESS_COLOR    "\033[1;38;5;46m"
#define ERROR_BG         "\033[41m"

#define CURSOR_HOME      "\033[H"
#define CLEAR_TO_EOL     "\033[K"
#define CLEAR_TO_END     "\033[J"
#define HIDE_CURSOR      "\033[?25l"
#define SHOW_CURSOR      "\033[?25h"

// fix for flickering
#define SYNC_START       "\033[?2026h"
#define SYNC_END         "\033[?2026l"

const char* passages[] = {
  "The terminal opens like a quiet room where though can move without noise, "
  "and the cursor waits patiently for each careful keystroke. Programming is "
  "not only about speed or clever tricks; it is about settling into a problem, "
  "breaking it into small honest pieces, and letting the machine repeat what "
  "you have clearly explained. In this calm space, every command is a small "
  "intention, every function is a promise, and every test is a gentle check "
  "that the idea still holds together. The soft rhythm of typing becomes a "
  "focus practice, where mistakes are not failures but signals asking you to "
  "slow down and understand more deeply. When the colors glow quietly and the "
  "screen updates without hurry, you can feel how programming, patience, and "
  "attention support each other. Keep your breathing steady, let your shoulders "
  "relax, and let each character arrive with purpose. Over time, this steady "
  "practice builds speed naturally, because accuracy comes first, clamness comes "
  "next, and confidence follows quietly behind.",
  "Passage 2 - Hello MFs",
  "Passage 3 - Bye MFs",
};

#define PASSAGE_COUNT (sizeof(passages) / sizeof(passages[0]))

static struct termios originalTermios;
static bool rawEnabled = false;
static bool cursorHidden = false;

void showCursor(void) {
  if (cursorHidden) {
    printf(SHOW_CURSOR);
    fflush(stdout);
    cursorHidden = false;
  }
}

void hideCursor(void) {
  if (!cursorHidden) {
    printf(HIDE_CURSOR);
    fflush(stdout);
    cursorHidden = true;
  }
}

void disableRawMode(void) {
  if (rawEnabled) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
    rawEnabled = false;
  }
}

void enableRawMode(void) {
  if (!rawEnabled) {
    if (tcgetattr(STDIN_FILENO, &originalTermios) == -1) 
      return;

    struct termios raw = originalTermios;

    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
      return;

    rawEnabled = true;
  }
}

void cleanupTerminal(void) {
  showCursor();
  disableRawMode();
}

double getMonotonicSeconds(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);

  return (double) t.tv_sec + (double) t.tv_nsec / 1000000000.0;
}

void clearScreen(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void printHeader(void) {
  printf(CLEAR_TO_EOL "\n");
  printf(TITLE_COLOR " ===================================" RESET CLEAR_TO_EOL "\n");
  printf(TITLE_COLOR "  T Y P I N G - S P E E D - T E S T" RESET CLEAR_TO_EOL "\n");
  printf(TITLE_COLOR " ===================================" RESET CLEAR_TO_EOL "\n");
  printf(CLEAR_TO_EOL "\n");
}

int countCorrect(const char *target, const char *typed, size_t typedLen) {
  size_t targetLen = strlen(target);
  size_t compareLen = typedLen < targetLen ? typedLen : targetLen;

  int correct = 0;

  for (size_t i = 0; i < compareLen; i++) {
    if (typed[i] == target[i])
      correct++;
  }

  return correct;
}

double calculateWpm(int correct, double seconds) {
  if (seconds < 1.0)
    seconds = 1.0;

  double minutes = seconds / 60.0;
  
  return (correct / 5.0) / minutes;
}

void printTimeBar(double elapsed) {
  if (elapsed < 0.0)
    elapsed = 0.0;

  if (elapsed > TEST_SECONDS)
    elapsed = TEST_SECONDS;

  double remaining = TEST_SECONDS - elapsed;
  int filled = (int)((elapsed / TEST_SECONDS) * BAR_WIDTH);

  if (filled > BAR_WIDTH)
    filled = BAR_WIDTH;

  printf(HUD_COLOR "   [" RESET);

  for (int i = 0; i < BAR_WIDTH; i++) {
    if (i < filled)
      printf(SUCCESS_COLOR "#" RESET);
    else 
      printf(DIM "-" RESET);
  }

  printf(HUD_COLOR "] " RESET);

  printf(BOLD "%.1fs elapsed | %.1fs left" RESET CLEAR_TO_EOL "\n",
         elapsed, remaining);
}

void printLiveTyped(const char *target, const char *typed, size_t typedLen) {
  size_t targetLen = strlen(target);

  printf("  ");

  for (size_t i = 0; i < typedLen; i++) {
    if (i < targetLen && typed[i] == target[i])
      printf(SUCCESS_COLOR "%c" RESET, typed[i]);
    else {
      if (typed[i] == ' ')
        printf(ERROR_BG " " RESET);
      else 
        printf(ERROR_COLOR "%c" RESET, typed[i]);
    }
  }

  if (typedLen < targetLen) {
    printf(REVERSE "%c" RESET, target[typedLen]);
    printf(DIM "%s" RESET, target + typedLen + 1);
  }

  printf(RESET CLEAR_TO_EOL "\n");
}

void printTestScreen(const char *target, const char *typed, size_t typedLen, double elapsed) {
  printf(SYNC_START CURSOR_HOME);

  printHeader();

  int correct = countCorrect(target, typed, typedLen);
  double wpm = calculateWpm(correct, elapsed);

  double accuracy = 100.0;

  if (typedLen > 0)
    accuracy = ((double)correct * 100.0) / (double)typedLen;

  printf(HUD_COLOR "  Duration: " RESET BOLD "60 seconds" RESET CLEAR_TO_EOL "\n");

  printf(HUD_COLOR "  WPM: " RESET TITLE_COLOR "%.1f" RESET
         HUD_COLOR "   Accuracy: " RESET TITLE_COLOR "%.1f%%" RESET
         HUD_COLOR "   Typed: " RESET BOLD "%zu/%zu" RESET CLEAR_TO_EOL "\n",
         wpm, accuracy, typedLen, strlen(target));

  printTimeBar(elapsed);

  printf(CLEAR_TO_EOL "\n");

  printf(INFO_COLOR "   Type the text below. Backspace works. Ctrl + C quits." RESET CLEAR_TO_EOL "\n");

  printf(CLEAR_TO_EOL "\n");

  printf(INFO_COLOR "Text: " RESET CLEAR_TO_EOL "\n");

  printLiveTyped(target, typed, typedLen);

  printf(CLEAR_TO_END SYNC_END);

  fflush(stdout);
}

void runTest(const char *target,
             char *typed,
             size_t bufferSize,
             size_t *typedLenOut,
             double *elapsedOut,
             bool *finishedEarlyOut) {
  size_t targetLen = strlen(target);
  size_t typedLen = 0;

  typed[0] = '\0';

  clearScreen();
  hideCursor();
  enableRawMode();

  double start = getMonotonicSeconds();
  double elapsed = 0.0;
  
  size_t lastTypedLen = (size_t)-1;
  int lastTenth = -1;

  while (true) {
    elapsed = getMonotonicSeconds() - start;

    if (elapsed >= TEST_SECONDS || typedLen >= targetLen)
      break;

    int tenth = (int)(elapsed * 10.0);

    if (typedLen != lastTypedLen || tenth != lastTenth) {
      printTestScreen(target, typed, typedLen, elapsed);
      lastTypedLen = typedLen;
      lastTenth = tenth;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval timeout; // wait for 0.1s before updating time and screen
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    
    int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);

    if (ready < 0)
      continue;

    if (ready > 0) {
      char buf[64];
      ssize_t n = read(STDIN_FILENO, buf, sizeof(buf)); // ssize_t is signed size type (-ve used for errors)

      for (ssize_t i = 0; i < n; i++) {
        char ch = buf[i];

        if (ch == 3) { 
          // when Ctrl + C is pressed
          cleanupTerminal();
          clearScreen();
          printf(TITLE_COLOR "Test Aborted.\n\n" RESET);
          exit(0); 
        }
        else if (ch == 127 || ch == 8) { 
          // backspace and delete key press 
          if (typedLen > 0) {
            typedLen--;
            typed[typedLen] = '\0';
          }
        }
        else if (ch >= 32 && ch <= 126) { 
          // 32 -> 126 are all printable ASCII characters from a-z, A-Z, 0-9, spaces, punctuation
          if (typedLen < targetLen && typedLen + 1 < bufferSize) {
            typed[typedLen] = ch;
            typedLen++;
            typed[typedLen] = '\0';
          }
        }

        if (typedLen >= targetLen)
          break;
      }
    }
  }

  elapsed = getMonotonicSeconds() - start;

  cleanupTerminal();

  bool finishedEarly = typedLen >= targetLen;

  if (!finishedEarly && elapsed > TEST_SECONDS) 
    elapsed = TEST_SECONDS;

  *typedLenOut = typedLen;
  *elapsedOut = elapsed;
  *finishedEarlyOut = finishedEarly;
}

void printStats(int correct,
                size_t typedLen,
                size_t targetLen,
                double seconds,
                double wpm, 
                double accuracy) {
  size_t errors = 0;

  if (typedLen > (size_t)correct)
    errors = typedLen - (size_t)correct;

  printf(HUD_COLOR "  TIME:     " RESET BOLD "%.2f seconds" RESET CLEAR_TO_EOL "\n", seconds);
  printf(HUD_COLOR "  WPM:      " RESET TITLE_COLOR "%.1f" RESET CLEAR_TO_EOL "\n", wpm);

  printf(HUD_COLOR "  Accuracy: " RESET);
  
  if (accuracy >= 95.0)
    printf(SUCCESS_COLOR "%.1f%%" RESET CLEAR_TO_EOL "\n", accuracy); 
  else if (accuracy >= 80.0)
    printf(TITLE_COLOR "%.1f%%" RESET CLEAR_TO_EOL "\n", accuracy);
  else 
    printf(ERROR_COLOR "%.1f%%" RESET CLEAR_TO_EOL "\n", accuracy);

  printf(HUD_COLOR "  Correct:  " RESET BOLD "%d/%zu characters" RESET CLEAR_TO_EOL "\n",
         correct, targetLen);

  printf(HUD_COLOR "  Errors:   " RESET BOLD "%zu typed characters" RESET CLEAR_TO_EOL "\n\n",
         errors);
}

void printRating(double wpm, double accuracy) {
  if (accuracy < 70.0) 
    printf(ERROR_COLOR "  Focus on accuracy first. Speed will follow." RESET CLEAR_TO_EOL "\n\n");
  else if (wpm  >= 80.0)
    printf(SUCCESS_COLOR "  Excellent speed!" RESET CLEAR_TO_EOL "\n\n");
  else if (wpm >= 60.0)
    printf(SUCCESS_COLOR "  Nice! That's a strong typing pace." RESET CLEAR_TO_EOL "\n\n");
  else if (wpm >= 40.0)
    printf(TITLE_COLOR "  Good work. Keep practicing!" RESET CLEAR_TO_EOL "\n\n");
  else 
    printf(INFO_COLOR "  Keep going! Daily practice builds speed." RESET CLEAR_TO_EOL "\n\n");
}

bool askPlayAgain(void) {
  char line[32];

  printf(SUCCESS_COLOR "  Play again? (y/n): " RESET);
  fflush(stdout);

  if (fgets(line, sizeof(line), stdin) == NULL) 
    return false;

  return line[0] == 'y' || line[0] == 'Y';
}

int main() {
  // when the program exits normally call `cleanupTerminal()`
  atexit(cleanupTerminal); 

  // sets output in no buffering state (prevents flickering)
  setvbuf(stdout, NULL, _IONBF, 0);

  srand((unsigned int)time(NULL));

  char input[2048];
  bool play = true;

  clearScreen();

  printf(TITLE_COLOR "  TYPING SPEED TEST\n\n" RESET);
  printf(INFO_COLOR "  Press Enter to continue..." RESET);
  fflush(stdout);

  if (fgets(input, sizeof(input), stdin) == NULL)
    return 0;

  while (play) {
    clearScreen();
    printHeader();

    int index = rand() % (int)PASSAGE_COUNT;
    const char *target = passages[index];

    printf(INFO_COLOR "  The test lasts 60 seconds or early if you finish the passage." RESET CLEAR_TO_EOL "\n\n");
    printf(PROMPT_COLOR "  Press Enter to start..." RESET);
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL)
      return 0;

    char typed[2048];
    size_t typedLen = 0;
    double elapsed = 0.0;
    bool finishedEarly = false;

    runTest(target, typed, sizeof(typed), &typedLen, &elapsed, &finishedEarly);

    int correct = countCorrect(target, typed, typedLen);
    double wpm = calculateWpm(correct, elapsed);

    double accuracy = 0.0;

    if (typedLen > 0)
      accuracy = ((double)correct * 100.0) / (double)typedLen;

    if (accuracy > 100.0)
      accuracy = 100.0;

    clearScreen();
    printHeader();

    if (finishedEarly)
      printf(SUCCESS_COLOR "  You finished the passage early!" RESET CLEAR_TO_EOL "\n\n");
    else 
      printf(TITLE_COLOR "  Time's up!" RESET CLEAR_TO_EOL "\n\n");

    printStats(correct, typedLen, strlen(target), elapsed, wpm, accuracy);
    printRating(wpm, accuracy);

    if (!askPlayAgain())
      play = false;

    clearScreen();
  }

  printf(TITLE_COLOR "\n  Thanks for typing!\n\n" RESET);
  
  return 0;
}
