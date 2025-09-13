#pragma once
#include <Arduino.h>
#include "Settings.h"
#include "PumpControl.h"

class Scheduler {
public:
  void begin() { (void)bootMs_; }
  void update();
  uint32_t nextRunMs(int idx) const; // millis-based absolute time
private:
  uint32_t bootMs_ = 0;
};

extern Scheduler scheduler;
