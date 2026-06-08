#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdbool.h>
#include <stdint.h>

// typedef struct TIMER {
//   uint32_t timerID;
//   uint32_t timerTarget; // Seconds
//   bool timerRing;
// } timer_t;

typedef struct TIMER {
  //uint32_t durationMillis;
  uint64_t startMillis;
  bool started;
} timer_t;

void initTimers(void);
timer_t timerCreate(/*uint32_t durationMillis*/void);
void timerStart(timer_t* timer);
uint64_t timerCheck(timer_t* timer);
void timerReset(timer_t* timer);

#endif // _TIMER_H_
