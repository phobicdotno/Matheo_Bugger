#include <WiFi.h>
#include "m_wifi.h"
#include "m_display.h"
#include "m_web.h"
#include "m_ota.h"

// The main loop: call each module's loop handlers.
void setup() {
  Serial.begin(115200);
  delay(2000);
  // Configure the physical button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  setupDisplay();
  setupWiFi();
  setupWeb();
  setupOTA();
}

void loop() {
  // Process web requests
  server.handleClient();
  // Update display animations
  handleDisplayLoop();
  // Check for physical button press
  handleButton();
}
