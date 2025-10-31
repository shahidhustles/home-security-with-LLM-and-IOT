"""HTTP client for ESP32 communication."""

import httpx
import logging
from typing import Any, Dict, Optional
from src.config import Config

logger = logging.getLogger(__name__)


class ESP32Client:
    """Async HTTP client for ESP32 REST API communication."""

    def __init__(self, base_url: str = Config.ESP32_IP, timeout: int = Config.ESP32_TIMEOUT):
        """Initialize ESP32 client.

        Args:
            base_url: Base URL of ESP32 server
            timeout: HTTP request timeout in seconds
        """
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout

    async def post(self, endpoint: str, data: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """Send POST request to ESP32.

        Args:
            endpoint: API endpoint (e.g., "/api/lock")
            data: JSON body to send

        Returns:
            Response as dictionary

        Raises:
            Returns error dict if connection fails (doesn't raise exception)
        """
        try:
            url = f"{self.base_url}{endpoint}"
            logger.debug(f"POST {url}")
            async with httpx.AsyncClient(timeout=self.timeout) as client:
                response = await client.post(url, json=data or {})
                response.raise_for_status()
                return response.json()
        except httpx.TimeoutException:
            error_msg = f"ESP32 timeout after {self.timeout}s: {endpoint}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
        except httpx.ConnectError as e:
            error_msg = f"Cannot connect to ESP32 at {self.base_url}: {str(e)}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
        except httpx.RequestError as e:
            error_msg = f"ESP32 request error: {str(e)}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
        except Exception as e:
            error_msg = f"Unexpected error communicating with ESP32: {str(e)}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}

    async def get(self, endpoint: str, params: Optional[Dict[str, str]] = None) -> Dict[str, Any]:
        """Send GET request to ESP32.

        Args:
            endpoint: API endpoint (e.g., "/api/door/status")
            params: Query parameters

        Returns:
            Response as dictionary

        Raises:
            Returns error dict if connection fails (doesn't raise exception)
        """
        try:
            url = f"{self.base_url}{endpoint}"
            logger.debug(f"GET {url}")
            async with httpx.AsyncClient(timeout=self.timeout) as client:
                response = await client.get(url, params=params or {})
                response.raise_for_status()
                return response.json()
        except httpx.TimeoutException:
            error_msg = f"ESP32 timeout after {self.timeout}s: {endpoint}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
        except httpx.ConnectError as e:
            error_msg = f"Cannot connect to ESP32 at {self.base_url}: {str(e)}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
        except httpx.RequestError as e:
            error_msg = f"ESP32 request error: {str(e)}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
        except Exception as e:
            error_msg = f"Unexpected error communicating with ESP32: {str(e)}"
            logger.error(error_msg)
            return {"status": "error", "message": error_msg}
