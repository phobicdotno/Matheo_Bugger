#pragma once
#include <Arduino.h>

void setupWiFi();
void handleWiFi();
bool tryConnect(const String &ssid,
                const String &pass,
                uint32_t timeoutMs);
void saveCredentials(const String &ssid,
                     const String &pass);
