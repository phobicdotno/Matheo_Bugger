#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// === MATRIX SETUP ===
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5
#define BUTTON_PIN 13
bool messageConfirmed = false;

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// === NETWORK CONFIG ===
const char* ssid = "teliaphobic";
const char* password = "bdxCugDF";

IPAddress local_IP(192, 168, 1, 111);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

String currentText = "";

bool displayOn = true;


// === HANDLERS ===
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>❤️ Matheo Bugger ❤️</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      padding: 1em;
      margin: 0;
      background-color: #f0f0f0;
    }
    h2 {
      text-align: center;
      font-size: 1.5em;
    }
    form {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin-bottom: 1.5em;
    }
    input[type="text"] {
      width: 90%;
      max-width: 300px;
      padding: 12px;
      font-size: 1em;
      margin: 0.5em 0;
    }
    input[type="submit"] {
      padding: 12px 20px;
      font-size: 1em;
      border: none;
      border-radius: 5px;
      background-color: #007bff;
      color: white;
      cursor: pointer;
      margin-top: 0.3em;
    }
    input[type="submit"]:hover {
      background-color: #0056b3;
    }
    p {
      text-align: center;
      font-size: 1.1em;
    }
    #confirmStatus {
      font-weight: bold;
    }
  </style>
  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById('displayState').innerText = data.displayOn ? "ON" : "OFF";
          const statusEl = document.getElementById('confirmStatus');
          if (data.messageConfirmed) {
            statusEl.innerHTML = "MESSAGE RECEIVED! ✅";
          } else {
            statusEl.innerText = "Waiting for confirmation...";
          }
        });
    }
    setInterval(updateStatus, 2000);
    window.onload = updateStatus;
  </script>
</head>
<body>
    <h2>❤️ Matheo Bugger ❤️</h2>

  <form action="/set" method="get" accept-charset="UTF-8">
    <input type="text" name="text" placeholder="Enter message..." />
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

void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
    messageConfirmed = false;  // Reset confirmation on new message
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}


void handleToggle() {
  displayOn = !displayOn;
  if (!displayOn) {
    display.displayClear();
  } else {
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // === MATRIX INIT ===
  display.begin();
  display.setIntensity(5);
  display.displayClear();
  display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);

  // === WIFI INIT ===
  WiFi.config(local_IP, gateway, subnet);
WiFi.begin(ssid, password);

Serial.print("Connecting to WiFi");
int wifiTimeout = 0;
while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
  delay(500);
  Serial.print(".");
  wifiTimeout++;
}

if (WiFi.status() == WL_CONNECTED) {
  IPAddress ip = WiFi.localIP();
  Serial.println("\n✅ WiFi connected!");
  Serial.print("📡 IP: ");
  Serial.println(ip);
  currentText = "Connected! IP: " + ip.toString();
} else {
  Serial.println("\n❌ WiFi failed to connect.");
  currentText = "No WiFi!";
}

// === WEB SERVER INIT ===
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
Serial.println("🌐 Web server started.");

  
}

void loop() {
  server.handleClient();

  if (displayOn) {
    if (display.displayAnimate()) {
      display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
    }
  }

  // Check for button press (active LOW)
  if (digitalRead(BUTTON_PIN) == LOW) {
    messageConfirmed = true;
    delay(300);  // Debounce
  }
}

