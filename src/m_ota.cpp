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

  // 2) Serve UI files from /data
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/fw.html", LittleFS, "/fw.html");
  server.serveStatic("/status.html", LittleFS, "/status.html");

  // 3) Redirect pretty routes to .html files
  server.on("/fw", HTTP_GET, []() {
    server.sendHeader("Location", "/fw.html", true);
    server.send(302, "text/plain", "");
  });
  server.on("/status", HTTP_GET, []() {
    server.sendHeader("Location", "/status.html", true);
    server.send(302, "text/plain", "");
  });

  // 4) Firmware upload
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

  // 5) WiFi Scan
  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();  // blocking scan (works reliably)
  
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

  // 6) WiFi Connect
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
    delay(200);  // allow network settle

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
