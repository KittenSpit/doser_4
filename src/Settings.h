#pragma once
#include <Arduino.h>
#include <Preferences.h>

#ifndef NUM_PUMPS
#define NUM_PUMPS 3
#endif

struct PumpCal {
  float  mlPerSec = 1.00f;
  uint8_t duty    = 200; // 0..255
};

class Settings {
public:
  void begin(const char* ns="doser") { prefs_.begin(ns,false); load(); }
  void load();
  void save();

  PumpCal pump[NUM_PUMPS];
  uint32_t scheduleIntervalSec = 0;   // 0 = disabled
  uint32_t scheduleOffsetSec[NUM_PUMPS] = {0};

private:
  Preferences prefs_;
};

extern Settings settings;
