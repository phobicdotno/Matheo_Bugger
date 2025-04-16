#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5
#define BUTTON_PIN 13

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

bool messageConfirmed = false;
bool displayOn = true;
String currentText = "";

// WiFi Config
const char* ssid = "teliaphobic";
const char* password = "bdxCugDF";
IPAddress local_IP(192, 168, 1, 111);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// === HTML UI ===
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>❤️ Matheo Bugger ❤️</title>
  <style>
    body { font-family: sans-serif; padding: 1em; background-color: #f0f0f0; }
    h2 { text-align: center; }
    form { display: flex; flex-direction: column; align-items: center; }
    input[type="text"] {
      width: 90%; max-width: 300px; padding: 12px; font-size: 1em;
    }
    input[type="submit"] {
      margin-top: 0.5em; padding: 12px; font-size: 1em;
      background: #007bff; color: white; border: none; border-radius: 5px;
    }
    input[type="submit"]:hover { background-color: #0056b3; }
    #confirmStatus { font-weight: bold; text-align: center; }
  </style>
  <script>
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('displayState').innerText = data.displayOn ? "ON" : "OFF";
          const statusEl = document.getElementById('confirmStatus');
          statusEl.innerHTML = data.messageConfirmed ? "MESSAGE RECEIVED! ✅" : "Waiting for confirmation...";
        });
    }
    setInterval(updateStatus, 2000);
    window.onload = updateStatus;
  </script>
</head>
<body>
  <h2>❤️ Matheo Bugger ❤️</h2>
  <form action="/set" method="get" accept-charset="ISO-8859-1">
    <input type="text" name="text" placeholder="Type your message..." />
    <input type="submit" value="Update Message" />
  </form>
  <form action="/toggle" method="get">
    <input type="submit" value="Toggle Display" />
  </form>
  <p><strong>Display:</strong> <span id="displayState">...</span></p>
  <p id="confirmStatus">Loading...</p>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// === Set handler ===
void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
    messageConfirmed = false;

    displayOn = true;
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// === Toggle handler ===
void handleToggle() {
  displayOn = !displayOn;

  if (!displayOn) {
    display.displayClear();
    display.displayText("OK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();

    for (int i = 0; i < 3; i++) {
      for (int intensity = 15; intensity >= 2; intensity--) {
        display.setIntensity(intensity);
        delay(30);
      }
      for (int intensity = 2; intensity <= 15; intensity++) {
        display.setIntensity(intensity);
        delay(30);
      }
    }

    display.displayClear();
    display.setIntensity(5);
  } else {
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.begin();
  display.setIntensity(5);
  display.displayClear();

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout++ < 20) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    currentText = "WiFi OK: " + WiFi.localIP().toString();
  } else {
    currentText = "No WiFi!";
  }

  display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/toggle", handleToggle);
  server.on("/status", []() {
    String json = "{";
    json += "\"displayOn\":" + String(displayOn ? "true" : "false") + ",";
    json += "\"messageConfirmed\":" + String(messageConfirmed ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  if (displayOn) {
    if (display.displayAnimate()) {
      display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
    }
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    messageConfirmed = true;

    display.displayClear();
    display.displayText("OK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();

    for (int i = 0; i < 3; i++) {
      for (int intensity = 15; intensity >= 2; intensity--) {
        display.setIntensity(intensity);
        delay(30);
      }
      for (int intensity = 2; intensity <= 15; intensity++) {
        display.setIntensity(intensity);
        delay(30);
      }
    }

    display.displayClear();
    display.setIntensity(5);
    displayOn = false;

    delay(300); // debounce
  }
}
