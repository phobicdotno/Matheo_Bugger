#pragma once
#include <Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include "m_display.h"

// Hardware definitions (must match your wiring)
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5
#define BUTTON_PIN 13

extern MD_Parola display;
extern bool displayOn;
extern String currentText;
extern bool messageConfirmed;

// Functions to set up and handle the display
void setupDisplay();
void handleDisplayLoop();
void displayBlinkText(const char *txt);
void handleButton();
