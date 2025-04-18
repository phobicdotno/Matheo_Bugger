#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "m_wifi.h"
#include "m_display.h"

// ---------- storage ----------
static Preferences prefs;

static void loadCredentials(String &ssid, String &pass)
{
  prefs.begin("wifi", true);          // read‑only
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
}

void saveCredentials(const String &ssid, const String &pass)
{
  prefs.begin("wifi", false);         // read‑write
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}
// ---------- end storage ----------

void setupWiFi()
{
  String savedSsid, savedPass;
  loadCredentials(savedSsid, savedPass);

  WiFi.mode(WIFI_STA);
  if (savedSsid.length())
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());

  Serial.print("Connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
    Serial.print('.');
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    Serial.printf("\nWiFi OK: %s\n", ip.toString().c_str());
    currentText = "WiFi OK: " + ip.toString();
  } else {
    Serial.println("\nNo WiFi – starting fallback AP");
    WiFi.mode(WIFI_AP_STA);                  // keep STA alive for later
    WiFi.softAP("ESP32-fallback");
    IPAddress ip = WiFi.softAPIP();          // 192.168.4.1
    Serial.printf("AP IP = %s\n", ip.toString().c_str());
    currentText = "AP " + ip.toString();
  }

  displayOn = true;
  display.displayClear();
  display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
}

// helper for OTA page: fires a fresh connect attempt
bool tryConnect(const String &ssid, const String &pass, uint32_t timeoutMs)
{
  WiFi.disconnect(true, true);        // true,true erases old STA cfg in RAM only
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}
