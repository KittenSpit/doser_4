#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <LittleFS.h>
#include "Settings.h"
#include "PumpControl.h"
#include "Scheduler.h"
#include "Display.h"
#include "WebServerSetup.h"
#include "Logger.h"






// === EDIT THESE ===
// DRV8871 pin map (IN1/IN2 per pump)
static const uint8_t PUMP_IN1[NUM_PUMPS] = { 25, 26, 27 };  // example
static const uint8_t PUMP_IN2[NUM_PUMPS] = { 32, 33, 14 };  // example

// WiFi
const char* WIFI_SSID = "PHD1 2.4";
const char* WIFI_PSK  = "Andrew1Laura2";

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Serial Start");
  logger.add("boot","starting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  uint32_t t0 = millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-t0<15000) { delay(200); Serial.print("."); }
  Serial.println();
  Serial.println(WiFi.localIP().toString());
  logger.add("wifi", WiFi.isConnected()? "connected: "+WiFi.localIP().toString() : "wifi failed");

  settings.begin();
  pumpCtl.begin(PUMP_IN1, PUMP_IN2, NUM_PUMPS);
  scheduler.begin();
  Wire.begin(); // change pins if needed: Wire.begin(21,22)

  if (!displayUI.begin()) logger.add("oled","init fail"); else logger.add("oled","init ok");

  web_begin();
}

void loop() {
  pumpCtl.update();
  scheduler.update();
  displayUI.update();
}
