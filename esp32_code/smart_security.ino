/*
  ============================================================================
  Smart Security System - ESP32 REST API Server
  ============================================================================
  
  This Arduino sketch runs on an ESP32/ESP32-CAM microcontroller and provides
  REST API endpoints for a smart home security system. The endpoints are called
  by a Python MCP server which in turn is called by Claude AI.
  
  Features:
  - Door lock control via solenoid (GPIO output)
  - Motion detection via PIR sensor (GPIO input)
  - ESP32-CAM snapshot capture (for ESP32-CAM variant)
  - REST API endpoints for all operations
  - JSON responses
  - Error handling and logging
  
  Hardware Requirements:
  - ESP32 or ESP32-CAM development board
  - Solenoid lock with relay module (optional for demo, can mock with LED)
  - PIR motion sensor module
  - WiFi connectivity
  
  ============================================================================
  SETUP INSTRUCTIONS
  ============================================================================
  
  1. Install Arduino IDE and ESP32 board support:
     https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md
  
  2. Install required libraries:
     - ArduinoJson (for JSON handling)
     - AsyncTCP
     - ESPAsyncWebServer
     
     Go to Sketch -> Include Library -> Manage Libraries and search for:
     - "ArduinoJson" by Benoit Blanchon
     - "AsyncTCP" by Me-No-Dev
     - "ESPAsyncWebServer" by Me-No-Dev
  
  3. Configure PIN assignments (see CONFIGURATION section below)
  
  4. Update WiFi credentials:
     - Change WIFI_SSID and WIFI_PASSWORD
  
  5. Upload to ESP32 using Arduino IDE
     - Select "Tools" -> "Board" -> "ESP32 Dev Module" (or "ESP32-CAM" if using camera)
     - Select correct COM port
     - Upload
  
  6. Open Serial Monitor (115200 baud) to see ESP32 IP address
     - Note: IP will appear after WiFi connection is established
  
  7. Configure Python MCP server:
     - Update .env: ESP32_IP=http://<your_esp32_ip>
     - Change MOCK_MODE=false
     - Restart MCP server
  
  ============================================================================
  API ENDPOINTS (JSON format)
  ============================================================================
  
  POST /api/lock
    Body: {"device_id": "main_door"}
    Response: {"status": "locked", "device": "main_door", "timestamp": 1234567890}
  
  POST /api/unlock
    Body: {"device_id": "main_door"}
    Response: {"status": "unlocked", "device": "main_door", "timestamp": 1234567890}
  
  GET /api/door/status?device_id=main_door
    Response: {"locked": true, "device": "main_door", "timestamp": 1234567890}
  
  GET /api/motion
    Response: {
      "motion_detected": false,
      "last_motion_time": 1234567890,
      "seconds_ago": 120,
      "timestamp": 1234567890
    }
  
  GET /api/snapshot
    Response: {
      "status": "ok",
      "url": "http://<esp32_ip>:80/fs/jpg/last.jpg",
      "timestamp": 1234567890
    }
  
  ============================================================================
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <time.h>

// ============================================================================
// CONFIGURATION SECTION - UPDATE THESE VALUES
// ============================================================================

// WiFi Configuration
const char* WIFI_SSID = "VivoY73";           // Change this
const char* WIFI_PASSWORD = "12345678";   // Change this
const unsigned long WIFI_TIMEOUT = 15000;           // 15 seconds timeout

// PIN Configuration
// These should match your hardware setup
const int SOLENOID_LOCK_PIN = 15;      // GPIO pin for lock (HIGH = locked, LOW = unlocked)
const int PIR_SENSOR_PIN = 12;         // GPIO pin for PIR motion sensor
const int STATUS_LED_PIN = 2;          // GPIO pin for status indicator (optional)

// Device State (Hardcoded data for demo - update as needed)
// In production, you might store this in EEPROM or database
bool door_locked = true;                          // Initial lock state
unsigned long last_motion_timestamp = 0;          // Timestamp of last motion
bool motion_currently_detected = false;            // Current motion state

// Web Server Configuration
AsyncWebServer server(80);
const int SERVER_PORT = 80;

// ===========================================a================================
// UTILITY FUNCTIONS
// ============================================================================

// Get current Unix timestamp
unsigned long getTimestamp() {
  time_t now = time(nullptr);
  return (unsigned long)now;
}

// Format JSON error response
String formatErrorResponse(const char* message) {
  StaticJsonDocument<256> doc;
  doc["status"] = "error";
  doc["message"] = message;
  doc["timestamp"] = getTimestamp();
  
  String response;
  serializeJson(doc, response);
  return response;
}

// Format JSON success response
String formatResponse(const char* key, const char* value) {
  StaticJsonDocument<256> doc;
  doc[key] = value;
  doc["timestamp"] = getTimestamp();
  
  String response;
  serializeJson(doc, response);
  return response;
}

// Initialize GPIO pins
void initializePins() {
  pinMode(SOLENOID_LOCK_PIN, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // Set initial states
  digitalWrite(SOLENOID_LOCK_PIN, door_locked ? HIGH : LOW);
  digitalWrite(STATUS_LED_PIN, LOW);
}

// Connect to WiFi
bool connectToWiFi() {
  Serial.println("[WiFi] Connecting to WiFi...");
  Serial.print("[WiFi] SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    if (millis() - startTime > WIFI_TIMEOUT) {
      Serial.println();
      Serial.println("[WiFi] Connection timeout!");
      return false;
    }
  }
  
  Serial.println();
  Serial.println("[WiFi] Connected!");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WiFi] Signal Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  return true;
}

// Update motion sensor state (call this regularly)
void updateMotionSensor() {
  bool current_motion = digitalRead(PIR_SENSOR_PIN);
  
  if (current_motion && !motion_currently_detected) {
    // Motion just detected
    motion_currently_detected = true;
    last_motion_timestamp = getTimestamp();
    digitalWrite(STATUS_LED_PIN, HIGH);  // Turn on LED
    Serial.println("[Motion] Motion detected!");
  } else if (!current_motion && motion_currently_detected) {
    // Motion just stopped
    motion_currently_detected = false;
    digitalWrite(STATUS_LED_PIN, LOW);   // Turn off LED
    Serial.println("[Motion] Motion stopped");
  }
}

// ============================================================================
// API ENDPOINT HANDLERS
// ============================================================================

/*
  POST /api/lock
  Lock the door via solenoid
*/
void handleLock(AsyncWebServerRequest* request) {
  Serial.println("[API] POST /api/lock called");
  
  door_locked = true;
  digitalWrite(SOLENOID_LOCK_PIN, HIGH);  // Activate lock
  
  delay(100);  // Brief delay for relay to settle
  
  StaticJsonDocument<256> response;
  response["status"] = "locked";
  response["device"] = "main_door";
  response["timestamp"] = getTimestamp();
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
  Serial.println("[API] Lock successful");
}

