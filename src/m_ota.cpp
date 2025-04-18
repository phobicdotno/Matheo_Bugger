// src/m_ota.cpp

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESP.h>
#include "m_ota.h"
#include "m_web.h"      // brings in extern WebServer server
#include "m_wifi.h"     // tryConnect(), saveCredentials()
#include "m_display.h"  // currentText, displayOn, scrolledOnce, displayBlinkText()

void setupOTA() {
  // === Firmware upload & WiFi config page ===
  server.on("/fw", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>⚙️ Matheo Bugger FW & WiFi</title>
  <style>
    body { font-family: sans-serif; background: #f0f0f0; padding: 1em; }
    .wrapper { max-width: 500px; margin: auto; display: flex; flex-direction: column; gap: 1em; }
    input, select, button { width: 100%; padding: 12px; font-size: 1em; box-sizing: border-box; }
    #progressWrapper { width: 100%; height: 20px; background: #ddd; border-radius: 10px; overflow: hidden; }
    #progressBar { width: 0%; height: 100%; background: #28a745; transition: width 0.3s ease; }
    #wifiStatus { text-align: center; font-weight: bold; }
  </style>
</head>
<body>
  <div class="wrapper">
    <h2>⚙️ Firmware & WiFi Config</h2>
    <form id="uploadForm">
      <input type="file" id="file" required />
      <input type="submit" value="Upload Firmware" />
    </form>
    <div id="progressWrapper"><div id="progressBar"></div></div>
    <div id="status">Waiting...</div>
    <h3>WiFi Networks</h3>
    <button id="scanBtn">Scan Networks</button>
    <select id="ssidList"></select>
    <input type="password" id="wifiPass" placeholder="WiFi Password" />
    <button id="connectBtn">Connect WiFi</button>
    <div id="wifiStatus">Not connected</div>
  </div>
  <script>
    // Firmware upload
    document.getElementById('uploadForm').onsubmit = e => {
      e.preventDefault();
      const file = document.getElementById('file').files[0];
      if (!file) return;
      const xhr = new XMLHttpRequest();
      xhr.open("POST","/fw",true);
      xhr.upload.onprogress = ev => {
        document.getElementById('progressBar').style.width =
          Math.round(ev.loaded/ev.total*100) + '%';
      };
      xhr.onload = () => {
        document.getElementById('status').innerText = xhr.responseText;
        setTimeout(()=>location.reload(),5000);
      };
      const fd = new FormData();
      fd.append("update", file);
      xhr.send(fd);
      document.getElementById('status').innerText = "Uploading…";
    };

    // Scan networks → populate dropdown
    document.getElementById('scanBtn').onclick = () => {
      const st = document.getElementById('wifiStatus');
      st.innerText = "Scanning…";
      fetch('/scan')
        .then(r => r.json())
        .then(data => {
          const sel = document.getElementById('ssidList');
          sel.innerHTML = '';
          data.networks.forEach(net => {
            const name = net.split(' (')[0];
            const opt = document.createElement('option');
            opt.value = name;
            opt.text  = net;
            sel.add(opt);
          });
          st.innerText = "Scan complete!";
        });
    };

    // Connect to selected SSID
    document.getElementById('connectBtn').onclick = () => {
      const ssid = encodeURIComponent(document.getElementById('ssidList').value);
      const pass = encodeURIComponent(document.getElementById('wifiPass').value);
      const st = document.getElementById('wifiStatus');
      st.innerText = "Connecting…";
      fetch('/connect', {
        method: 'POST',
        headers: {'Content-Type':'application/x-www-form-urlencoded'},
        body: `ssid=${ssid}&pass=${pass}`
      })
      .then(r => r.text())
      .then(ip => {
        if (ip === 'FAIL') {
          st.innerText = "❌ Connection Failed";
        } else {
          window.location.href = 'http://' + ip + '/';
        }
      });
    };
  </script>
</body>
</html>
    )rawliteral";
    server.send(200, "text/html", html);
  });

  // POST handler: firmware upload
  server.on("/fw", HTTP_POST, []() {
    server.send(200, "text/plain; charset=utf-8",
      Update.hasError() ? "❌ Update Failed" : "✅ Update Success! Rebooting...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& up = server.upload();
    if (up.status == UPLOAD_FILE_START) Update.begin();
    if (up.status == UPLOAD_FILE_WRITE) Update.write(up.buf, up.currentSize);
    if (up.status == UPLOAD_FILE_END)   Update.end(true);
  });

  // JSON scan endpoint
  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
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

  // Connect endpoint: returns “FAIL” or the new IP
  server.on("/connect", HTTP_POST, []() {
    String ssid = server.arg("ssid"), pass = server.arg("pass");
    if (!tryConnect(ssid, pass, 15000)) {
      server.send(200, "text/plain", "FAIL");
      return;
    }
    saveCredentials(ssid, pass);
    WiFi.softAPdisconnect(true);
    IPAddress ip = WiFi.localIP();
    currentText  = ip.toString();
    displayOn    = true;
    scrolledOnce = false;
    display.displayClear();
    display.displayScroll(
      currentText.c_str(),
      PA_RIGHT, PA_SCROLL_RIGHT, 75
    );
    server.send(200, "text/plain", ip.toString());
  });

  server.begin();
}
