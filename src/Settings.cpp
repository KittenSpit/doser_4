#include "Settings.h"

Settings settings;

void Settings::load() {
  for (int i=0;i<NUM_PUMPS;i++){
    String k = "p"+String(i);
    pump[i].mlPerSec = prefs_.getFloat((k+"_mls").c_str(), pump[i].mlPerSec);
    pump[i].duty     = prefs_.getUChar((k+"_duty").c_str(), pump[i].duty);
    scheduleOffsetSec[i] = prefs_.getUInt((k+"_ofs").c_str(), scheduleOffsetSec[i]);
  }
  scheduleIntervalSec = prefs_.getUInt("sched_int", scheduleIntervalSec);
}

void Settings::save() {
  for (int i=0;i<NUM_PUMPS;i++){
    String k = "p"+String(i);
    prefs_.putFloat((k+"_mls").c_str(), pump[i].mlPerSec);
    prefs_.putUChar((k+"_duty").c_str(), pump[i].duty);
    prefs_.putUInt((k+"_ofs").c_str(), scheduleOffsetSec[i]);
  }
  prefs_.putUInt("sched_int", scheduleIntervalSec);
}
