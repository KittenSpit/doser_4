#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "PumpControl.h"
#include "Scheduler.h"

class Display {
public:
  bool begin();
  void update();
private:
  Adafruit_SSD1306 oled_{OLED_W, OLED_H, &Wire, -1};
  uint32_t lastDrawMs_ = 0;
};

extern Display displayUI;
