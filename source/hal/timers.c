#include "hal/timers.h"
#include "mcal/SysTick.h"
#include <stdint.h>

static uint64_t ticks = 0;

static void updateTick(void);

void initTimers(void) {
  pisr_register(updateTick, 1);
}

timer_t timerCreate(void/*uint32_t durationMillis*/) {
  return (timer_t){.startMillis = 0/*, .durationMillis = durationMillis*/, .started = false};
}

void timerStart(timer_t* timer) {
  timer->startMillis = ticks;
  timer->started = true;
}

uint64_t timerCheck(timer_t* timer) {
  return (ticks - timer->startMillis);
}

void timerReset(timer_t* timer) {
  timer->startMillis = ticks;
}


static void updateTick() {
  ++ticks;
}