/*
  POST /api/unlock
  Unlock the door via solenoid
*/
void handleUnlock(AsyncWebServerRequest* request) {
  Serial.println("[API] POST /api/unlock called");
  
  door_locked = false;
  digitalWrite(SOLENOID_LOCK_PIN, LOW);   // Deactivate lock
  
  delay(100);  // Brief delay for relay to settle
  
  StaticJsonDocument<256> response;
  response["status"] = "unlocked";
  response["device"] = "main_door";
  response["timestamp"] = getTimestamp();
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
  Serial.println("[API] Unlock successful");
}

/*
  GET /api/door/status?device_id=main_door
  Get current door lock status
*/
void handleDoorStatus(AsyncWebServerRequest* request) {
  Serial.println("[API] GET /api/door/status called");
  
  String device_id = "main_door";
  if (request->hasParam("device_id")) {
    device_id = request->getParam("device_id")->value();
  }
  
  StaticJsonDocument<256> response;
  response["locked"] = door_locked;
  response["device"] = device_id;
  response["timestamp"] = getTimestamp();
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
  Serial.print("[API] Door status: ");
  Serial.println(door_locked ? "locked" : "unlocked");
}

/*
  GET /api/motion
  Get motion sensor status
*/
void handleMotionStatus(AsyncWebServerRequest* request) {
  Serial.println("[API] GET /api/motion called");
  
  updateMotionSensor();  // Update sensor reading
  
  unsigned long current_time = getTimestamp();
  unsigned long seconds_ago = (last_motion_timestamp > 0) 
    ? (current_time - last_motion_timestamp) 
    : 0;
  
  StaticJsonDocument<256> response;
  response["motion_detected"] = motion_currently_detected;
  response["last_motion_time"] = last_motion_timestamp;
  response["seconds_ago"] = seconds_ago;
  response["timestamp"] = current_time;
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
  Serial.print("[API] Motion status: ");
  Serial.println(motion_currently_detected ? "detected" : "clear");
}

