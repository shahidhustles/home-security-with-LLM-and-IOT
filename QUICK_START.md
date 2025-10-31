# 🚀 Quick Start - 5 Minutes to Demo

Follow these steps to get the Smart Security MCP Server running with Claude Desktop.

## Prerequisites

- Python 3.10+ installed
- Claude Desktop installed
- A terminal/command line

## Step 1: Setup (2 minutes)

```bash
# Navigate to project
cd /Users/shahidpatel/cps/ldam-cp

# Create virtual environment
python3 -m venv venv

# Activate it
source venv/bin/activate  # macOS/Linux
# or
venv\Scripts\activate     # Windows

# Install dependencies
pip install -r requirements.txt

# Copy environment file
cp .env.example .env
```

## Step 2: Start MCP Server (1 minute)

```bash
# Make sure venv is activated, then:
python src/server.py
```

You should see:

```
Starting Smart Security MCP Server
Mock Mode: True
ESP32 IP: http://192.168.1.100
```

**Keep this terminal open!**

## Step 3: Configure Claude Desktop (2 minutes)

### macOS:

```bash
open ~/Library/Application\ Support/Claude/
```

### Windows:

```
Open: %APPDATA%\Claude\
```

### Linux:

```
Open: ~/.config/Claude/
```

**Create or edit `claude_desktop_config.json`:**

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

**Get the absolute path:**

```bash
cd /Users/shahidpatel/cps/ldam-cp
pwd
```

Then replace `/absolute/path/to/ldam-cp` in the config file.

### Restart Claude Desktop:

- Close Claude Desktop completely
- Open Claude Desktop again
- Look for the wrench icon in toolbar (MCP connected!)

## Step 4: Test It! (Interactive)

Ask Claude any of these:

```
"Lock the door"
"Is the door locked?"
"What's the security status?"
"Did I forget to lock anything?"
"Show me a camera snapshot"
"Was there any motion detected?"
```

## ✅ You're Done!

The system is now running in **Mock Mode** with simulated device responses. No hardware needed for testing!

---

## What's Next?

### To use with real ESP32 hardware:

1. Follow the guide in `esp32_code/ESP32_SETUP_README.md`
2. Upload the Arduino code to your ESP32
3. Note the IP address from Serial Monitor
4. Update `.env`:
   ```
   MOCK_MODE=false
   ESP32_IP=http://192.168.1.xxx
   ```
5. Restart the MCP server

### That's it! Now Claude controls your real security system. 🔐

---

## Troubleshooting

### MCP not showing in Claude

1. Verify config file path is correct
2. Check Python path is absolute (use `pwd`)
3. Restart Claude completely
4. Ensure server is running in terminal

### Server won't start

```bash
# Make sure venv is activated
source venv/bin/activate

# Reinstall dependencies
pip install -r requirements.txt

# Try again
python src/server.py
```

---

## Demo Queries Reference

### Basic Control

```
"Lock the main door"
"Unlock the front door"
"Is my door locked?"
```

### Status Checks

```
"What's the security status?"
"Check all doors"
"Is everything secure?"
```

### Motion & Camera

```
"Did you detect any motion?"
"When was the last motion?"
"Take a snapshot"
"Show me the camera"
```

### Context-Aware (Best!)

```
"Did I forget to lock anything?"
"Are all my doors secured?"
"Give me a security summary"
"Is anything unlocked?"
```

### Multilingual (Claude handles it!)

```
"Darwaza band karo" (Hindi: Lock the door)
"Darwaza kholo" (Hindi: Unlock the door)
"¿La puerta está cerrada?" (Spanish: Is the door closed?)
```

---

## Tips for Best Results

1. **Use natural language** - No need for rigid commands
2. **Be specific** - "Lock the main door" vs just "Lock"
3. **Ask context questions** - "Did I forget to lock?" is powerful
4. **Multiple languages** - Claude will translate
5. **Combine queries** - "Lock the door and check for motion"

---

**Questions?** Check the main README.md for detailed documentation.

**Ready to demo?** Your AI security system is live! 🚀
