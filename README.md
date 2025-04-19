# ❤️ Matheo Bugger ESP32 Display System

Built for precision, reliability, and style — powered by ESP32-WROOM-32D

---

## 🔧 Core Features

### 🧠 Smart WiFi Management
- 📡 Scan and list available WiFi networks
- 🔐 Connect with custom SSID + password (DHCP)
- 💾 Auto-save credentials (Preferences/NVS)
- 🔁 Auto-reconnect on reboot
- 📶 Manual long-press shows current IP on display

### 🔧 Firmware Management
- 💻 Upload new firmware via web interface (OTA)
- 💾 Shows available flash space for update
- 🔃 Soft reboots automatically after upload

---

## 💻 Web UI System (LittleFS-powered)

- 🎨 Responsive HTML/CSS/JS served from flash
- 🧭 Routes:
  - `/` — message input + display toggle
  - `/fw` — firmware update + WiFi config
  - `/status` — full system diagnostics

- 🌐 Bottom tab bar navigation (emoji, flat, clean)

---

## 🔢 LED Matrix Display (MAX7219)

- 🖥️ Controlled via MD_Parola + MD_MAX72XX
- 🔁 Scrolls incoming messages
- ✅ Confirms messages via physical button
- 🔕 Auto-clears after confirmation
- 💡 Adjustable brightness animation effect
- ↕️ Flipped and mirrored zones for clean mount

---

## 📊 System Monitoring (`/status`)

Returns real-time JSON with:
- IP address, Gateway
- SSID + MAC address
- Signal strength (RSSI)
- LittleFS usage: total/free
- OTA firmware flash space
- Connection counter since boot

---

## 🔁 Upload Workflow

- ✅ One-click Upload (firmware + filesystem)
- 🧠 Smart `combined` PlatformIO env
- 🧽 No more BOOT button needed on supported dev boards
- 🧑‍💻 Dev-friendly config, debug logs, and live status polling

---

## 🧪 System Requirements

- ESP32-WROOM-32D module or compatible dev board  
- PlatformIO + VS Code  
- CH340 / CP210x or similar USB bridge  
- Connected 8×32 LED matrix (MAX7219)  
- Optional button for physical message confirmation
