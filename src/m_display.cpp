#include "m_display.h"

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

void handleButton()
{
  if (digitalRead(BUTTON_PIN) == LOW) {
    messageConfirmed = true;
    displayBlinkText("OK");
    displayOn = false;
    hasScrolled = true;           // allow next /set to scroll again
    delay(300);
  }
}
