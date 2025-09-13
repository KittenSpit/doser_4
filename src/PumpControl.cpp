#include "PumpControl.h"

PumpControl pumpCtl;

void PumpControl::begin(const uint8_t* in1Pins, const uint8_t* in2Pins, int num) {
  const int freq = 5000;
  const int res  = 8;

  for (int i=0;i<num;i++){
    p_[i].in1Pin = in1Pins[i];
    p_[i].in2Pin = in2Pins[i];
    p_[i].ch1    = i*2;     // two channels per pump
    p_[i].ch2    = i*2 + 1;

    ledcSetup(p_[i].ch1, freq, res);
    ledcSetup(p_[i].ch2, freq, res);
    ledcAttachPin(p_[i].in1Pin, p_[i].ch1);
    ledcAttachPin(p_[i].in2Pin, p_[i].ch2);
    // Coast/off
    ledcWrite(p_[i].ch1, 0);
    ledcWrite(p_[i].ch2, 0);
  }
  logger.add("pump","init drv8871 ok");
}

void PumpControl::setDrive_(int idx, PumpDir dir, uint8_t duty) {
  auto& s = p_[idx];
  s.dir = dir;
  switch (dir) {
    case DIR_FWD:
      ledcWrite(s.ch1, duty);
      ledcWrite(s.ch2, 0);
      break;
    case DIR_REV:
      ledcWrite(s.ch1, 0);
      ledcWrite(s.ch2, duty);
      break;
    case DIR_BRAKE:
      // Both high ~ brake (use duty to modulate).
      ledcWrite(s.ch1, duty);
      ledcWrite(s.ch2, duty);
      break;
    case DIR_COAST:
    default:
      ledcWrite(s.ch1, 0);
      ledcWrite(s.ch2, 0);
      break;
  }
}

bool PumpControl::startPump(int idx, uint32_t durationMs, PumpDir dir) {
  if (idx<0 || idx>=NUM_PUMPS) return false;
  if (p_[idx].running) return false;
  auto& s = p_[idx];
  s.running=true;
  s.startMs=millis();
  s.durMs=durationMs;
  s.deliveredML += settings.pump[idx].mlPerSec * (durationMs/1000.0f);
  setDrive_(idx, dir, settings.pump[idx].duty);
  logger.add("pump", "start idx="+String(idx)+" durMs="+String(durationMs)+(dir==DIR_FWD?" fwd":" rev"));
  return true;
}

void PumpControl::stopPump(int idx, bool brake){
  if (idx<0 || idx>=NUM_PUMPS) return;
  auto& s = p_[idx];
  if (brake) setDrive_(idx, DIR_BRAKE, settings.pump[idx].duty);
  else       setDrive_(idx, DIR_COAST, 0);
  s.running=false;
  logger.add("pump", String(brake?"brake ":"stop ") + "idx="+String(idx));
}

void PumpControl::purge(int idx, uint32_t durationMs){
  // reverse direction run
  startPump(idx, durationMs, DIR_REV);
  logger.add("pump","purge idx="+String(idx)+" durMs="+String(durationMs));
}

void PumpControl::update() {
  const uint32_t now = millis();
  for (int i=0;i<NUM_PUMPS;i++){
    auto& s = p_[i];
    if (s.running && (now - s.startMs >= s.durMs)){
      stopPump(i); // coast stop by default
    }
  }
}
