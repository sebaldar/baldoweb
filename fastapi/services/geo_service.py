"""
services/geo_service.py
=======================
Risolve un nome di luogo in coordinate (lat, lon).

Priorità:
  1. Geocoding del luogo estratto dal prompt (se significativo)
  2. Coordinate inviate dal frontend (GPS o IP del dispositivo)
  3. Default Roma (41.89, 12.49)
"""

import logging
from typing import Optional, Tuple

from geopy.geocoders import Nominatim
from geopy.exc import GeocoderTimedOut, GeocoderServiceError

logger = logging.getLogger(__name__)

# Luoghi generici/fantastici che non ha senso geocodificare
_LUOGHI_IGNORATI = {
    "un luogo magico", "luogo magico", "ignoto", "sconosciuto",
    "un posto lontano", "il regno magico", "un castello", "un bosco",
    "un bosco magico", "la foresta", "una foresta", "la montagna",
    "un villaggio", "fantasy", "immaginario",
}

# Coordinate di default (Roma)
_DEFAULT_LAT = 41.8928
_DEFAULT_LON = 12.4964


class GeoService:
    def __init__(self, user_agent: str = "baldo_story_agent"):
        self.geolocator = Nominatim(user_agent=user_agent)

    async def get_coordinates(
        self,
        luogo_testo: str,
        client_lat: Optional[float] = None,
        client_lon: Optional[float] = None,
    ) -> Tuple[float, float]:
        """
        Restituisce (lat, lon) seguendo questa priorità:

        1. Geocoding del luogo estratto dal prompt
           (se il testo è significativo e Nominatim lo riconosce)
        2. Coordinate del dispositivo/IP ricevute dal frontend
           (client_lat / client_lon)
        3. Default Roma

        Args:
            luogo_testo:  nome del luogo estratto dall'analisi del prompt
            client_lat:   latitudine inviata dal frontend (opzionale)
            client_lon:   longitudine inviata dal frontend (opzionale)
        """

        # ── Step 1: prova il geocoding del luogo nel prompt ──────────────────
        if luogo_testo and luogo_testo.lower().strip() not in _LUOGHI_IGNORATI:
            try:
                location = self.geolocator.geocode(luogo_testo, timeout=5)
                if location:
                    logger.info(
                        f"📍 Geocoding '{luogo_testo}' → "
                        f"{location.latitude:.4f}, {location.longitude:.4f}"
                    )
                    return location.latitude, location.longitude

                logger.info(
                    f"📍 Nominatim: nessun risultato per '{luogo_testo}', "
                    "provo coordinate client."
                )

            except (GeocoderTimedOut, GeocoderServiceError) as e:
                logger.warning(f"⚠️ Geocoding fallito per '{luogo_testo}': {e}")

        # ── Step 2: usa le coordinate mandate dal frontend ───────────────────
        if client_lat is not None and client_lon is not None:
            logger.info(
                f"🌐 Uso coordinate client (GPS/IP): "
                f"{client_lat:.4f}, {client_lon:.4f}"
            )
            return float(client_lat), float(client_lon)

        # ── Step 3: default Roma ─────────────────────────────────────────────
        logger.info("📌 Nessuna coordinate disponibile, uso Roma come default.")
        return _DEFAULT_LAT, _DEFAULT_LON
