// src/m_web.cpp

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "m_web.h"
#include "m_display.h"

// Define the global web server instance
WebServer server(80);

// Handler for the root URL: returns the HTML UI
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

// Handler for setting a new message
void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
    messageConfirmed = false;
    scrolledOnce = false;     // re‑arm display scroll
    displayOn = true;
    display.displayClear();
    display.displayScroll(
      currentText.c_str(),
      PA_RIGHT, PA_SCROLL_RIGHT, 75
    );
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Handler for toggling the display on/off
void handleToggle() {
  displayOn = !displayOn;
  if (!displayOn) {
    displayBlinkText("BYE");
    display.displayClear();
    scrolledOnce = true;      // prevent any further scroll
  } else {
    displayBlinkText("HELLO");
    display.displayClear();
    scrolledOnce = false;     // allow scroll
    display.displayScroll(
      currentText.c_str(),
      PA_RIGHT, PA_SCROLL_RIGHT, 75
    );
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Setup all web routes and start the server
void setupWeb() {
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/toggle", handleToggle);
  server.on("/status", []() {
    String j = "{\"displayOn\":";
    j += (displayOn ? "true" : "false");
    j += ",\"messageConfirmed\":";
    j += (messageConfirmed ? "true" : "false");
    j += "}";
    server.send(200, "application/json", j);
  });
  server.begin();
}
