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
            # Il valore va nel prompt di Baldo per un bambino: i gradi Celsius
            # come numero sono un dato da adulto (un bambino di 4 anni non sa
            # cosa siano "25 gradi") — l'LLM li ripeteva alla lettera perché
            # glieli passavamo già formattati così. Restituiamo una sensazione
            # concreta al loro posto, il numero resta solo nel log sopra.
            return f"{condizione}, {self._sensazione_temperatura(temp)}"
        except Exception as e:
            logger.error(f"Errore recupero meteo: {e}")
            return "cielo sereno" # Fallback poetico

    @staticmethod
    def _sensazione_temperatura(temp: float) -> str:
        """Traduce una temperatura in gradi in una sensazione concreta per un bambino."""
        if temp < 5:
            return "un freddo pungente, di quelli da sciarpa e guanti"
        if temp < 12:
            return "un freddo deciso, meglio la giacca pesante"
        if temp < 18:
            return "un'aria frizzante, giusta per una giacca leggera"
        if temp < 24:
            return "una temperatura piacevole, perfetta per giocare fuori"
        if temp < 30:
            # Niente riferimenti al momento della giornata qui ("pomeriggio",
            # "sera"...): questa funzione non sa che ora è davvero nella
            # storia (osservato: un caso con ora_storia=12:09 descritto come
            # "pomeriggio" — un bambino che guarda l'orologio se ne accorge).
            # Il momento della giornata lo gestisce già composer.py altrove.
            return "un caldo gradevole, di quelli in cui si esce senza giacca"
        return "un caldo torrido, di quelli da acqua fresca e ombra"
