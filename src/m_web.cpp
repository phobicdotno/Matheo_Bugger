#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "m_web.h"
#include "m_display.h"

WebServer server(80);
static unsigned int connectionCount = 0;

// === / (HTML UI) ===
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>❤️ Matheo Bugger ❤️</title>
  <style>
    body { font-family: sans-serif; background: #f0f0f0; padding: 1em; }
    .wrapper {
      display: flex;
      flex-direction: column;
      align-items: center;
      max-width: 500px;
      margin: auto;
    }
    form { width: 100%; display: flex; flex-direction: column; gap: 0.5em; }
    input[type="text"] {
      width: 100%; padding: 12px; font-size: 1em;
    }
    input[type="submit"] {
      padding: 12px; font-size: 1em;
      background: #007bff; color: white; border: none; border-radius: 5px;
    }
    input[type="submit"]:hover { background-color: #0056b3; }
    #toggleBtn {
      margin-top: 1em; padding: 12px; font-size: 1em;
      border: none; border-radius: 5px; color: white;
      cursor: pointer; width: 160px;
    }
    #toggleBtn.on { background-color: #007bff; }
    #toggleBtn.off { background-color: #6c757d; }
    #confirmStatus { font-weight: bold; text-align: center; margin-top: 1em; }
  </style>
  <script>
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          const btn = document.getElementById('toggleBtn');
          const statusEl = document.getElementById('confirmStatus');
          statusEl.textContent = data.messageConfirmed
            ? 'MESSAGE RECEIVED! ✅'
            : 'Waiting for confirmation...';
          btn.className = data.displayOn ? 'on' : 'off';
          btn.textContent = data.displayOn ? 'Display ON' : 'Display OFF';
          console.log("IP: " + data.ip + ", Signal: " + data.signal + " dBm");
        });
    }
    function toggleDisplay() {
      fetch('/toggle').then(updateStatus);
    }
    window.addEventListener('load', updateStatus);
    setInterval(updateStatus, 2000);
  </script>
</head>
<body>
  <div class="wrapper">
    <h2>❤️ Matheo Bugger ❤️</h2>
    <form action="/set" method="get" accept-charset="ISO-8859-1">
      <input type="text" name="text" placeholder="Type your message..." />
      <input type="submit" value="Update Message" />
    </form>
    <button id="toggleBtn" onclick="toggleDisplay()">Display ON</button>
    <p id="confirmStatus">Loading...</p>
  </div>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// === /set ===
void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
    messageConfirmed = false;
    scrolledOnce = false;
    displayOn = true;
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// === /toggle ===
void handleToggle() {
  displayOn = !displayOn;
  if (!displayOn) {
    displayBlinkText("BYE");
    display.displayClear();
    scrolledOnce = true;
  } else {
    displayBlinkText("HELLO");
    display.displayClear();
    scrolledOnce = false;
    display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// === /status (with extended info) ===
void handleStatus() {
  connectionCount++;

  String json = "{";
  json += "\"displayOn\":" + String(displayOn ? "true" : "false") + ",";
  json += "\"messageConfirmed\":" + String(messageConfirmed ? "true" : "false") + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"signal\":" + String(WiFi.RSSI()) + ",";
  json += "\"clientCount\":" + String(connectionCount) + ",";

  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  json += "\"freeSpace\":" + String(total - used) + ",";
  json += "\"totalSpace\":" + String(total);
  json += "}";

  server.send(200, "application/json", json);
}

// === Setup routes ===
void setupWeb() {
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  server.begin();
}
  