#include "Logger.h"

Logger logger;

void Logger::add(const String& evt, const String& msg) {
  buf_[head_] = { millis(), evt, msg };
  head_ = (head_ + 1) % LOG_CAPACITY;
  if (count_ < LOG_CAPACITY) count_++;
}

void Logger::jsonEsc(String& out, const String& s) {
  for (size_t i=0; i<s.length(); ++i) {
    char c = s[i];
    switch (c) {
      case '\"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) { char b[7]; snprintf(b,7,"\\u%04X",(unsigned)c); out += b; }
        else out += c;
    }
  }
}

String Logger::toJson(bool pretty) const {
  const char* nl  = pretty? "\n":"";
  const char* sp1 = pretty? "  ":"";
  const char* sp2 = pretty? "    ":"";
  String out; out.reserve(count_*48);
  out += "[";
  if (pretty && count_) out += nl;
  for (int i=0;i<count_;++i){
    const LogEntry& e = buf_[idxOldest(i)];
    if (pretty) out += sp1;
    out += "{";
    if (pretty) out += nl, out += sp2;
    out += "\"ts\":" + String(e.ts) + ",";
    if (pretty) out += nl, out += sp2;
    out += "\"event\":\""; jsonEsc(out,e.event); out += "\",";
    if (pretty) out += nl, out += sp2;
    out += "\"msg\":\"";   jsonEsc(out,e.msg);   out += "\"";
    if (pretty) out += nl, out += sp1;
    out += "}";
    if (i<count_-1) out += ",";
    if (pretty) out += nl;
  }
  out += "]";
  return out;
}

String Logger::csvEsc(const String& s) {
  String o = s; o.replace("\"","\"\"");
  if (o.indexOf(',')!=-1 || o.indexOf('"')!=-1 || o.indexOf('\n')!=-1 || o.indexOf('\r')!=-1) o = "\"" + o + "\"";
  return o;
}

String Logger::toCSV() const {
  String out = "ts,event,msg\n";
  for (int i=0;i<count_;++i){
    const LogEntry& e = buf_[idxOldest(i)];
    out += String(e.ts) + "," + csvEsc(e.event) + "," + csvEsc(e.msg) + "\n";
  }
  return out;
}

void Logger::clear(){ head_=0; count_=0; }
