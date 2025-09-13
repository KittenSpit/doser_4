#pragma once
#include <Arduino.h>

#ifndef LOG_CAPACITY
#define LOG_CAPACITY 128
#endif

struct LogEntry {
  uint32_t ts;
  String   event;
  String   msg;
};

class Logger {
public:
  void add(const String& evt, const String& msg = "");
  String toJson(bool pretty=false) const;
  String toCSV() const;
  void clear();
  int size() const { return count_; }

private:
  LogEntry buf_[LOG_CAPACITY];
  int head_ = 0, count_ = 0;

  static void jsonEsc(String& out, const String& s);
  static String csvEsc(const String& s);
  inline int idxOldest(int i) const {
    return (head_ - count_ + i + LOG_CAPACITY) % LOG_CAPACITY;
  }
};

extern Logger logger;
