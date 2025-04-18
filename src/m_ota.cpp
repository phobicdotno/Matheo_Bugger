#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESP.h>
#include <LittleFS.h>
#include "m_ota.h"
#include "m_web.h"       // extern WebServer server
#include "m_wifi.h"      // tryConnect(), saveCredentials()
#include "m_display.h"   // currentText, displayOn, scrolledOnce, displayBlinkText()

void setupOTA() {
  // 1) Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("❌ LittleFS mount failed");
    return;
  }

  // 2) Serve UI files
  server.serveStatic("/fw",        LittleFS, "/index.html");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");

  // 3) Firmware upload
  server.on("/fw", HTTP_POST, []() {
    server.send(200, "text/plain; charset=utf-8",
      Update.hasError() ? "❌ Update Failed" : "✅ Update Success! Rebooting..."
    );
    delay(500);
    ESP.restart();
  }, []() {
    HTTPUpload& up = server.upload();
    if (up.status == UPLOAD_FILE_START) Update.begin();
    else if (up.status == UPLOAD_FILE_WRITE) Update.write(up.buf, up.currentSize);
    else if (up.status == UPLOAD_FILE_END)   Update.end(true);
  });

  // 4) Scan endpoint
  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();  // <– blocking, reliable
  
    String j = "{\"networks\":[";
    for (int i = 0; i < n; ++i) {
      if (i) j += ',';
      int ch = WiFi.channel(i);
      String band = (ch <= 14) ? "2.4GHz" : "5GHz";
      j += "\"" + WiFi.SSID(i) +
           " (" + String(WiFi.RSSI(i)) + " dBm, Ch " +
           String(ch) + ", " + band + ")\"";
    }
    j += "]}";
    server.send(200, "application/json", j);
  });

    // 5) Connect endpoint → SIMPLE FORM POST
  server.on("/connect", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
  
    Serial.printf("[CONNECT] Trying SSID='%s'\n", ssid.c_str());
  
    if (!tryConnect(ssid, pass, 15000)) {
      server.send(200, "text/plain; charset=utf-8", "❌ Connection failed (timeout)");
      return;
    }
  
    saveCredentials(ssid, pass);
    WiFi.softAPdisconnect(true);
    delay(200);  // let interface stabilize
  
    IPAddress ip = WiFi.localIP();
    Serial.printf("[CONNECT] Success! New IP: %s\n", ip.toString().c_str());
  
    currentText = ip.toString();
    displayOn = true;
    scrolledOnce = false;
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
  
    String response = "✅ IP changed to " + ip.toString() +
                      " — please switch to WiFi: " + ssid;
    server.send(200, "text/plain; charset=utf-8", response);
  });
  
}