import httpx
import logging
from config import settings

logger = logging.getLogger(__name__)

class WeatherClient:
    def __init__(self, http_client: httpx.AsyncClient):
        self.http = http_client
        self.api_key = settings.OPENWEATHER_API_KEY
        self.base_url = "https://api.openweathermap.org/data/2.5/weather"

    async def get_weather(self, lat: float, lon: float) -> str:
        """Recupera le condizioni meteo attuali per le coordinate fornite."""
        try:
            params = {
                "lat": lat,
                "lon": lon,
                "appid": self.api_key,
                "lang": "it",
                "units": "metric"
            }
            response = await self.http.get(self.base_url, params=params, timeout=5.0)
            response.raise_for_status()
            data = response.json()
            
            condizione = data["weather"][0]["description"]
            temp = data["main"]["temp"]
            
            logger.info(f"Meteo recuperato: {condizione}, {temp}°C")
            return f"{condizione}, {temp}°C"
        except Exception as e:
            logger.error(f"Errore recupero meteo: {e}")
            return "cielo sereno" # Fallback poetico
