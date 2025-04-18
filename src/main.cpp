// src/main.cpp
#include <WiFi.h>
#include "m_wifi.h"
#include "m_display.h"
#include "m_web.h"
#include "m_ota.h"

void setup(){
  Serial.begin(115200);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  setupDisplay();
  setupWiFi();
  setupWeb();
  setupOTA();
}

void loop(){
  server.handleClient();
  handleDisplayLoop();
  handleButton();
}
