"""Camera snapshot tools with Gemini Vision AI analysis."""

import logging
import time
import httpx
from google import genai
from google.genai import types
from src.config import Config
from src.utils.mock_data import get_mock_camera_snapshot

logger = logging.getLogger(__name__)

# Gemini client (lazy initialization)
_gemini_client = None


def get_gemini_client():
    """Get or create Gemini client."""
    global _gemini_client
    if _gemini_client is None:
        if not Config.GEMINI_API_KEY:
            raise ValueError("GEMINI_API_KEY not configured")
        _gemini_client = genai.Client(api_key=Config.GEMINI_API_KEY)
    return _gemini_client


async def analyze_image_with_gemini(image_data: bytes) -> str:
    """Analyze image using Gemini 2.5 Flash vision.

    Args:
        image_data: Raw JPEG image bytes

    Returns:
        Text description of what Gemini sees in the image
    """
    try:
        client = get_gemini_client()

        # Security-focused prompt for home camera analysis
        prompt = """You are a home security camera AI assistant. Analyze this camera image and provide a concise security report.

Focus on:
1. People: How many, what they're doing, any identifying features
2. Activity: Normal or suspicious behavior
3. Environment: Lighting, visibility, any notable objects
4. Security concerns: Anything unusual or potentially concerning

Be concise but thorough. If you can't see clearly, mention that."""

        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=[
                prompt,
                types.Part.from_bytes(data=image_data, mime_type="image/jpeg"),
            ],
        )

        return response.text

    except Exception as e:
        logger.error(f"Gemini vision analysis failed: {str(e)}")
        return f"Vision analysis unavailable: {str(e)}"


async def get_gofile_server() -> str:
    """Get the best Gofile server for upload."""
    try:
        async with httpx.AsyncClient(timeout=10) as client:
            response = await client.get("https://api.gofile.io/servers")
            data = response.json()
            if data.get("status") == "ok":
                return data["data"]["servers"][0]["name"]
    except Exception:
        pass
    return "store1"  # fallback


async def upload_image(image_data: bytes) -> dict:
    """Upload image to Gofile anonymously.

    Args:
        image_data: Raw image bytes

    Returns:
        Dictionary with upload result containing image URL
    """
    try:
        # Get best server
        server = await get_gofile_server()
        upload_url = f"https://{server}.gofile.io/contents/uploadfile"

        files = {"file": ("snapshot.jpg", image_data, "image/jpeg")}
        headers = {"User-Agent": "Mozilla/5.0 (compatible; SmartSecurity/1.0)"}

        async with httpx.AsyncClient(timeout=30) as client:
            response = await client.post(upload_url, files=files, headers=headers)
            response.raise_for_status()

            result = response.json()

            if result.get("status") == "ok":
                # Gofile returns downloadPage URL
                download_url = result["data"]["downloadPage"]
                return {"success": True, "url": download_url}
            else:
                return {"success": False, "error": result.get("status", "Upload failed")}

    except Exception as e:
        logger.error(f"Image upload failed: {str(e)}")
        return {"success": False, "error": str(e)}


async def get_camera_snapshot() -> dict:
    """Capture current camera image from ESP32-CAM and analyze with Gemini AI.

    Returns:
        Dictionary with:
            - status: "ok" or "error"
            - image_url: Public URL to view the image
            - view_image_here: Formatted link for display
            - ai_description: Gemini's analysis of what's in the image
            - image_size: Size of image in bytes
            - timestamp: Unix timestamp of capture
            - message: Error message if failed
    """
    logger.info("Attempting to capture camera snapshot")

    if Config.MOCK_MODE:
        logger.debug("Using mock response for get_camera_snapshot")
        return get_mock_camera_snapshot()

    try:
        url = f"{Config.ESP32_IP}/api/snapshot"
        logger.debug(f"Fetching image from {url}")

        async with httpx.AsyncClient(timeout=Config.ESP32_TIMEOUT) as client:
            response = await client.get(url)
            response.raise_for_status()

            # Get image binary data
            image_data = response.content
            logger.info(f"Snapshot captured successfully: {len(image_data)} bytes")

            # Analyze image with Gemini Vision AI
            logger.info("Analyzing image with Gemini 2.5 Flash...")
            ai_description = await analyze_image_with_gemini(image_data)
            logger.info("Gemini analysis complete")

            # Upload to Gofile for user-viewable URL
            upload_result = await upload_image(image_data)

            image_url = None
            if upload_result["success"]:
                image_url = upload_result["url"]
                logger.info(f"Image uploaded: {image_url}")
            else:
                logger.warning(f"Image upload failed: {upload_result.get('error')}")

            return {
                "status": "ok",
                "image_url": image_url,
                "view_image_here": (
                    f"📷 Click to view: {image_url}" if image_url else "Upload failed"
                ),
                "ai_description": ai_description,
                "image_size": len(image_data),
                "timestamp": int(time.time()),
            }

    except httpx.TimeoutException:
        error_msg = f"Camera timeout after {Config.ESP32_TIMEOUT}s"
        logger.error(error_msg)
        return {"status": "error", "message": error_msg}
    except httpx.ConnectError as e:
        error_msg = f"Cannot connect to camera at {Config.ESP32_IP}: {str(e)}"
        logger.error(error_msg)
        return {"status": "error", "message": error_msg}
    except Exception as e:
        error_msg = f"Failed to capture snapshot: {str(e)}"
        logger.error(error_msg)
        return {"status": "error", "message": error_msg}
