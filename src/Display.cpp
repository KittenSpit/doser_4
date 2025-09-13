#include "Display.h"
#include "Settings.h"

Display displayUI;

bool Display::begin() {
  if (!oled_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) return false;
  oled_.clearDisplay();
  oled_.setTextSize(1);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setCursor(0,0);
  oled_.println(F(WEB_TITLE " ready"));
  oled_.display();
  return true;
}

void Display::update() {
  const uint32_t now = millis();
  if (now - lastDrawMs_ < 500) return; // ~2 Hz
  lastDrawMs_ = now;

  oled_.clearDisplay();
  oled_.setCursor(0,0);
  oled_.setTextSize(1);
  oled_.println(F(WEB_TITLE));

  for (int i=0;i<NUM_PUMPS;i++){
    const auto& s = pumpCtl.state(i);
    oled_.setCursor(0, 12 + i*16);
    String line = "P"+String(i)+": ";
    line += s.running ? "RUN " : "IDLE";
    if (s.running){
      uint32_t left = (s.startMs + s.durMs > now) ? (s.startMs + s.durMs - now) : 0;
      line += " t-" + String(left/1000) + "s";
    } else {
      uint32_t due = scheduler.nextRunMs(i);
      if (due != UINT32_MAX) {
        uint32_t sec = (due>now)? (due-now)/1000 : 0;
        line += " next " + String(sec) + "s";
      } else {
        line += " no sched";
      }
    }
    oled_.println(line);
  }
  oled_.display();
}
