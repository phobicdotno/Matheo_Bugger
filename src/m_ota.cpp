#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESP.h>
#include "m_ota.h"
#include "m_web.h"
#include "m_wifi.h"
#include "m_display.h"

void setupOTA() {
  server.on("/fw", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Firmware Update & WiFi Config</title>
  <style>
    body { font-family: sans-serif; padding: 1em; background: #f0f0f0; text-align: center; }
    .status, .wifi { margin: 1em auto; max-width: 400px; }
    input[type="submit"], button {
      padding: 12px 24px; background: #007bff; color: white; border: none; border-radius: 5px;
      cursor: pointer; font-size: 1em; margin-top: 10px;
    }
    button:hover, input[type="submit"]:hover { background-color: #0056b3; }
    select, input[type="password"] {
      padding: 8px; font-size: 1em; width: 90%; margin-top: 10px;
    }
    #progressWrapper { margin: 1em auto; width: 300px; height: 20px; background: #ddd; border-radius: 10px; overflow: hidden; }
    #progressBar { width: 0%; height: 100%; background: #28a745; transition: width 0.3s ease; }
  </style>
</head>
<body>
  <h2>Firmware Update & WiFi Config</h2>
  
  <form id="uploadForm">
    <input type="file" id="file" required>
    <input type="submit" value="Upload Firmware">
  </form>
  
  <div id="progressWrapper"><div id="progressBar"></div></div>
  <div id="status">Waiting...</div>

  <div class="wifi">
    <h3>WiFi Networks</h3>
    <button onclick="scanNetworks()">Scan Networks</button><br>
    <select id="ssidList"></select><br>
    <input type="password" id="wifiPass" placeholder="WiFi Password"><br>
    <button onclick="connectWifi()">Connect WiFi (DHCP)</button><br>
    <div id="wifiStatus">Not connected</div>
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

    function scanNetworks() {
      document.getElementById('wifiStatus').innerText = "Scanning...";
      fetch('/scan').then(r => r.json()).then(data => {
        const ssidList = document.getElementById('ssidList');
        ssidList.innerHTML = '';
        data.networks.forEach(net => {
          let option = document.createElement('option');
          option.value = net;
          option.text = net;
          ssidList.add(option);
        });
        document.getElementById('wifiStatus').innerText = "Scan complete!";
      });
    }

    function connectWifi() {
      const ssid = document.getElementById('ssidList').value;
      const pass = document.getElementById('wifiPass').value;
      document.getElementById('wifiStatus').innerText = "Connecting...";
      
      fetch('/connect', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
      }).then(r => r.text()).then(status => {
        document.getElementById('wifiStatus').innerText = status;
      });
    }
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

  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    String json = "{\"networks\":[";
    for (int i = 0; i < n; ++i) {
      int ch = WiFi.channel(i);
      String band = (ch <= 14) ? "2.4GHz" : "5GHz";
      if (i) json += ",";
      json += "\"" + WiFi.SSID(i) + " (" +
              String(WiFi.RSSI(i)) + " dBm, Ch: " +
              String(ch) + ", " + band + ")\"";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/connect", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    bool ok = tryConnect(ssid, pass, 15000);   // one 15‑s attempt

    if (ok) {
      saveCredentials(ssid, pass);

      IPAddress ip = WiFi.localIP();
      currentText = "IP: " + ip.toString();
      displayOn = true;
      display.displayClear();
      display.displayScroll(currentText.c_str(), PA_RIGHT, PA_SCROLL_RIGHT, 75);

      server.send(200, "text/plain",
                  "✅ Connected! IP: " + ip.toString());
    } else {
      server.send(200, "text/plain", "❌ Connection Failed");
    }
  });     // end of /connect route
}