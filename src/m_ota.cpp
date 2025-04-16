#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESP.h> // For heap and flash info
#include "m_ota.h"
#include "m_web.h"

void setupOTA() {
  server.on("/fw", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Firmware Update & System Stats</title>
  <style>
    body { font-family: sans-serif; padding: 1em; background: #f0f0f0; text-align: center; }
    .status { margin: 1em auto; max-width: 400px; }
    input[type="submit"] {
      padding: 12px 24px; background: #007bff; color: white; border: none; border-radius: 5px;
      cursor: pointer; font-size: 1em;
    }
    input[type="submit"]:hover { background-color: #0056b3; }
    #progressWrapper { margin: 2em auto; width: 300px; height: 20px; background: #ddd; border-radius: 10px; overflow: hidden; }
    #progressBar { width: 0%; height: 100%; background: #28a745; transition: width 0.3s ease; }
  </style>
</head>
<body>
  <h2>Firmware Update & System Stats</h2>
  
  <form id="uploadForm">
    <input type="file" id="file" required>
    <input type="submit" value="Upload Firmware">
  </form>
  
  <div id="progressWrapper"><div id="progressBar"></div></div>
  <div id="status">Waiting...</div>

  <div class="status">
    <h3>System Info</h3>
    <p>CPU Frequency: <span id="cpuFreq">...</span> MHz</p>
    <p>Free RAM: <span id="freeRAM">...</span> bytes</p>
    <p>Flash Usage: <span id="flashUsed">...</span> / <span id="flashTotal">...</span> bytes</p>
    <p>WiFi Strength (RSSI): <span id="wifiRSSI">...</span> dBm</p>
  </div>

  <script>
    const form = document.getElementById('uploadForm');
    form.addEventListener('submit', e => {
      e.preventDefault();
      const file = document.getElementById('file').files[0];
      if (!file) return;

      const xhr = new XMLHttpRequest();
      xhr.open("POST", "/fw", true);
      xhr.upload.onprogress = e => {
        const percent = (e.loaded / e.total) * 100;
        document.getElementById('progressBar').style.width = percent + '%';
      };
      xhr.onload = () => {
        document.getElementById('status').innerText = xhr.responseText;
        setTimeout(() => { location.reload(); }, 5000);
      };
      const formData = new FormData();
      formData.append("update", file);
      xhr.send(formData);
      document.getElementById('status').innerText = "Uploading...";
    });

    function updateStats() {
      fetch('/stats').then(r => r.json()).then(data => {
        document.getElementById('cpuFreq').innerText = data.cpuFreq;
        document.getElementById('freeRAM').innerText = data.freeRAM;
        document.getElementById('flashUsed').innerText = data.flashUsed;
        document.getElementById('flashTotal').innerText = data.flashTotal;
        document.getElementById('wifiRSSI').innerText = data.wifiRSSI;
      });
    }
    setInterval(updateStats, 3000);
    updateStats();
  </script>
</body>
</html>
    )rawliteral";

    server.send(200, "text/html", html);
  });

  server.on("/fw", HTTP_POST, []() {
    server.send(200, "text/plain; charset=utf-8", Update.hasError() ? "❌ Update Failed" : "✅ Update Success! Rebooting...");
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

  server.on("/stats", []() {
    uint32_t flashSize = ESP.getFlashChipSize();
    uint32_t flashUsed = ESP.getSketchSize();

    String json = "{";
    json += "\"cpuFreq\":" + String(getCpuFrequencyMhz()) + ",";
    json += "\"freeRAM\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"flashUsed\":" + String(flashUsed) + ",";
    json += "\"flashTotal\":" + String(flashSize) + ",";
    json += "\"wifiRSSI\":" + String(WiFi.RSSI());
    json += "}";

    server.send(200, "application/json", json);
  });
}
