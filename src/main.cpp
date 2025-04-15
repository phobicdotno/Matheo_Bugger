#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// === MATRIX SETUP ===
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// === NETWORK CONFIG ===
const char* ssid = "teliaphobic";
const char* password = "bdxCugDF";

IPAddress local_IP(192, 168, 1, 229);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

String currentText = "Hello!";
bool displayOn = true;

// === HTML UI ===
const char HTML_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>Matheo Bugger</title></head>
<body>
  <h2>Matheo Bugger</h2>
  <form action="/set" method="get">
    Text: <input type="text" name="text" />
    <input type="submit" value="Update" />
  </form>
  <br>
  <form action="/toggle" method="get">
    <input type="submit" value="Toggle Display" />
  </form>
</body>
</html>
)rawliteral";

// === HANDLERS ===
void handleRoot() {
  server.send(200, "text/html", HTML_page);
}

void handleSet() {
  if (server.hasArg("text")) {
    currentText = server.arg("text");
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

  // === MATRIX INIT ===
  display.begin();
  display.setIntensity(5);
  display.displayClear();
  display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);

  // === WIFI INIT ===
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // === WEB SERVER INIT ===
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();

  if (displayOn) {
    if (display.displayAnimate()) {
      display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 75);
    }
  }
}
