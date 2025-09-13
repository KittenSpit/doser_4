#pragma once
#include <Arduino.h>
#include "Settings.h"
#include "Logger.h"

// DRV8871: IN1/IN2
// 00 = Coast, 01 = Reverse, 10 = Forward, 11 = Brake
enum PumpDir : uint8_t { DIR_COAST=0, DIR_FWD=1, DIR_REV=2, DIR_BRAKE=3 };

struct PumpState {
  bool running=false;
  uint32_t startMs=0;
  uint32_t durMs=0;
  float deliveredML=0.0f;
  // hardware mapping
  uint8_t in1Pin=255, in2Pin=255;
  uint8_t ch1=255,   ch2=255;  // LEDC channels for IN1/IN2
  PumpDir dir=DIR_COAST;
};

class PumpControl {
public:
  // Provide arrays of IN1 and IN2 pins for each pump
  void begin(const uint8_t* in1Pins, const uint8_t* in2Pins, int num);
  void update();

  bool startPump(int idx, uint32_t durationMs, PumpDir dir=DIR_FWD);
  void stopPump(int idx, bool brake=false);
  void purge(int idx, uint32_t durationMs); // reverse briefly

  const PumpState& state(int idx) const { return p_[idx]; }

private:
  PumpState p_[NUM_PUMPS];
  void setDrive_(int idx, PumpDir dir, uint8_t duty);
};

extern PumpControl pumpCtl;
