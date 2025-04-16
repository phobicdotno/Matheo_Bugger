#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
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
      margin: 0 auto;
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
      margin-top: 1em;
      padding: 12px;
      font-size: 1em;
      border: none;
      border-radius: 5px;
      color: white;
      cursor: pointer;
      width: 160px;
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
          statusEl.innerHTML = data.messageConfirmed ? "MESSAGE RECEIVED! ✅" : "Waiting for confirmation...";
          btn.className = data.displayOn ? "on" : "off";
          btn.innerText = data.displayOn ? "Display ON" : "Display OFF";
        });
    }

    function toggleDisplay() {
      fetch('/toggle').then(() => updateStatus());
    }

    setInterval(updateStatus, 2000);
    window.onload = updateStatus;
  </script>
</head>
<body>
  <div class="wrapper">
    <h2>❤️ Matheo Bugger ❤️</h2>
    <form action="/set" method="get" accept-charset="ISO-8859-1">
      <input type="text" name="text" placeholder="Type your message..." />
      <input type="submit" value="Update Message" />
    </form>
    <button id="toggleBtn" onclick="toggleDisplay()" class="on">Display ON</button>
    <p id="confirmStatus">Loading...</p>
  </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
    messageConfirmed = false;

    // Always turn display on and scroll
    displayOn = true;
    display.displayClear();
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleToggle() {
  displayOn = !displayOn;

  if (!displayOn) {
    // TURN OFF — Blink OK
    display.displayClear();
    display.displayText("OK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();
    for (int i = 0; i < 3; i++) {
      for (int intensity = 15; intensity >= 2; intensity--) {
        display.setIntensity(intensity); delay(30);
      }
      for (int intensity = 2; intensity <= 15; intensity++) {
        display.setIntensity(intensity); delay(30);
      }
    }
    display.displayClear();
    display.setIntensity(5);
  } else {
    // TURN ON — Blink HELLO only (no scroll)
    display.displayClear();
    display.displayText("HELLO", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();
    for (int i = 0; i < 3; i++) {
      for (int intensity = 15; intensity >= 2; intensity--) {
        display.setIntensity(intensity); delay(30);
      }
      for (int intensity = 2; intensity <= 15; intensity++) {
        display.setIntensity(intensity); delay(30);
      }
    }
    display.displayClear();
    display.setIntensity(5);
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
  while (WiFi.status() != WL_CONNECTED && timeout++ < 20) delay(500);

  currentText = WiFi.status() == WL_CONNECTED ? "WiFi OK: " + WiFi.localIP().toString() : "No WiFi!";
  display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/toggle", handleToggle);
  server.on("/status", []() {
    String json = "{\"displayOn\":" + String(displayOn ? "true" : "false") +
                  ",\"messageConfirmed\":" + String(messageConfirmed ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // OTA /fw
  server.on("/fw", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<title>Firmware Update</title>
<style>
body { font-family: sans-serif; text-align: center; padding: 2em; background: #f0f0f0; }
input[type="submit"] {
  margin-top: 1em; padding: 12px 24px; font-size: 1em;
  background: #007bff; color: white; border: none; border-radius: 5px;
}
input[type="submit"]:hover { background-color: #0056b3; }
#progressWrapper {
  margin: 2em auto 1em;
  width: 90%; max-width: 300px;
  height: 20px;
  background: #ddd; border-radius: 10px; overflow: hidden;
}
#progressBar {
  width: 0%; height: 100%;
  background: #28a745; transition: width 0.3s ease;
}
#spinner {
  margin: 1em auto;
  width: 32px; height: 32px;
  border: 4px solid #ccc;
  border-top: 4px solid #007bff;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}
@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}
</style></head><body>
<h2>Firmware Update</h2>
<form id="uploadForm">
  <input type="file" id="file" required>
  <input type="submit" value="Upload Firmware">
</form>
<div id="progressWrapper" style="display:none;"><div id="progressBar"></div></div>
<div id="status" style="margin-top: 1em; font-weight: bold;">Waiting...</div>
<div id="reconnect" style="display:none;"><div id="spinner"></div><div style="margin-top: 1em;">Reconnecting...</div></div>
<script>
const form = document.getElementById('uploadForm');
const fileInput = document.getElementById('file');
const status = document.getElementById('status');
const progressWrapper = document.getElementById('progressWrapper');
const progressBar = document.getElementById('progressBar');
const reconnect = document.getElementById('reconnect');

form.addEventListener('submit', e => {
  e.preventDefault();
  const file = fileInput.files[0];
  if (!file) return;

  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/fw", true);

  xhr.upload.onprogress = e => {
    if (e.lengthComputable) {
      progressWrapper.style.display = "block";
      const percent = Math.round((e.loaded / e.total) * 100);
      progressBar.style.width = percent + "%";
    }
  };

  xhr.onload = () => {
    status.innerHTML = xhr.status == 200
      ? "Update Success! Rebooting..."
      : "❌ Upload failed";
    reconnect.style.display = "block";
    setTimeout(() => {
      const check = setInterval(() => {
        fetch("/")
          .then(r => { if (r.ok) { clearInterval(check); window.location.href = "/"; } })
          .catch(() => {});
      }, 1000);
    }, 3000);
  };

  const formData = new FormData();
  formData.append("update", file);
  xhr.send(formData);
  status.innerText = "Uploading...";
});
</script></body></html>
    )rawliteral");
  });

  server.on("/fw", HTTP_POST, []() {
    server.send(200, "text/plain; charset=utf-8",
      Update.hasError() ? "❌ Update Failed" : "✅ Update Success! Rebooting...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Update.begin();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      Update.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();

  if (displayOn && display.displayAnimate()) {
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    messageConfirmed = true;
    display.displayClear();
    display.displayText("OK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();
    for (int i = 0; i < 3; i++) {
      for (int intensity = 15; intensity >= 2; intensity--) {
        display.setIntensity(intensity); delay(30);
      }
      for (int intensity = 2; intensity <= 15; intensity++) {
        display.setIntensity(intensity); delay(30);
      }
    }
    display.displayClear();
    display.setIntensity(5);
    displayOn = false;
    delay(300);
  }
}
