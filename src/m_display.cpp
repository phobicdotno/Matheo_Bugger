#include "m_display.h"
#include <WiFi.h>

MD_Parola display(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
bool displayOn    = true;
String currentText;
bool messageConfirmed = false;
bool scrolledOnce    = false;

void setupDisplay() {
  display.begin();
  display.setZone(0,0,MAX_DEVICES-1);
  display.setZoneEffect(0,true,PA_FLIP_UD);
  display.setZoneEffect(0,true,PA_FLIP_LR);
  display.setIntensity(5);
  display.displayClear();
}

void handleDisplayLoop() {
  if (!displayOn) return;
  if (display.displayAnimate()) {
    if (!scrolledOnce) {
      display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
      scrolledOnce = true;
    }
  }
}

void displayBlinkText(const char *txt) {
  display.displayClear();
  display.displayText(txt, PA_CENTER, 0,0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
  for (int i=0;i<3;i++){
    for (int d=15;d>=2;d--){ display.setIntensity(d); delay(30); }
    for (int d=2; d<=15; d++){ display.setIntensity(d); delay(30); }
  }
  display.displayClear();
  display.setIntensity(5);
}

void handleButton() {
  static unsigned long pressStart = 0;
  static bool wasPressed = false;
  static bool longDone = false;

  bool pressed = (digitalRead(BUTTON_PIN)==LOW);

  if (pressed && !wasPressed) {
    pressStart = millis();
    longDone  = false;
  }
  if (pressed && !longDone && millis() - pressStart >= 2500) {
    // Long‑press: show IP
    IPAddress ip = WiFi.localIP();
    Serial.printf("Long‑press IP: %s\n", ip.toString().c_str());
    currentText  = ip.toString();
    displayOn    = true;
    scrolledOnce = false;
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
    longDone = true;
  }
  if (!pressed && wasPressed && !longDone) {
    // Short‑press: confirm
    messageConfirmed = true;
    displayBlinkText("OK");
    displayOn = false;
  }

  wasPressed = pressed;
}
