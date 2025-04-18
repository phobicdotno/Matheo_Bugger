#include <Arduino.h>
#include <WiFi.h>
#include <IPAddress.h>
#include <Preferences.h>
#include "m_wifi.h"
#include "m_display.h"

// NVS namespace
static Preferences prefs;

// Factory fallback network (only used when no credentials are stored)
static const char* FACTORY_SSID = "teliaphobic";
static const char* FACTORY_PASS = "bdxCugDF";

// Optional static IP when using the factory network
static const bool USE_STATIC_IP = true;
static IPAddress local_IP(192,168,1,111);
static IPAddress gateway (192,168,1,1);
static IPAddress subnet  (255,255,255,0);

// Load stored credentials (empty strings if none)
static void loadCredentials(String &ssid, String &pass) {
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
}

// Public: called in setup()
void setupWiFi() {
  String ssid, pass;
  loadCredentials(ssid, pass);

  // Choose either stored or factory credentials
  const char* net_ssid = ssid.length() ? ssid.c_str() : FACTORY_SSID;
  const char* net_pass = ssid.length() ? pass.c_str() : FACTORY_PASS;

  WiFi.mode(WIFI_STA);
  if (USE_STATIC_IP && ssid.length() == 0) {
    WiFi.config(local_IP, gateway, subnet);
  } else {
    WiFi.config(0U, 0U, 0U);  // DHCP
  }

  WiFi.begin(net_ssid, net_pass);
  Serial.print("Connecting");
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    Serial.print('.');
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    Serial.printf("\nWiFi OK: %s\n", ip.toString().c_str());
    currentText = "WiFi OK: " + ip.toString();
  } else {
    Serial.println("\nNo WiFi – starting fallback AP");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("MatheoBugger-fallback");
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("AP IP = %s\n", ip.toString().c_str());
    currentText = "AP " + ip.toString();
  }

  displayOn = true;
  display.displayClear();
  display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
}

// Public: save credentials after a successful connect
void saveCredentials(const String &ssid, const String &pass) {
  if (ssid.isEmpty() || pass.isEmpty()) {
    Serial.println("saveCredentials: empty field – skipped");
    return;
  }
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.println("Credentials saved");
}

// Public: try to join a network (used by /connect handler)
bool tryConnect(const String &ssid, const String &pass, uint32_t timeoutMs) {
  WiFi.disconnect(true, true);
  WiFi.config(0U, 0U, 0U);  // force DHCP
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }
  Serial.printf("tryConnect status=%d\n", WiFi.status());
  return WiFi.status() == WL_CONNECTED;
}
