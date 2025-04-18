#include <Arduino.h>
#include <WiFi.h>
#include <IPAddress.h>
#include <Preferences.h>

#include "m_wifi.h"
#include "m_display.h"

// factory network (used only when NVS is empty)
static const char *FACTORY_SSID = "teliaphobic";
static const char *FACTORY_PASS = "bdxCugDF";

// optional static IP for the factory network
static const bool USE_STATIC_IP = true;
static IPAddress  local_IP (192,168,1,111);
static IPAddress  gateway  (192,168,1,  1);
static IPAddress  subnet   (255,255,255,0);

// NVS store
static Preferences prefs;

// forward declarations (also in m_wifi.h)
bool  tryConnect(const String &ssid,
                 const String &pass,
                 uint32_t timeoutMs);
void  saveCredentials(const String &ssid,
                       const String &pass);

// load from NVS
static void loadCredentials(String &ssid, String &pass)
{
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
}

// runs in setup()
void setupWiFi()
{
  String cfgSsid, cfgPass;
  loadCredentials(cfgSsid, cfgPass);

  const char *ssid = cfgSsid.length() ? cfgSsid.c_str() : FACTORY_SSID;
  const char *pass = cfgSsid.length() ? cfgPass.c_str() : FACTORY_PASS;

  WiFi.mode(WIFI_STA);
  if (USE_STATIC_IP && !cfgSsid.length())
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
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("MatheoBugger-fallback");
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("AP IP = %s\n", ip.toString().c_str());
    currentText = "AP " + ip.toString();
  }

  displayOn = true;
  display.displayClear();
  display.displayScroll(currentText.c_str(),
                        PA_RIGHT, PA_SCROLL_RIGHT, 75);
}

// save to NVS
void saveCredentials(const String &ssid, const String &pass)
{
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.println("Credentials saved to NVS");
}

// one‑shot connection helper
bool tryConnect(const String &ssid,
                const String &pass,
                uint32_t timeoutMs)
{
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs)
    delay(250);

  return WiFi.status() == WL_CONNECTED;
}
