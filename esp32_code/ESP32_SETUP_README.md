# ESP32 Smart Security System - Arduino Setup Guide

## Overview

This guide explains how to upload the `smart_security.ino` Arduino sketch to your ESP32 microcontroller. The sketch implements a REST API server that controls door locks, monitors motion sensors, and captures camera snapshots.

## Table of Contents

1. [Hardware Requirements](#hardware-requirements)
2. [Software Installation](#software-installation)
3. [PIN Configuration](#pin-configuration)
4. [WiFi Setup](#wifi-setup)
5. [Uploading the Code](#uploading-the-code)
6. [Testing](#testing)
7. [Integration with Python MCP Server](#integration-with-python-mcp-server)
8. [Troubleshooting](#troubleshooting)
9. [Hardcoding Data for Demo](#hardcoding-data-for-demo)

---

## Hardware Requirements

### Essential Components

- **ESP32 Development Board**

  - ESP32 Dev Module (recommended for development)
  - ESP32-CAM (if you want camera support)
  - Alternative: Arduino with WiFi shield

- **GPIO Outputs:**

  - Solenoid Lock with Relay Module (GPIO 13)

    - 5V relay module to control solenoid
    - Can substitute with LED for demo/testing

  - Status LED (GPIO 2) - optional but useful
    - 3mm or 5mm LED with 220Ω resistor
    - Indicates motion/activity

- **GPIO Inputs:**

  - PIR Motion Sensor Module (GPIO 12)
    - Common: HC-SR501 or equivalent
    - 5V operation, outputs HIGH on motion
    - Adjustable detection range and timing

- **Power Supply:**

  - USB power for development
  - 5V 2A supply recommended if using relay + solenoid

- **Cables:**
  - Micro-USB cable for uploading code
  - Jumper wires (male-to-male, male-to-female)
  - Breadboard (recommended for prototyping)

### Optional Components

- WiFi antenna (if ESP32 board doesn't have one)
- Decoupling capacitors (100nF near power pins)
- Push button for testing (GPIO 0)

### Wiring Diagram

```
ESP32 Dev Board:
├── GPIO 13 → Relay Module IN → Solenoid (or LED for testing)
├── GPIO 12 ← PIR Sensor OUT
├── GPIO 2  → Status LED (optional)
├── GND     → Relay GND, PIR GND, LED GND
├── 5V      → Relay VCC, PIR VCC
└── 3.3V    → (available if needed)

PIR Sensor (HC-SR501):
├── VCC → ESP32 5V
├── GND → ESP32 GND
└── OUT → ESP32 GPIO 12

Relay Module:
├── VCC → ESP32 5V
├── GND → ESP32 GND
├── IN  → ESP32 GPIO 13
├── NO  → Solenoid +
└── COM → Solenoid -

Status LED:
├── Anode (+)   → ESP32 GPIO 2 via 220Ω resistor
└── Cathode (-) → ESP32 GND
```

---

## Software Installation

### Step 1: Install Arduino IDE

1. Download from: https://www.arduino.cc/en/software
2. Install for your operating system (Windows, Mac, Linux)
3. Open Arduino IDE

### Step 2: Add ESP32 Board Support

1. **Open Board Manager:**

   - Arduino IDE → Preferences (or Arduino → Settings on Mac)
   - Find "Additional Boards Manager URLs" field
   - Add this URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Click OK

2. **Install ESP32 Boards:**

   - Tools → Board → Boards Manager
   - Search for "ESP32"
   - Install "esp32 by Espressif Systems" (latest version recommended)
   - Wait for installation to complete (can take several minutes)

3. **Select Board:**
   - Tools → Board → ESP32 Dev Module (or your specific variant)

### Step 3: Install Required Libraries

In Arduino IDE, go to: **Sketch → Include Library → Manage Libraries**

Search for and install these libraries:

1. **ArduinoJson**

   - Search: "ArduinoJson"
   - Author: Benoit Blanchon
   - Version: 6.20.0+ (or latest)
   - Click Install

2. **AsyncTCP**

   - Search: "AsyncTCP"
   - Author: Me-No-Dev
   - Install

3. **ESPAsyncWebServer**
   - Search: "ESPAsyncWebServer"
   - Author: Me-No-Dev
   - Install

After installation, you should see green checkmarks next to each library.

---

## PIN Configuration

The sketch uses these GPIO pins. You can modify them in the `CONFIGURATION SECTION` at the top of the code:

```cpp
const int SOLENOID_LOCK_PIN = 13;      // Door lock control
const int PIR_SENSOR_PIN = 12;         // Motion sensor input
const int STATUS_LED_PIN = 2;          // Status indicator (optional)
```

### Using Different PINs

If you need to use different pins:

1. Check ESP32 pinout diagram for your board
2. Avoid pins: 0, 1, 6-11 (reserved for SPI flash)
3. Safe pins: 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 37, 38, 39
4. Update the constants in the code
5. Update the `Configuration` section in this README

---

## WiFi Setup

### Configuration

In the Arduino sketch, find and update these lines:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";           // Change this
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";   // Change this
```

### Steps

1. **Find Your WiFi Name:**

   - Look for available WiFi networks on your computer
   - Note the exact SSID (case-sensitive)

2. **Update SSID and Password:**

   ```cpp
   const char* WIFI_SSID = "MyHomeWiFi";
   const char* WIFI_PASSWORD = "MyPassword123";
   ```

3. **WiFi Troubleshooting:**
   - Double-check SSID spelling (case-sensitive)
   - Verify password is correct
   - Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz on most boards)
   - Check router is working

---

## Uploading the Code

### Step 1: Connect ESP32

1. Connect ESP32 to computer using micro-USB cable
2. In Arduino IDE: **Tools → Port**
3. Select the COM port that appeared (usually `/dev/ttyUSB0` on Linux/Mac or `COM3` on Windows)
4. Verify: **Tools → Board** shows "ESP32 Dev Module"

### Step 2: Verify Code Compiles

1. Click **Sketch → Verify/Compile** (checkmark icon)
2. Wait for compilation to complete
3. You should see: `Compiling... Done ✓`

### Step 3: Upload Code

1. Click **Sketch → Upload** (arrow icon) or press `Ctrl+U`
2. Watch the progress bar at the bottom
3. You should see: `Uploading... Done ✓`

### Step 4: Open Serial Monitor

1. **Tools → Serial Monitor** (or `Ctrl+Shift+M`)
2. Set baud rate to **115200** (bottom right dropdown)
3. You should see boot messages:

```
================================================
Smart Security System - ESP32
================================================
[Boot] System starting...
[Boot] GPIO pins initialized
[WiFi] Connecting to WiFi...
[WiFi] Connected!
[WiFi] IP Address: 192.168.1.100
[Boot] Setting up API endpoints...
[Boot] Web server started on port 80
================================================
```

4. **Note the IP Address** - you'll need this later!

### If Upload Fails

**Error: "Failed to connect to ESP32"**

- Check USB cable connection
- Try a different USB cable
- Restart Arduino IDE
- Check Device Manager for COM port

**Error: "Timed out waiting for packet"**

- Hold BOOT button while uploading
- Check USB driver installation

---

## Testing

### Test 1: Health Check

After the ESP32 starts, test if it's responding:

```bash
# From command line (macOS/Linux):
curl http://192.168.1.100/api/health

# Or open in browser:
http://192.168.1.100/api/health
```

You should get:

```json
{
  "status": "ok",
  "device": "ESP32 Security System",
  "timestamp": 1234567890,
  "door_locked": true,
  "motion_detected": false
}
```

### Test 2: Door Lock

```bash
# Lock the door:
curl -X POST http://192.168.1.100/api/lock

# Response should be:
{"status": "locked", "device": "main_door", "timestamp": 1234567890}
```

### Test 3: Door Status

```bash
# Check door status:
curl http://192.168.1.100/api/door/status

# Response:
{"locked": true, "device": "main_door", "timestamp": 1234567890}
```

### Test 4: Motion Sensor

```bash
# Check motion status:
curl http://192.168.1.100/api/motion

# Response:
{"motion_detected": false, "last_motion_time": 0, "seconds_ago": 0, "timestamp": 1234567890}
```

### Test 5: Trigger Motion (Testing)

```bash
# Simulate motion for testing:
curl -X POST http://192.168.1.100/api/trigger_motion

# This helps test without actual motion sensor
```

---

## Integration with Python MCP Server

Once you've confirmed the ESP32 is working:

### Step 1: Note the IP Address

From Serial Monitor output, get your ESP32 IP address:

```
[WiFi] IP Address: 192.168.1.100
```

### Step 2: Update Python Configuration

In the Python MCP server, edit `.env`:

```bash
# Was:
MOCK_MODE=true
ESP32_IP=http://192.168.1.100

# Change to:
MOCK_MODE=false
ESP32_IP=http://192.168.1.100
```

Replace `192.168.1.100` with your actual ESP32 IP address.

### Step 3: Restart MCP Server

```bash
# Stop current server (Ctrl+C)
# Start again:
cd /path/to/ldam-cp
python src/server.py
```

### Step 4: Test with Claude

In Claude Desktop, test a query:

```
"Is the door locked?"
```

It should now:

1. Call the MCP server
2. MCP server calls `/api/door/status` on ESP32
3. Claude responds based on actual ESP32 status

---

## Hardcoding Data for Demo

The sketch stores device state in variables at the top of `loop()`. You can hardcode demo data:

### Option 1: Initial State in Setup

Change initial values in `CONFIGURATION SECTION`:

```cpp
// Device State (Hardcoded data for demo)
bool door_locked = true;                          // Initial: locked
unsigned long last_motion_timestamp = 0;          // Initial: no motion
bool motion_currently_detected = false;            // Initial: no motion now
```

### Option 2: Simulate Demo Sequence

Add a demo timer in `loop()`:

```cpp
// In loop() function:
static unsigned long demo_timer = 0;

// Every 30 seconds, simulate events
if (millis() - demo_timer > 30000) {
  demo_timer = millis();

  // Toggle lock
  door_locked = !door_locked;
  digitalWrite(SOLENOID_LOCK_PIN, door_locked ? HIGH : LOW);

  Serial.println("[Demo] Door toggled");
}
```

### Option 3: Web Control for Setup

Use the test endpoints to set state before demo:

```bash
# Pre-demo setup:
curl -X POST http://192.168.1.100/api/lock
curl -X POST http://192.168.1.100/api/trigger_motion

# Then run demo queries
```

---

## Troubleshooting

### ESP32 Not Connecting to WiFi

**Symptoms:** "WiFi Connection timeout!" in Serial Monitor

**Solutions:**

1. Check SSID/password are correct (case-sensitive)
2. Verify router is on 2.4GHz (not 5GHz)
3. Check router accepts WPA2 (not WEP)
4. Try moving closer to router

**Add debugging:**

```cpp
Serial.print("[WiFi] Connecting to: ");
Serial.println(WIFI_SSID);
Serial.print("[WiFi] Using password: ");
Serial.println(WIFI_PASSWORD);
```

### API Endpoints Not Responding

**Symptoms:** `curl` returns "Connection refused"

**Solutions:**

1. Verify ESP32 IP address is correct
2. Ensure ESP32 and computer are on same WiFi network
3. Check firewall isn't blocking port 80
4. Restart ESP32 (press RESET button)

### Solenoid Not Activating

**Symptoms:** POST /api/lock returns success but lock doesn't move

**Solutions:**

1. Check GPIO 13 has 5V output (use multimeter)
2. Verify relay module is powered (check LED on relay)
3. Test relay with jumper wire (bridge IN to VCC directly)
4. Check solenoid power supply is connected
5. Try using LED on GPIO 13 for testing instead

### Motion Sensor Never Triggers

**Symptoms:** `motion_detected` always false

**Solutions:**

1. Check GPIO 12 is wired to PIR OUT
2. Verify PIR sensor has power (check LED on PIR)
3. Give PIR 30-60 seconds to calibrate after power-on
4. Adjust PIR potentiometers:
   - Sensitivity dial (usually on back)
   - Time delay dial
5. Test with curl: `curl -X POST http://192.168.1.100/api/trigger_motion`

### Compile Errors

**Error: `ESPAsyncWebServer not found`**

- Reinstall "ESPAsyncWebServer" library via Library Manager

**Error: `ArduinoJson not found`**

- Reinstall "ArduinoJson" library via Library Manager

---

## Useful References

- [ESP32 Pinout](https://randomncreativity.com/esp32-pinout/)
- [ESPAsyncWebServer Documentation](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [HC-SR501 PIR Sensor Setup](https://lastminuteengineers.com/pir-sensor-arduino-tutorial/)

---

## Next Steps

1. ✅ Upload code to ESP32
2. ✅ Confirm WiFi connection
3. ✅ Test `/api/health` endpoint
4. ✅ Test each endpoint individually
5. ✅ Update Python MCP `.env` with ESP32 IP
6. ✅ Change `MOCK_MODE=false`
7. ✅ Test Claude queries with real hardware
8. 🎉 Ready for demo!

---

## Support

If issues persist:

1. Check Serial Monitor output (Tools → Serial Monitor)
2. Enable debug logging in code: `LOG_LEVEL=DEBUG`
3. Test each endpoint manually with `curl`
4. Verify WiFi connectivity: `ping <esp32_ip>`
5. Check hardware connections with multimeter
