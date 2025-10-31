"""Door control tools (lock/unlock operations)."""

import logging
from src.config import Config
from src.utils.http_client import ESP32Client
from src.utils.mock_data import (
    get_mock_lock_response,
    get_mock_unlock_response,
    get_mock_door_status,
)

logger = logging.getLogger(__name__)
esp32_client = ESP32Client()


async def lock_door(device_id: str = "main_door") -> dict:
    """Lock a door using solenoid.

    Args:
        device_id: Identifier of the door to lock (default: main_door)

    Returns:
        Dictionary with lock status and timestamp
    """
    logger.info(f"Attempting to lock door: {device_id}")

    if Config.MOCK_MODE:
        logger.debug("Using mock response for lock_door")
        return get_mock_lock_response()

    # Real ESP32 call
    response = await esp32_client.post("/api/lock", {"device_id": device_id})

    if "error" in response.get("status", "").lower():
        logger.error(f"Failed to lock door: {response.get('message')}")
    else:
        logger.info(f"Door locked successfully: {response}")

    return response


async def unlock_door(device_id: str = "main_door") -> dict:
    """Unlock a door by deactivating solenoid.

    Args:
        device_id: Identifier of the door to unlock (default: main_door)

    Returns:
        Dictionary with unlock status and timestamp
    """
    logger.info(f"Attempting to unlock door: {device_id}")

    if Config.MOCK_MODE:
        logger.debug("Using mock response for unlock_door")
        return get_mock_unlock_response()

    # Real ESP32 call
    response = await esp32_client.post("/api/unlock", {"device_id": device_id})

    if "error" in response.get("status", "").lower():
        logger.error(f"Failed to unlock door: {response.get('message')}")
    else:
        logger.info(f"Door unlocked successfully: {response}")

    return response


async def get_door_status(device_id: str = "main_door") -> dict:
    """Check if a door is locked or unlocked.

    Args:
        device_id: Identifier of the door to check (default: main_door)

    Returns:
        Dictionary with lock status
    """
    logger.info(f"Checking door status: {device_id}")

    if Config.MOCK_MODE:
        logger.debug("Using mock response for get_door_status")
        return get_mock_door_status()

    # Real ESP32 call
    response = await esp32_client.get("/api/door/status", {"device_id": device_id})

    if "error" in response.get("status", "").lower():
        logger.error(f"Failed to get door status: {response.get('message')}")
    else:
        logger.info(f"Door status: {response}")

    return response
