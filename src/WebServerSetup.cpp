#include "WebServerSetup.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "PumpControl.h"
#include "Scheduler.h"
#include "Settings.h"
#include "Logger.h"

AsyncWebServer server(80);

// Build status JSON (ArduinoJson v7 API)
static String statusJson() {
  JsonDocument doc;
  doc["uptime_ms"] = millis();
  doc["schedule_interval_s"] = settings.scheduleIntervalSec;

  JsonArray arr = doc["pumps"].to<JsonArray>();
  for (int i = 0; i < NUM_PUMPS; i++) {
    const auto& s = pumpCtl.state(i);
    JsonObject o = arr.add<JsonObject>();
    o["idx"] = i;
    o["running"] = s.running;
    o["start_ms"] = s.startMs;
    o["dur_ms"] = s.durMs;
    o["delivered_ml"] = s.deliveredML;
    o["ml_per_sec"] = settings.pump[i].mlPerSec;
    o["duty"] = settings.pump[i].duty;
    uint32_t due = scheduler.nextRunMs(i);
    o["next_run_in_s"] = (due == UINT32_MAX) ? -1
                         : (int)((due > millis()) ? (due - millis()) / 1000 : 0);
  }

  String out; serializeJson(doc, out);
  return out;
}

void web_begin() {
  if (!LittleFS.begin(true)) logger.add("fs", "LittleFS mount failed");
  else                       logger.add("fs", "LittleFS mounted");

  // CORS (optional but helps with tools)
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");

  // -------- API ROUTES FIRST (accept ANY method & trailing slashes) --------
  auto sendStatus = [](AsyncWebServerRequest* req){
    req->send(200, "application/json", statusJson());
  };
  server.on("/api/status",  HTTP_ANY, sendStatus);
  server.on("/api/status/", HTTP_ANY, sendStatus);

  server.on("/api/prime", HTTP_ANY, [](AsyncWebServerRequest* r){
    if (!r->hasParam("pump") || !r->hasParam("sec")) { r->send(400,"text/plain","missing pump/sec"); return; }
    int idx = r->getParam("pump")->value().toInt();
    int sec = r->getParam("sec")->value().toInt();
    if (!pumpCtl.startPump(idx, max(1,sec)*1000UL, DIR_FWD)) { r->send(409,"text/plain","busy/invalid"); return; }
    r->send(200,"text/plain","ok");
  });

  server.on("/api/purge", HTTP_ANY, [](AsyncWebServerRequest* r){
    if (!r->hasParam("pump") || !r->hasParam("sec")) { r->send(400,"text/plain","missing pump/sec"); return; }
    int idx = r->getParam("pump")->value().toInt();
    int sec = r->getParam("sec")->value().toInt();
    pumpCtl.purge(idx, max(1,sec)*1000UL);
    r->send(200,"text/plain","ok");
  });

  server.on("/api/stop", HTTP_ANY, [](AsyncWebServerRequest* r){
    if (!r->hasParam("pump")) { r->send(400,"text/plain","missing pump"); return; }
    pumpCtl.stopPump(r->getParam("pump")->value().toInt());
    r->send(200,"text/plain","ok");
  });

  // Settings (GET/POST/OPTIONS)
  server.on("/api/settings", HTTP_OPTIONS, [](AsyncWebServerRequest* r){ r->send(204); });
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* r){
    JsonDocument doc;
    doc["schedule_interval_s"] = settings.scheduleIntervalSec;
    JsonArray arr = doc["pumps"].to<JsonArray>();
    for (int i = 0; i < NUM_PUMPS; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["idx"] = i;
      o["ml_per_sec"] = settings.pump[i].mlPerSec;
      o["duty"] = settings.pump[i].duty;
      o["offset_s"] = settings.scheduleOffsetSec[i];
    }
    String out; serializeJson(doc, out);
    r->send(200, "application/json", out);
  });
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest* r){}, NULL,
    [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t, size_t){
      JsonDocument doc;
      if (deserializeJson(doc, data, len)) { r->send(400,"text/plain","bad json"); return; }
      if (doc["schedule_interval_s"].is<uint32_t>()) settings.scheduleIntervalSec = doc["schedule_interval_s"].as<uint32_t>();
      if (doc["pumps"].is<JsonArray>()) {
        for (JsonObject p : doc["pumps"].as<JsonArray>()) {
          int idx = p["idx"] | -1; if (idx<0 || idx>=NUM_PUMPS) continue;
          if (p["ml_per_sec"].is<float>())   settings.pump[idx].mlPerSec = p["ml_per_sec"].as<float>();
          if (p["duty"].is<uint8_t>())       settings.pump[idx].duty     = p["duty"].as<uint8_t>();
          if (p["offset_s"].is<uint32_t>())  settings.scheduleOffsetSec[idx] = p["offset_s"].as<uint32_t>();
        }
      }
      settings.save();
      logger.add("cfg","settings saved");
      r->send(200,"text/plain","saved");
    });

  // Logs
  server.on("/logs.json", HTTP_ANY, [](AsyncWebServerRequest* r){
    r->send(200, "application/json", logger.toJson(true));
  });
  server.on("/logs.csv", HTTP_ANY, [](AsyncWebServerRequest* r){
    r->send(200, "text/csv", logger.toCSV());
  });

  // Health probe some clients hit
  server.on("/status", HTTP_ANY, [](AsyncWebServerRequest* req){ req->send(204); });

  // -------- STATIC UI LAST (so it never intercepts /api/*) --------
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Keep /api/* off FS fallback
  server.onNotFound([](AsyncWebServerRequest *req){
    String u = req->url();
    if (u.startsWith("/api/") || u.startsWith("/logs")) {
      req->send(404, "text/plain", "API route not found"); return;
    }
    if (LittleFS.exists("/index.html")) req->send(LittleFS, "/index.html", "text/html");
    else req->send(404, "text/plain", "Not found");
  });

  server.begin();
  logger.add("web", "server ready");
}
