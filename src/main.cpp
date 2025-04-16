#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <Update.h>

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
    <input type="text" name="text" placeholder="Type here" />
    <input type="submit" value="Update Message" />
  </form>
  <form action="/toggle" method="get">
    <input type="submit" value="Toggle Display" />
  </form>
  <p style="text-align: center;"><strong>Display:</strong> <span id="displayState">...</span></p>
  <p id="confirmStatus">Loading...</p>
</body>
</html>
)rawliteral";

  server.send(200, "text/plain; charset=utf-8", Update.hasError() ? "❌ Update Failed" : "✅ Update Success! Rebooting...");

}

// === Set handler ===
void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
    messageConfirmed = false;
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
  } else {
    display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Let USB settle (for dev mode)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.begin();
  display.setIntensity(5);
  display.displayClear();

  // WiFi Init
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

  server.on("/fw", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Firmware Update</title>
        <style>
          body {
            font-family: sans-serif;
            padding: 1em;
            background-color: #f0f0f0;
          }
          h2 {
            text-align: center;
          }
          form {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin-top: 2em;
          }
          input[type="file"] {
            margin-bottom: 1em;
          }
          input[type="submit"] {
            padding: 12px;
            font-size: 1em;
            background: #007bff;
            color: white;
            border: none;
            border-radius: 5px;
          }
          input[type="submit"]:hover {
            background-color: #0056b3;
          }
          #progressWrapper {
            margin-top: 1.5em;
            width: 90%;
            max-width: 300px;
            height: 20px;
            background: #ddd;
            border-radius: 10px;
            overflow: hidden;
            display: none;
          }
          #progressBar {
            width: 0%;
            height: 100%;
            background: #28a745;
            transition: width 0.2s ease;
          }
          #status {
            margin-top: 1em;
            text-align: center;
            font-weight: bold;
          }
        </style>
      </head>
      <body>
        <h2>Firmware Update</h2>
        <form id="uploadForm">
          <input type="file" name="update" id="file" required>
          <input type="submit" value="Upload Firmware">
        </form>
        <div id="progressWrapper">
          <div id="progressBar"></div>
        </div>
        <div id="status">Waiting...</div>
  
        <script>
          const form = document.getElementById('uploadForm');
          const fileInput = document.getElementById('file');
          const progressWrapper = document.getElementById('progressWrapper');
          const progressBar = document.getElementById('progressBar');
          const status = document.getElementById('status');
  
          form.addEventListener('submit', e => {
            e.preventDefault();
            const file = fileInput.files[0];
            if (!file) return;
  
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/fw', true);
  
            xhr.upload.onprogress = function(e) {
              if (e.lengthComputable) {
                progressWrapper.style.display = 'block';
                const percent = (e.loaded / e.total) * 100;
                progressBar.style.width = percent + '%';
              }
            };
  
            xhr.onload = function() {
              if (xhr.status == 200) {
                status.innerText = xhr.responseText;
                progressBar.style.background = '#28a745'; // green
              } else {
                status.innerText = '❌ Upload failed';
                progressBar.style.background = '#dc3545'; // red
              }
            };
  
            const formData = new FormData();
            formData.append('update', file);
            xhr.send(formData);
            status.innerText = 'Uploading...';
          });
        </script>
      </body>
      </html>
    )rawliteral");
  });
  

  server.on("/fw", HTTP_POST, []() {
    server.send(200, "text/plain", Update.hasError() ? "❌ Update Failed" : "✅ Update Success! Rebooting...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("🔁 Updating: %s\n", upload.filename.c_str());
      if (!Update.begin()) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.println("✅ Update finished");
      } else {
        Update.printError(Serial);
      }
    }
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
    delay(300); // debounce
  }
}
