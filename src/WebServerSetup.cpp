#include "WebServerSetup.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "PumpControl.h"
#include "Scheduler.h"
#include "Settings.h"
#include "Logger.h"

AsyncWebServer server(80);

static String statusJson() {
  DynamicJsonDocument doc(1024 + NUM_PUMPS*128);
  doc["uptime_ms"] = millis();
  doc["schedule_interval_s"] = settings.scheduleIntervalSec;
  auto& arr = doc.createNestedArray("pumps");
  for (int i=0;i<NUM_PUMPS;i++){
    const auto& s = pumpCtl.state(i);
    JsonObject o = arr.createNestedObject();
    o["idx"] = i;
    o["running"] = s.running;
    o["start_ms"] = s.startMs;
    o["dur_ms"] = s.durMs;
    o["delivered_ml"] = s.deliveredML;
    o["ml_per_sec"] = settings.pump[i].mlPerSec;
    o["duty"] = settings.pump[i].duty;
    uint32_t due = scheduler.nextRunMs(i);
    o["next_run_in_s"] = (due==UINT32_MAX) ? -1 : (int)((due>millis())? (due-millis())/1000 : 0);
  }
  String out; serializeJson(doc, out); return out;
}

void web_begin() {
  if (!LittleFS.begin(true)) {
    logger.add("fs","LittleFS mount failed");
  } else {
    logger.add("fs","LittleFS mounted");
  }

  // Serve static UI from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // API: status
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send(200, "application/json", statusJson());
  });

  // API: actions
  server.on("/api/prime", HTTP_GET, [](AsyncWebServerRequest* r){
    if (!r->hasParam("pump") || !r->hasParam("sec")) { r->send(400,"text/plain","missing pump/sec"); return; }
    int idx = r->getParam("pump")->value().toInt();
    int sec = r->getParam("sec")->value().toInt();
    if (!pumpCtl.startPump(idx, max(1,sec)*1000UL, DIR_FWD)) { r->send(409,"text/plain","busy/invalid"); return; }
    r->send(200,"text/plain","ok");
  });

  server.on("/api/purge", HTTP_GET, [](AsyncWebServerRequest* r){
    if (!r->hasParam("pump") || !r->hasParam("sec")) { r->send(400,"text/plain","missing pump/sec"); return; }
    int idx = r->getParam("pump")->value().toInt();
    int sec = r->getParam("sec")->value().toInt();
    pumpCtl.purge(idx, max(1,sec)*1000UL);
    r->send(200,"text/plain","ok");
  });

  server.on("/api/stop", HTTP_GET, [](AsyncWebServerRequest* r){
    if (!r->hasParam("pump")) { r->send(400,"text/plain","missing pump"); return; }
    pumpCtl.stopPump(r->getParam("pump")->value().toInt());
    r->send(200,"text/plain","ok");
  });

  // API: settings
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* r){
    DynamicJsonDocument doc(1024+NUM_PUMPS*128);
    doc["schedule_interval_s"] = settings.scheduleIntervalSec;
    auto& arr = doc.createNestedArray("pumps");
    for (int i=0;i<NUM_PUMPS;i++){
      JsonObject o = arr.createNestedObject();
      o["idx"]=i; o["ml_per_sec"]=settings.pump[i].mlPerSec; o["duty"]=settings.pump[i].duty; o["offset_s"]=settings.scheduleOffsetSec[i];
    }
    String out; serializeJson(doc,out);
    r->send(200,"application/json",out);
  });

  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest* r){}, NULL,
    [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t, size_t) {
      DynamicJsonDocument doc(2048+NUM_PUMPS*128);
      DeserializationError err = deserializeJson(doc, data, len);
      if (err) { r->send(400,"text/plain","bad json"); return; }

      settings.scheduleIntervalSec = doc["schedule_interval_s"] | settings.scheduleIntervalSec;
      if (doc.containsKey("pumps")) {
        for (JsonObject p : doc["pumps"].as<JsonArray>()) {
          int idx = p["idx"] | -1;
          if (idx>=0 && idx<NUM_PUMPS) {
            settings.pump[idx].mlPerSec = p["ml_per_sec"] | settings.pump[idx].mlPerSec;
            settings.pump[idx].duty     = p["duty"]       | settings.pump[idx].duty;
            settings.scheduleOffsetSec[idx] = p["offset_s"] | settings.scheduleOffsetSec[idx];
          }
        }
      }
      settings.save();
      logger.add("cfg","settings saved");
      r->send(200,"text/plain","saved");
    });

  // Logs
  server.on("/logs.json", HTTP_GET, [](AsyncWebServerRequest* r){
    r->send(200, "application/json", logger.toJson(true));
  });
  server.on("/logs.csv", HTTP_GET, [](AsyncWebServerRequest* r){
    r->send(200, "text/csv", logger.toCSV());
  });

  // Fallback: if a route under / is missing, serve index.html (SPA)
  server.onNotFound([](AsyncWebServerRequest *request){
    if (LittleFS.exists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });

  server.begin();
  logger.add("web","server ready");
}
