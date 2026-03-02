import logging
from typing import Tuple
from geopy.geocoders import Nominatim
from geopy.exc import GeocoderTimedOut, GeocoderServiceError

logger = logging.getLogger(__name__)

class GeoService:
    def __init__(self, user_agent: str = "baldo_story_agent"):
        # Nominatim richiede un user_agent identificativo
        self.geolocator = Nominatim(user_agent=user_agent)

    async def get_coordinates(self, luogo_testo: str) -> Tuple[float, float]:
        """
        Trasforma un nome di luogo in coordinate (lat, lon).
        Default: Roma (41.89, 12.49) in caso di errore.
        """
        if not luogo_testo or luogo_testo.lower() in ["un luogo magico", "ignoto"]:
            return 41.8928, 12.4964

        try:
            # Eseguiamo la ricerca (Nominatim è sincrono, ma lo gestiamo)
            location = self.geolocator.geocode(luogo_testo, timeout=5)
            
            if location:
                logger.info(f"Geocoding successo: {luogo_testo} -> {location.latitude}, {location.longitude}")
                return location.latitude, location.longitude
            
            logger.warning(f"Luogo non trovato: {luogo_testo}. Uso Roma come default.")
            return 41.8928, 12.4964

        except (GeocoderTimedOut, GeocoderServiceError) as e:
            logger.error(f"Errore Geocoding: {e}")
            return 41.8928, 12.4964
