#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "m_ota.h"
#include "m_web.h"

// Setup OTA routes for firmware upload
void setupOTA() {
  server.on("/fw", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="utf-8">
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
</style>
</head>
<body>
<h2>Firmware Update</h2>
<form id="uploadForm">
  <input type="file" id="file" required>
  <input type="submit" value="Upload Firmware">
</form>
<div id="progressWrapper" style="display:none;"><div id="progressBar"></div></div>
<div id="status" style="margin-top: 1em; font-weight: bold;">Waiting...</div>
<div id="reconnect" style="display:none;">
  <div id="spinner"></div>
  <div style="margin-top: 1em;">Reconnecting...</div>
</div>
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
</script>
</body>
</html>
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
}
