// src/m_wifi.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <IPAddress.h>
#include <Preferences.h>
#include "m_wifi.h"
#include "m_display.h"

static const char* FACTORY_SSID = "teliaphobic";
static const char* FACTORY_PASS = "bdxCugDF";
static const bool USE_STATIC_IP = true;
static IPAddress local_IP(192,168,1,111);
static IPAddress gateway(192,168,1,1);
static IPAddress subnet(255,255,255,0);
static Preferences prefs;

void setupWiFi() {
  String s,p;
  prefs.begin("wifi", true);
  s = prefs.getString("ssid",""); p = prefs.getString("pass","");
  prefs.end();

  const char* ss = s.length()?s.c_str():FACTORY_SSID;
  const char* pp = s.length()?p.c_str():FACTORY_PASS;

  WiFi.mode(WIFI_STA);
  if (USE_STATIC_IP && s.length()==0) WiFi.config(local_IP,gateway,subnet);
  else WiFi.config(0U,0U,0U);

  WiFi.begin(ss,pp);
  Serial.print("Connecting");
  uint32_t t0=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-t0<10000){
    Serial.print('.'); delay(500);
  }
  if(WiFi.status()==WL_CONNECTED){
    IPAddress ip=WiFi.localIP();
    Serial.printf("\nWiFi OK: %s\n",ip.toString().c_str());
    currentText="WiFi OK: "+ip.toString();
    scrolledOnce=false;
  } else {
    Serial.println("\nNo WiFi – fallback AP");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("MatheoBugger-fallback");
    IPAddress ip=WiFi.softAPIP();
    Serial.printf("AP IP = %s\n",ip.toString().c_str());
    currentText="AP "+ip.toString();
    scrolledOnce=false;
  }
  displayOn=true;
  display.displayClear();
  display.displayScroll(currentText.c_str(),PA_RIGHT,PA_SCROLL_RIGHT,75);
}

void saveCredentials(const String& ssid,const String& pass){
  if(ssid.isEmpty()||pass.isEmpty())return;
  prefs.begin("wifi",false);
  prefs.putString("ssid",ssid);
  prefs.putString("pass",pass);
  prefs.end();
  Serial.println("Credentials saved");
}

bool tryConnect(const String& ssid,const String& pass,uint32_t tm){
  WiFi.disconnect(true,true);
  WiFi.config(0U,0U,0U);
  WiFi.begin(ssid.c_str(),pass.c_str());
  uint32_t t0=millis();
  while(WiFi.status()!=WL_CONNECTED&&millis()-t0<tm)delay(250);
  return WiFi.status()==WL_CONNECTED;
}
