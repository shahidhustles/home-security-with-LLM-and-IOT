"""Motion sensor tools."""

import logging
from src.config import Config
from src.utils.http_client import ESP32Client
from src.utils.mock_data import get_mock_motion_status

logger = logging.getLogger(__name__)
esp32_client = ESP32Client()


async def get_motion_status() -> dict:
    """Check motion sensor state.

    Returns:
        Dictionary with motion detection status and timestamp information
    """
    logger.info("Checking motion sensor status")

    if Config.MOCK_MODE:
        logger.debug("Using mock response for get_motion_status")
        return get_mock_motion_status()

    # Real ESP32 call
    response = await esp32_client.get("/api/motion")

    if "error" in response.get("status", "").lower():
        logger.error(f"Failed to get motion status: {response.get('message')}")
    else:
        logger.info(f"Motion status: {response}")

    return response
