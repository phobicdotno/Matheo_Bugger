#include <Arduino.h>
#include <WiFi.h>
#include <IPAddress.h>
#include <Preferences.h>

#include "m_wifi.h"
#include "m_display.h"      // gives currentText, displayOn, display

// ──────────────────────
// default “factory” Wi‑Fi
// ──────────────────────
static const char *FACTORY_SSID = "teliaphobic";
static const char *FACTORY_PASS = "bdxCugDF";

// optional static lease used with the factory SSID
static const bool USE_STATIC_IP = true;
static IPAddress  local_IP (192,168,1,111);
static IPAddress  gateway  (192,168,1,  1);
static IPAddress  subnet   (255,255,255,0);

// NVS namespace for stored credentials
static Preferences prefs;

// forward‑declared helpers (prototypes in m_wifi.h)
bool  tryConnect(const String &ssid,
                 const String &pass,
                 uint32_t timeoutMs);
void  saveCredentials(const String &ssid,
                       const String &pass);

// ──────────────────────
// load stored credentials (empty string if none)
// ──────────────────────
static void loadCredentials(String &ssid, String &pass)
{
  prefs.begin("wifi", true);        // read‑only
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
}

// ──────────────────────
// first‑boot / power‑up Wi‑Fi routine
// ──────────────────────
void setupWiFi()
{
  String cfgSsid, cfgPass;
  loadCredentials(cfgSsid, cfgPass);

  // choose which credentials to try
  const char *ssid = cfgSsid.length() ? cfgSsid.c_str() : FACTORY_SSID;
  const char *pass = cfgSsid.length() ? cfgPass.c_str() : FACTORY_PASS;

  WiFi.mode(WIFI_STA);
  if (USE_STATIC_IP && !cfgSsid.length())      // static IP only for factory creds
    WiFi.config(local_IP, gateway, subnet);

  WiFi.begin(ssid, pass);

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
    WiFi.mode(WIFI_AP_STA);              // keep STA alive for later attempts
    WiFi.softAP("MatheoBugger-fallback");
    IPAddress ip = WiFi.softAPIP();      // 192.168.4.1
    Serial.printf("AP IP = %s\n", ip.toString().c_str());
    currentText = "AP " + ip.toString();
  }

  displayOn = true;
  display.displayClear();
  display.displayScroll(currentText.c_str(),
                        PA_RIGHT, PA_SCROLL_RIGHT, 75);
}

// ──────────────────────
// saveCredentials – public helper
// ──────────────────────
void saveCredentials(const String &ssid, const String &pass)
{
  prefs.begin("wifi", false);           // read‑write
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.println("Credentials saved to NVS");
}

// ──────────────────────
// tryConnect – public helper
// ──────────────────────
bool tryConnect(const String &ssid,
                const String &pass,
                uint32_t timeoutMs)
{
  WiFi.disconnect(true, true);          // drop current STA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs)
    delay(250);

  return WiFi.status() == WL_CONNECTED;
}
