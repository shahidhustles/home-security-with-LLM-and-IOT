# Smart Security and Surveillance Assistant - MCP Server

A conversational IoT security system that allows users to control door locks, cameras, and motion sensors through natural language queries with Claude AI.

**Architecture:** Claude Desktop → MCP Python Server → ESP32 REST API → Hardware (Locks, Sensors, Cameras)

**Current Status:** ✅ Ready for Claude Desktop integration with Mock Mode testing

## Quick Start (5 minutes)

### 1. Setup Python Environment

```bash
cd /Users/shahidpatel/cps/ldam-cp

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Copy environment file
cp .env.example .env
```

### 2. Start MCP Server (Mock Mode)

```bash
# Make sure you're in the project directory with venv activated
python src/server.py
```

You should see:

```
Starting Smart Security MCP Server
Mock Mode: True
ESP32 IP: http://192.168.1.100
```

Keep this running in a terminal.

### 3. Configure Claude Desktop

#### macOS:

```bash
# Open Claude Desktop config directory
open ~/Library/Application\ Support/Claude/
# Edit claude_desktop_config.json
```

#### Windows:

```
Edit: %APPDATA%\Claude\claude_desktop_config.json
```

#### Linux:

```
Edit: ~/.config/Claude/claude_desktop_config.json
```

**Add this content:**

```json
{
  "mcpServers": {
    "smart-security": {
      "command": "python",
      "args": ["/absolute/path/to/ldam-cp/src/server.py"]
    }
  }
}
```

⚠️ **Replace `/absolute/path/to/ldam-cp` with your actual path!**

Get the full path:

```bash
cd /Users/shahidpatel/cps/ldam-cp
pwd
```

### 4. Restart Claude Desktop

- Close Claude Desktop completely
- Open Claude Desktop again
- You should see a small wrench icon in the toolbar indicating MCP is connected

### 5. Test with Claude

Ask Claude questions like:

```
"Lock the door"
"Is the door locked?"
"Check the security status"
"Did I forget to lock anything?"
"Show me the camera snapshot"
"Was there any motion detected?"
```

---

## Project Structure

```
ldam-cp/
├── src/
│   ├── server.py                 # Main MCP server (entry point)
│   ├── config.py                 # Configuration loader
│   ├── tools/
│   │   ├── __init__.py
│   │   ├── door_control.py       # lock_door, unlock_door, get_door_status
│   │   ├── motion_sensor.py      # get_motion_status
│   │   ├── camera.py             # get_camera_snapshot
│   │   └── summary.py            # get_security_summary, check_unlocked_devices
│   └── utils/
│       ├── __init__.py
│       ├── http_client.py        # ESP32 HTTP communication
│       └── mock_data.py          # Mock responses for testing
├── esp32_code/
│   ├── smart_security.ino        # Arduino sketch for ESP32
│   └── ESP32_SETUP_README.md     # Detailed ESP32 setup guide
├── tests/
│   └── (tests coming soon)
├── .env.example                  # Environment template
├── .env                          # Actual config (don't commit)
├── requirements.txt              # Python dependencies
├── claude_desktop_config.json.example  # Claude config template
└── README.md                     # This file
```

---

## MCP Tools Available

### Door Control

- **`lock_door(device_id="main_door")`** - Lock a door
- **`unlock_door(device_id="main_door")`** - Unlock a door
- **`get_door_status(device_id="main_door")`** - Check if locked/unlocked

### Motion Detection

- **`get_motion_status()`** - Get motion sensor state and last motion time

### Camera

- **`get_camera_snapshot()`** - Capture ESP32-CAM snapshot

### Summary & Context-Aware

- **`get_security_summary()`** - Aggregate all device status
- **`check_unlocked_devices()`** - Find unsecured devices (answers "Did I forget to lock?")

### Example Claude Queries

```
# Status checks
"What's the door status?"
"Is motion being detected?"
"Show me the camera"

# Actions
"Lock the door please"
"Unlock the front door"

# Context-aware
"Are all doors secure?"
"Did I forget to lock anything?"
"Give me a security update"

# Multilingual (Claude handles translation)
"Darwaza band karo" (Hindi for "lock the door")
"¿La puerta está cerrada?" (Spanish for "is the door locked?")
```

---

## Development Modes

### Mock Mode (Testing without Hardware) ✅

**Perfect for:** Initial setup, testing Claude integration, demonstration

```bash
# .env file
MOCK_MODE=true
ESP32_IP=http://192.168.1.100
```

- Returns simulated device responses
- No hardware required
- Fast, predictable testing
- Excellent for iterating on Claude prompts

### Real ESP32 Mode (Production)

**After you've uploaded code to ESP32:**

```bash
# .env file
MOCK_MODE=false
ESP32_IP=http://192.168.1.100   # Your actual ESP32 IP
ESP32_TIMEOUT=5                  # Adjust as needed
```

**First:** Upload the Arduino sketch to ESP32! See `esp32_code/ESP32_SETUP_README.md`

---

## Configuration

### Environment Variables (.env)

```bash
# ESP32 Configuration
ESP32_IP=http://192.168.1.100    # IP address of your ESP32
ESP32_TIMEOUT=5                   # HTTP request timeout (seconds)

# Testing Mode
MOCK_MODE=true                    # Set to 'false' when ESP32 is connected

# Logging
LOG_LEVEL=INFO                    # DEBUG, INFO, WARNING, ERROR
```

### Enable Debug Logging

```bash
LOG_LEVEL=DEBUG
```

Monitor logs in the terminal where you're running the server.

---

## ESP32 Hardware Setup

See `esp32_code/ESP32_SETUP_README.md` for **complete ESP32 setup instructions**.

