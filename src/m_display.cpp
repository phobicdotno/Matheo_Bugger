#include "m_display.h"

// Define globals
MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
bool displayOn = true;
String currentText = "";
bool messageConfirmed = false;

// Initialize the display with flip effects for 180° rotation
void setupDisplay() {
  display.begin();
  display.setZone(0, 0, MAX_DEVICES - 1);
  display.setZoneEffect(0, true, PA_FLIP_UD);
  display.setZoneEffect(0, true, PA_FLIP_LR);
  display.setIntensity(5);
  display.displayClear();
}

// In the loop, let MD_Parola handle the animation and re-trigger scroll when animation completes.
void handleDisplayLoop() {
  if (displayOn) {
    if (display.displayAnimate()) {
      // Re-trigger scroll every time the animation completes
      display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
    }
  }
}

// Blink a given text (for example, "HELLO" or "OK") with a smooth fade effect.
void displayBlinkText(const char *txt) {
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

// Check the physical button; when pressed, blink "OK" and turn the display off.
void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    messageConfirmed = true;
    displayBlinkText("OK");
    displayOn = false;
    delay(300);
  }
}