/*
  GET /api/snapshot
  Get camera snapshot URL (for ESP32-CAM)
  NOTE: This returns a mock URL. For actual camera support, 
  you'll need ESP32-CAM and additional camera setup.
*/
void handleSnapshot(AsyncWebServerRequest* request) {
  Serial.println("[API] GET /api/snapshot called");
  
  // Get local IP for snapshot URL
  String ip = WiFi.localIP().toString();
  String snapshot_url = "http://" + ip + ":80/fs/jpg/last.jpg";
  
  StaticJsonDocument<256> response;
  response["status"] = "ok";
  response["url"] = snapshot_url;
  response["timestamp"] = getTimestamp();
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
  Serial.println("[API] Snapshot URL provided");
}

/*
  POST /api/trigger_motion (TESTING ONLY)
  Manually trigger motion for testing
*/
void handleTriggerMotion(AsyncWebServerRequest* request) {
  Serial.println("[API] POST /api/trigger_motion called (TEST)");
  
  motion_currently_detected = true;
  last_motion_timestamp = getTimestamp();
  digitalWrite(STATUS_LED_PIN, HIGH);
  
  StaticJsonDocument<256> response;
  response["status"] = "motion_triggered";
  response["message"] = "Motion simulation triggered for testing";
  response["timestamp"] = getTimestamp();
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
}

/*
  GET /api/health
  Health check endpoint
*/
void handleHealth(AsyncWebServerRequest* request) {
  StaticJsonDocument<256> response;
  response["status"] = "ok";
  response["device"] = "ESP32 Security System";
  response["timestamp"] = getTimestamp();
  response["door_locked"] = door_locked;
  response["motion_detected"] = motion_currently_detected;
  
  String jsonResponse;
  serializeJson(response, jsonResponse);
  
  request->send(200, "application/json", jsonResponse);
}

// ============================================================================
// SETUP - Run once on startup
// ============================================================================

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);  // Wait for serial to initialize
  
  Serial.println("\n\n================================================");
  Serial.println("Smart Security System - ESP32");
  Serial.println("================================================");
  Serial.println("[Boot] System starting...");
  
  // Initialize GPIO pins
  initializePins();
  Serial.println("[Boot] GPIO pins initialized");
  
  // Connect to WiFi
  if (!connectToWiFi()) {
    Serial.println("[Boot] WiFi connection failed! Check credentials.");
    // Continue anyway for now, or implement retry logic
  }
  
  // Configure time (for timestamps)
  // Optional: sync with NTP server
  // configTime(0, 0, "pool.ntp.org");
  
  // Setup API endpoints
  Serial.println("[Boot] Setting up API endpoints...");
  
  // DOOR CONTROL ENDPOINTS
  server.on("/api/lock", HTTP_POST, [](AsyncWebServerRequest* request) {
    handleLock(request);
  });
  
  server.on("/api/unlock", HTTP_POST, [](AsyncWebServerRequest* request) {
    handleUnlock(request);
  });
  
  server.on("/api/door/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    handleDoorStatus(request);
  });
  
  // MOTION SENSOR ENDPOINTS
  server.on("/api/motion", HTTP_GET, [](AsyncWebServerRequest* request) {
    handleMotionStatus(request);
  });
  
  // CAMERA ENDPOINTS
  server.on("/api/snapshot", HTTP_GET, [](AsyncWebServerRequest* request) {
    handleSnapshot(request);
  });
  
  // TEST/UTILITY ENDPOINTS
  server.on("/api/trigger_motion", HTTP_POST, [](AsyncWebServerRequest* request) {
    handleTriggerMotion(request);
  });
  
  server.on("/api/health", HTTP_GET, [](AsyncWebServerRequest* request) {
    handleHealth(request);
  });
  
  // 404 handler
  server.onNotFound([](AsyncWebServerRequest* request) {
    StaticJsonDocument<256> response;
    response["status"] = "error";
    response["message"] = "Endpoint not found";
    response["requested_path"] = request->url();
    response["timestamp"] = getTimestamp();
    
    String jsonResponse;
    serializeJson(response, jsonResponse);
    
    request->send(404, "application/json", jsonResponse);
  });
  
  // Start the server
  server.begin();
  Serial.println("[Boot] Web server started on port 80");
  Serial.print("[Boot] Access API at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/api/health");
  Serial.println("================================================\n");
}

// ============================================================================
// LOOP - Run repeatedly
// ============================================================================

void loop() {
  // Update motion sensor regularly
  updateMotionSensor();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Warning] WiFi disconnected!");
    digitalWrite(STATUS_LED_PIN, LOW);
  }
  
  // Add small delay to prevent watchdog timeout
  delay(100);
}
