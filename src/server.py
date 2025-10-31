#!/usr/bin/env python3
"""
Smart Security and Surveillance Assistant MCP Server.

This MCP server provides tools for controlling and monitoring smart security devices
including door locks, motion sensors, and cameras through natural language queries
via Claude.

Usage:
    python src/server.py

Environment variables (see .env.example):
    ESP32_IP: IP address of ESP32 server (default: http://192.168.1.100)
    ESP32_TIMEOUT: HTTP timeout in seconds (default: 5)
    MOCK_MODE: Use mock responses without ESP32 (default: true)
    LOG_LEVEL: Logging level (default: INFO)

Configure in Claude Desktop:
    ~/.config/Claude/claude_desktop_config.json or
    ~/Library/Application Support/Claude/claude_desktop_config.json

    {
      "mcpServers": {
        "smart-security": {
          "command": "python",
          "args": ["/absolute/path/to/src/server.py"]
        }
      }
    }
"""

import asyncio
import logging
import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from mcp.server.fastmcp import FastMCP
from src.config import Config
from src.tools.door_control import lock_door, unlock_door, get_door_status
from src.tools.motion_sensor import get_motion_status
from src.tools.camera import get_camera_snapshot
from src.tools.summary import get_security_summary, check_unlocked_devices

# Initialize logging
Config.setup_logging()
logger = logging.getLogger(__name__)

# Log configuration on startup
Config.log_config()

# Create MCP server
mcp = FastMCP(
    "Smart Security System",
    instructions="""You are a smart home security assistant. You have access to tools 
that control and monitor door locks, motion sensors, and cameras. Help the user 
manage their security by answering queries about device status and executing control 
commands. Support natural language queries in any language (you handle the translation).""",
)


# ============================================================================
# DOOR CONTROL TOOLS
# ============================================================================


@mcp.tool()
async def lock_door_tool(device_id: str = "main_door") -> dict:
    """Lock a door using solenoid.

    This tool activates the door lock mechanism. Use this when user wants to
    secure a door.

    Args:
        device_id: The identifier of the door to lock (e.g., "main_door", "back_door")

    Returns:
        Dictionary with:
            - status: "locked" or error message
            - device: device identifier
            - timestamp: Unix timestamp of operation
            - message: Human-readable status message (if error)
    """
    logger.info(f"Tool called: lock_door with device_id={device_id}")
    result = await lock_door(device_id)
    logger.debug(f"lock_door result: {result}")
    return result


@mcp.tool()
async def unlock_door_tool(device_id: str = "main_door") -> dict:
    """Unlock a door by deactivating solenoid.

    Use this tool when user wants to unlock a door. Be cautious and confirm
    the user's intent before calling this tool.

    Args:
        device_id: The identifier of the door to unlock (e.g., "main_door", "back_door")

    Returns:
        Dictionary with:
            - status: "unlocked" or error message
            - device: device identifier
            - timestamp: Unix timestamp of operation
            - message: Human-readable status message (if error)
    """
    logger.info(f"Tool called: unlock_door with device_id={device_id}")
    result = await unlock_door(device_id)
    logger.debug(f"unlock_door result: {result}")
    return result


@mcp.tool()
async def get_door_status_tool(device_id: str = "main_door") -> dict:
    """Check if a door is locked or unlocked.

    Query the current state of a door lock. Useful for security checks and
    status inquiries.

    Args:
        device_id: The identifier of the door to check (e.g., "main_door", "back_door")

    Returns:
        Dictionary with:
            - locked: Boolean indicating if door is locked
            - device: device identifier
            - status: Textual status ("locked" or "unlocked")
            - timestamp: Unix timestamp of query
            - message: Human-readable status message (if error)
    """
    logger.info(f"Tool called: get_door_status with device_id={device_id}")
    result = await get_door_status(device_id)
    logger.debug(f"get_door_status result: {result}")
    return result


# ============================================================================
# MOTION SENSOR TOOLS
# ============================================================================


@mcp.tool()
async def get_motion_status_tool() -> dict:
    """Check motion sensor status.

    Retrieve motion detection data from PIR (Passive Infrared) sensor.
    Useful for checking if motion was recently detected in the area.

    Returns:
        Dictionary with:
            - motion_detected: Boolean indicating current motion state
            - last_motion_time: Unix timestamp of last detected motion
            - seconds_ago: How many seconds ago motion was last detected
            - message: Human-readable status message (if error)
    """
    logger.info("Tool called: get_motion_status")
    result = await get_motion_status()
    logger.debug(f"get_motion_status result: {result}")
    return result


# ============================================================================
# CAMERA TOOLS
# ============================================================================


@mcp.tool()
async def get_camera_snapshot_tool() -> dict:
    """Capture current camera image.

    Get a snapshot from ESP32-CAM. The image is saved on the ESP32 and
    accessible via the returned URL.

    Returns:
        Dictionary with:
            - status: "ok" if successful, "error" if failed
            - url: HTTP URL to access the snapshot image
            - timestamp: Unix timestamp of snapshot capture
            - message: Human-readable status message (if error)
    """
    logger.info("Tool called: get_camera_snapshot")
    result = await get_camera_snapshot()
    logger.debug(f"get_camera_snapshot result: {result}")
    return result


# ============================================================================
# SUMMARY AND CONTEXT-AWARE TOOLS
# ============================================================================


@mcp.tool()
async def get_security_summary_tool() -> dict:
    """Get complete security system status.

    Aggregate status of all security devices (door, motion, camera).
    Use this for comprehensive security overview queries like
    "What's the security status?" or "Is everything secure?"

    Returns:
        Dictionary with:
            - timestamp: Unix timestamp
            - devices: Object containing status of each device:
                - door: {locked: bool, status: str}
                - motion: {detected: bool, last_motion_time: int, seconds_since_motion: int}
                - camera: {status: str, snapshot_url: str}
            - overall_status: "secure" or "unsecured"
            - message: Human-readable summary (if error)
    """
    logger.info("Tool called: get_security_summary")
    result = await get_security_summary()
    logger.debug(f"get_security_summary result: {result}")
    return result


@mcp.tool()
async def check_unlocked_devices_tool() -> dict:
    """Check for unlocked or unsecured devices.

    Context-aware query to find any devices that are not in a secure state.
    Useful for queries like "Did I forget to lock anything?" or
    "Are all doors locked?"

    Returns:
        Dictionary with:
            - unsecured: Boolean if any devices are unsecured
            - count: Number of unsecured devices
            - devices: List of unsecured devices with details
            - message: Human-readable result message
    """
    logger.info("Tool called: check_unlocked_devices")
    result = await check_unlocked_devices()
    logger.debug(f"check_unlocked_devices result: {result}")
    return result


# ============================================================================
# SERVER LIFECYCLE
# ============================================================================


def main():
    """Run the MCP server using STDIO transport."""
    logger.info("Starting Smart Security MCP Server")
    logger.info(f"Mock Mode: {Config.MOCK_MODE}")
    logger.info(f"ESP32 IP: {Config.ESP32_IP}")

    try:
        # Run the server on standard I/O (mcp.run() handles asyncio internally)
        mcp.run()
    except KeyboardInterrupt:
        logger.info("Server interrupted by user")
    except Exception as e:
        logger.error(f"Server error: {str(e)}", exc_info=True)
        raise


if __name__ == "__main__":
    main()
