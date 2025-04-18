#include "m_display.h"
#include <WiFi.h>

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
bool displayOn = true;
String currentText = "";
bool messageConfirmed = false;
bool hasScrolled = false;

void setupDisplay()
{
  display.begin();
  display.setZone(0, 0, MAX_DEVICES - 1);
  display.setZoneEffect(0, true, PA_FLIP_UD);
  display.setZoneEffect(0, true, PA_FLIP_LR);
  display.setIntensity(5);
  display.displayClear();
}

void handleDisplayLoop()
{
  if (displayOn) {
    if (display.displayAnimate()) {
      if (!hasScrolled)
        display.displayScroll(currentText.c_str(),
                              PA_RIGHT, PA_SCROLL_RIGHT, 75);
      hasScrolled = true;
    }
  }
}

void displayBlinkText(const char *txt)
{
  display.displayClear();
  display.displayText(txt, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
  for (int i = 0; i < 3; i++) {
    for (int intensity = 15; intensity >= 2; intensity--) {
      display.setIntensity(intensity);
      delay(30);
    }
    for (int intensity = 2; intensity <= 15; intensity++) {
      display.setIntensity(intensity);
      delay(30);
    }
  }
  display.displayClear();
  display.setIntensity(5);
}

// ─────────────────────────────────────────────────────────────
// handleButton() ­– short press = confirm, long press (≥5 s) = show IP
// ─────────────────────────────────────────────────────────────
void handleButton()
{
  static unsigned long pressStart = 0;
  static bool wasPressed        = false;

  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  // edge: button just went down
  if (pressed && !wasPressed) {
    pressStart = millis();
  }

  // edge: button just went up
  if (!pressed && wasPressed) {
    unsigned long held = millis() - pressStart;

    if (held < 5000) {                 // short press  → confirm message
      messageConfirmed = true;
      displayBlinkText("OK");
      displayOn = false;
      hasScrolled = true;              // allow next /set to scroll
    }                                  // long press handled while still down
  }

  // while button is held down
  if (pressed && (millis() - pressStart >= 5000) && !displayOn) {
    // show IP **once** after 5‑s hold
    IPAddress ip = WiFi.localIP();
    currentText  = ip.toString();
    displayOn    = true;
    hasScrolled  = false;
    display.displayClear();
    display.displayScroll(currentText.c_str(),
                          PA_RIGHT, PA_SCROLL_RIGHT, 75);
  }

  wasPressed = pressed;
}

