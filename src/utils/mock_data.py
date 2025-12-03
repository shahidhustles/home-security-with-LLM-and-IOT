"""Mock data for testing without ESP32."""

import time
from typing import Dict, Any

# Mock device state
_mock_state = {
    "door_locked": True,
    "motion_detected": False,
    "last_motion_time": int(time.time()) - 300,  # 5 minutes ago
}


def get_mock_lock_response() -> Dict[str, Any]:
    """Get mock lock response."""
    _mock_state["door_locked"] = True
    return {
        "status": "locked",
        "device": "main_door",
        "timestamp": int(time.time()),
        "mock": True,
    }


def get_mock_unlock_response() -> Dict[str, Any]:
    """Get mock unlock response."""
    _mock_state["door_locked"] = False
    return {
        "status": "unlocked",
        "device": "main_door",
        "timestamp": int(time.time()),
        "mock": True,
    }


def get_mock_door_status() -> Dict[str, Any]:
    """Get mock door status."""
    return {
        "locked": _mock_state["door_locked"],
        "device": "main_door",
        "timestamp": int(time.time()),
        "mock": True,
    }


def get_mock_motion_status() -> Dict[str, Any]:
    """Get mock motion status."""
    seconds_ago = int(time.time()) - _mock_state["last_motion_time"]
    return {
        "motion_detected": _mock_state["motion_detected"],
        "last_motion_time": _mock_state["last_motion_time"],
        "seconds_ago": seconds_ago,
        "mock": True,
    }


def get_mock_camera_snapshot() -> Dict[str, Any]:
    """Get mock camera snapshot with AI description."""
    return {
        "status": "ok",
        "image_url": "https://gofile.io/d/mock_snapshot",
        "view_image_here": "📷 Click to view: https://gofile.io/d/mock_snapshot",
        "ai_description": "[MOCK] The camera shows a well-lit living room. No people detected. A couch is visible in the center of the frame with a coffee table in front. Natural light coming from the left side suggests a window. No suspicious activity or motion detected. The room appears secure.",
        "image_size": 45000,
        "timestamp": int(time.time()),
        "mock": True,
    }


def trigger_mock_motion() -> None:
    """Simulate motion detection for testing."""
    _mock_state["motion_detected"] = True
    _mock_state["last_motion_time"] = int(time.time())


def clear_mock_motion() -> None:
    """Clear motion detection state."""
    _mock_state["motion_detected"] = False
