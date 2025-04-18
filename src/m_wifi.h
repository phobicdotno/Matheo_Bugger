#pragma once
#include <Arduino.h>

void setupWiFi();

// add these forward‑declarations
bool tryConnect(const String &ssid, const String &pass, uint32_t timeoutMs);
void saveCredentials(const String &ssid, const String &pass);
