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
     - esp32-camera (for ESP32-CAM support)
     
     Go to Sketch -> Include Library -> Manage Libraries and search for:
     - "ArduinoJson" by Benoit Blanchon
     - "AsyncTCP" by Me-No-Dev
     - "ESPAsyncWebServer" by Me-No-Dev
     - "esp32-camera" by espressif
  
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
    Response: Binary JPEG image data (image/jpeg)
    Returns raw JPEG image from ESP32-CAM camera
    Timestamp available in response headers or can be logged separately
  
  ============================================================================
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <time.h>
#include "esp_camera.h"
#include "SPIFFS.h"

// ============================================================================
// CONFIGURATION SECTION - UPDATE THESE VALUES
// ============================================================================

// WiFi Configuration
const char* WIFI_SSID = "VivoY73";           // Change this
const char* WIFI_PASSWORD = "12345678";   // Change this
const unsigned long WIFI_TIMEOUT = 15000;           // 15 seconds timeout

// PIN Configuration
// These should match your hardware setup
const int SOLENOID_LOCK_PIN = 13;      // GPIO pin for lock (HIGH = locked, LOW = unlocked)
const int PIR_SENSOR_PIN = 12;         // GPIO pin for PIR motion sensor
const int STATUS_LED_PIN = 33;          // GPIO pin for status indicator (optional)

// ESP32-CAM Pin Configuration (OV2640)
// DO NOT CHANGE - These are hardwired for ESP32-CAM module
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

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

// Initialize ESP32-CAM
bool initCamera() {
  Serial.println("[Camera] Initializing ESP32-CAM...");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Image quality settings
  config.jpeg_quality = 10;  // 0-63, lower number = higher quality
  config.fb_count = 2;       // Frame buffer count
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  // Frame size
  config.frame_size = FRAMESIZE_VGA;  // 640x480
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[Camera] Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Camera settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // brightness (-2 to 2)
  s->set_contrast(s, 0);       // contrast (-2 to 2)
  s->set_saturation(s, 0);     // saturation (-2 to 2)
  s->set_special_effect(s, 0); // special effect (0 = none)
  s->set_awb_gain(s, 1);       // auto white balance (0 or 1)
  s->set_wb_mode(s, 0);        // white balance (0-4)
  s->set_exposure_ctrl(s, 1);  // exposure control (0 or 1)
  s->set_aec_value(s, 300);    // auto exposure value (0-1200)
  s->set_gain_ctrl(s, 1);      // gain control (0 or 1)
  s->set_agc_gain(s, 0);       // auto gain (0-30)
  s->set_gainceiling(s, (gainceiling_t)0);  // gain ceiling (0-6)
  s->set_bpc(s, 0);            // black pixel correction (0 or 1)
  s->set_wpc(s, 1);            // white pixel correction (0 or 1)
  s->set_raw_gma(s, 1);        // raw gamma (0 or 1)
  s->set_lenc(s, 1);           // lens correction (0 or 1)
  
  Serial.println("[Camera] Camera initialized successfully!");
  return true;
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
  
  // Capture frame from camera
  camera_fb_t * fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("[API] Camera capture failed");
    
    StaticJsonDocument<256> response;
    response["status"] = "error";
    response["message"] = "Failed to capture frame from camera";
    response["timestamp"] = getTimestamp();
    
    String jsonResponse;
    serializeJson(response, jsonResponse);
    
    request->send(500, "application/json", jsonResponse);
    return;
  }
  
  // Create data URL or send image directly
  // Option 1: Send image as base64 in JSON
  // Option 2: Send raw JPEG
  // Using Option 2 - send raw JPEG data
  
  Serial.printf("[API] Captured frame: %d bytes\n", fb->len);
  
  request->send_P(200, "image/jpeg", (const uint8_t *)fb->buf, fb->len);
  
  // Return frame buffer
  esp_camera_fb_return(fb);
  
  Serial.println("[API] Snapshot sent successfully");
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
  
  // Initialize camera
  if (!initCamera()) {
    Serial.println("[Boot] Camera initialization failed!");
    // Continue with other functionality
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