### Quick Summary

1. **Install Arduino IDE**
2. **Add ESP32 board support**
3. **Install libraries:** ArduinoJson, AsyncTCP, ESPAsyncWebServer
4. **Edit WiFi credentials** in `smart_security.ino`
5. **Upload to ESP32**
6. **Note the IP address** from Serial Monitor
7. **Update `.env`** with `MOCK_MODE=false` and ESP32 IP
8. **Restart MCP server**

---

## Testing

### Manual Tool Testing

Test the MCP server directly:

```bash
# In a new terminal, with venv activated:
python -c "
import asyncio
from src.tools.door_control import lock_door

result = asyncio.run(lock_door())
print(result)
"
```

### HTTP Testing (ESP32 Direct)

Test ESP32 API endpoints directly:

```bash
# Check if ESP32 is running
curl http://192.168.1.100/api/health

# Test lock endpoint
curl -X POST http://192.168.1.100/api/lock

# Check door status
curl http://192.168.1.100/api/door/status

# Check motion
curl http://192.168.1.100/api/motion
```

---

## Troubleshooting

### MCP Server Won't Start

**Error:** `ModuleNotFoundError: No module named 'mcp'`

```bash
# Make sure virtual environment is activated
source venv/bin/activate

# Reinstall dependencies
pip install -r requirements.txt
```

### Claude Says "Unknown Tool"

**Issue:** MCP server isn't properly connected

1. Check Claude Desktop config file exists at:

   - macOS: `~/Library/Application Support/Claude/claude_desktop_config.json`
   - Windows: `%APPDATA%\Claude\claude_desktop_config.json`

2. Verify the Python path is absolute and correct

3. Restart Claude Desktop completely

4. Ensure server is running in terminal

### ESP32 Connection Error

**Error:** "Cannot connect to ESP32 at http://192.168.1.100"

1. Verify ESP32 is powered and connected to WiFi
2. Confirm IP address matches ESP32 output
3. Test with: `curl http://192.168.1.100/api/health`
4. Check firewall isn't blocking port 80

---

## Hardcoding Data for Demo

The ESP32 code stores device state in variables. For demo without real hardware:

### In `smart_security.ino`:

```cpp
// Device State (Hardcoded data for demo)
bool door_locked = true;                          // Initial: locked
unsigned long last_motion_timestamp = 0;          // Initial: no motion
bool motion_currently_detected = false;            // Initial: no motion now
```

You can hardcode different states for demonstration purposes.

---

## Development Workflow

### Phase 1: Mock Testing ✅ (Current)

```bash
# 1. Create virtual environment
python3 -m venv venv
source venv/bin/activate

# 2. Install dependencies
pip install -r requirements.txt

# 3. Copy config
cp .env.example .env

# 4. Start server
python src/server.py

# 5. Configure Claude Desktop
# Edit ~/Library/Application Support/Claude/claude_desktop_config.json

# 6. Restart Claude and test queries
```

### Phase 2: ESP32 Integration

```bash
# 1. Upload Arduino sketch to ESP32
# See: esp32_code/ESP32_SETUP_README.md

# 2. Note ESP32 IP from Serial Monitor

# 3. Update .env
ESP32_IP=http://192.168.1.xxx
MOCK_MODE=false

# 4. Restart MCP server
python src/server.py

# 5. Test with real hardware in Claude
```

### Phase 3: Production (HTTP/SSE Transport)

```bash
# Later, switch to HTTP for production deployment
# This will allow multiple concurrent connections
```

---

## API Endpoints (ESP32)

The Python MCP server calls these endpoints on the ESP32:

| Method | Endpoint           | Purpose                  |
| ------ | ------------------ | ------------------------ |
| POST   | `/api/lock`        | Lock the door            |
| POST   | `/api/unlock`      | Unlock the door          |
| GET    | `/api/door/status` | Get lock status          |
| GET    | `/api/motion`      | Get motion sensor status |
| GET    | `/api/snapshot`    | Get camera snapshot URL  |
| GET    | `/api/health`      | Health check             |

See `esp32_code/smart_security.ino` for implementation details.

---

## Key Features

✅ **Natural Language Processing** - Claude understands context ("Did I forget to lock?")  
✅ **Async Operations** - Non-blocking HTTP to ESP32  
✅ **Error Handling** - Graceful failures with helpful messages  
✅ **Mock Mode** - Test without hardware  
✅ **Real Hardware** - Seamless transition from mock to production  
✅ **REST API** - Standard JSON communication  
✅ **Security Focused** - Proper error handling and logging

---

## Demo Ready! 🚀

Once setup is complete:

1. Ask Claude natural language security questions
2. Control devices without rigid commands
3. Get context-aware responses
4. Multilingual support via Claude's translation

**Your AI-powered security system is ready to demonstrate!**

---

## Next Steps

1. ✅ Run MCP server in Mock Mode
2. ✅ Configure Claude Desktop
3. ✅ Test natural language queries
4. ⬜ Upload ESP32 code (see `esp32_code/ESP32_SETUP_README.md`)
5. ⬜ Update `.env` with ESP32 IP
6. ⬜ Set `MOCK_MODE=false`
7. ⬜ Test with real hardware
8. 🎉 Demo!

---

## Project Status

- [x] MCP Server with FastMCP
- [x] All 7 security tools implemented
- [x] Mock mode for testing
- [x] Error handling with descriptive messages
- [x] Configuration system (.env)
- [x] Comprehensive logging
- [x] ESP32 Arduino code template
- [x] ESP32 setup documentation
- [ ] Unit tests
- [ ] Video streaming support (future)
- [ ] Cloud integration (future)
