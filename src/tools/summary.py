"""Summary and aggregation tools."""

import logging
from src.config import Config
from src.tools.door_control import get_door_status
from src.tools.motion_sensor import get_motion_status
from src.tools.camera import get_camera_snapshot

logger = logging.getLogger(__name__)


async def get_security_summary() -> dict:
    """Get aggregate status of all security devices.

    Returns:
        Dictionary with combined status of all devices
    """
    logger.info("Generating security summary")

    try:
        # Get status of all devices
        door_status = await get_door_status()
        motion_status = await get_motion_status()
        camera_status = await get_camera_snapshot()

        # Aggregate results
        summary = {
            "timestamp": int(__import__("time").time()),
            "devices": {
                "door": {
                    "locked": door_status.get("locked", False),
                    "status": door_status.get("status", "unknown"),
                },
                "motion": {
                    "detected": motion_status.get("motion_detected", False),
                    "last_motion_time": motion_status.get("last_motion_time"),
                    "seconds_since_motion": motion_status.get("seconds_ago"),
                },
                "camera": {
                    "status": camera_status.get("status", "error"),
                    "snapshot_url": camera_status.get("url"),
                },
            },
            "overall_status": "secure" if door_status.get("locked") else "unsecured",
            "mock": Config.MOCK_MODE,
        }

        logger.info(f"Security summary generated: {summary['overall_status']}")
        return summary

    except Exception as e:
        error_msg = f"Error generating security summary: {str(e)}"
        logger.error(error_msg)
        return {
            "status": "error",
            "message": error_msg,
        }


async def check_unlocked_devices() -> dict:
    """Find all unlocked or unsecured devices.

    Returns:
        Dictionary with list of unlocked devices or confirmation all are secured
    """
    logger.info("Checking for unlocked devices")

    try:
        door_status = await get_door_status()
        unlocked_devices = []

        # Check if door is unlocked
        if not door_status.get("locked", True):
            unlocked_devices.append(
                {
                    "device": "main_door",
                    "status": door_status.get("status", "unknown"),
                }
            )

        if unlocked_devices:
            result = {
                "unsecured": True,
                "count": len(unlocked_devices),
                "devices": unlocked_devices,
                "message": f"Found {len(unlocked_devices)} unlocked device(s)",
            }
            logger.warning(f"Found unlocked devices: {unlocked_devices}")
        else:
            result = {
                "unsecured": False,
                "count": 0,
                "devices": [],
                "message": "All devices are secured!",
            }
            logger.info("All devices are secured")

        return result

    except Exception as e:
        error_msg = f"Error checking unlocked devices: {str(e)}"
        logger.error(error_msg)
        return {
            "status": "error",
            "message": error_msg,
        }
