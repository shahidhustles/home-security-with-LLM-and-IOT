"""Configuration loader for MCP Security Server."""

import os
from dotenv import load_dotenv
import logging

# Load environment variables from .env file
load_dotenv()


class Config:
    """Configuration settings for the security MCP server."""

    # ESP32 Configuration
    ESP32_IP: str = os.getenv("ESP32_IP", "http://192.168.1.100")
    ESP32_TIMEOUT: int = int(os.getenv("ESP32_TIMEOUT", "5"))

    # Mock Mode
    MOCK_MODE: bool = os.getenv("MOCK_MODE", "true").lower() == "true"

    # Logging
    LOG_LEVEL: str = os.getenv("LOG_LEVEL", "INFO")

    @classmethod
    def setup_logging(cls):
        """Configure logging for the application."""
        logging.basicConfig(
            level=cls.LOG_LEVEL,
            format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        )

    @classmethod
    def log_config(cls):
        """Log current configuration (useful for debugging)."""
        logger = logging.getLogger(__name__)
        logger.info(f"Configuration: ESP32_IP={cls.ESP32_IP}")
        logger.info(f"Configuration: MOCK_MODE={cls.MOCK_MODE}")
        logger.info(f"Configuration: LOG_LEVEL={cls.LOG_LEVEL}")
