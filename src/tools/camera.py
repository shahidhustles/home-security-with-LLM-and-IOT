"""Camera snapshot tools."""

import logging
from src.config import Config
from src.utils.http_client import ESP32Client
from src.utils.mock_data import get_mock_camera_snapshot

logger = logging.getLogger(__name__)
esp32_client = ESP32Client()


async def get_camera_snapshot() -> dict:
    """Capture current camera image from ESP32-CAM.

    Returns:
        Dictionary with camera status and snapshot URL
    """
    logger.info("Attempting to capture camera snapshot")

    if Config.MOCK_MODE:
        logger.debug("Using mock response for get_camera_snapshot")
        return get_mock_camera_snapshot()

    # Real ESP32 call
    response = await esp32_client.get("/api/snapshot")

    if "error" in response.get("status", "").lower():
        logger.error(f"Failed to capture snapshot: {response.get('message')}")
    else:
        logger.info(f"Snapshot captured successfully: {response}")

    return response
