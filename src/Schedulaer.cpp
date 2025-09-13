#include "Scheduler.h"

Scheduler scheduler;

uint32_t Scheduler::nextRunMs(int idx) const {
  if (settings.scheduleIntervalSec == 0) return UINT32_MAX;
  const uint32_t intervalMs = settings.scheduleIntervalSec * 1000UL;
  const uint32_t ofsMs = settings.scheduleOffsetSec[idx] * 1000UL;
  const uint32_t now = millis();
  uint32_t phase = (now + UINT32_MAX - ofsMs) % intervalMs;
  return now + (intervalMs - phase);
}

void Scheduler::update() {
  if (settings.scheduleIntervalSec == 0) return;
  const uint32_t now = millis();
  for (int i=0;i<NUM_PUMPS;i++){
    uint32_t due = nextRunMs(i);
    if (due != UINT32_MAX && (int32_t)(due - now) <= 0) {
      pumpCtl.startPump(i, 3000, DIR_FWD); // example 3s per dose
    }
  }
}
