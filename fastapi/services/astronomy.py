"""
AstronomyClient
===============
Interfaccia verso il server Node.js che espone il motore astronomico C++.
Supporta coordinate geografiche e timestamp storici.
"""

import logging
import httpx
from config import settings

logger = logging.getLogger(__name__)

class AstronomyClient:
    def __init__(self, http_client: httpx.AsyncClient):
        self.http = http_client
        self.base_url = settings.ASTRONOMY_API_URL

    async def get_sky_data(
        self, 
        client_ip: str, 
        lat=None, 
        lon=None, 
        data_storia=None, 
        ora_storia=None  # <--- Aggiunto parametro ora
    ) -> dict:
        """
        Recupera i dati celesti chiamando il microservizio Node.js.
        Invia IP (per fallback), coordinate reali (se disponibili) e tempo target.
        """
        try:
            # Prepariamo i parametri della query string
            params = {"ip": client_ip}
            
            if lat is not None and lon is not None:
                params.update({"lat": lat, "lon": lon})
            
            if data_storia:
                params["data"] = data_storia
            
            if ora_storia:
                params["ora"] = ora_storia # <--- Passiamo l'ora al server Node

            logger.info(f"Chiamata Astronomy API: {params}")

            response = await self.http.get(self.base_url, params=params, timeout=10.0)
            response.raise_for_status()
            
            data = response.json()
            
            # Validazione struttura risposta del server Node.js
            if data.get("status") == "success" and "info_cielo" in data:
                info = data["info_cielo"]
                oggetti = info.get("oggetti_visibili", [])
                
                visibili = [o.get("nome") for o in oggetti]
                logger.info(f"Cielo reale ricevuto: {len(oggetti)} corpi visibili ({', '.join(visibili)})")
                
                # Restituiamo i dati normalizzati per il StoryComposer
                return {
                    "corpi": oggetti,
                    "fase_giorno": info.get("fase_giorno"),
                    "fase_luna": info.get("fase_luna"), 
                    "ora_locale": info.get("data_osservazione"), # Formattato da Node
                    "status": "reale"
                } 
            
            logger.warning("Risposta Node.js non valida o status != success. Uso fallback.")
            return self._cielo_fallback()

        except Exception as e:
            logger.error(f"Errore comunicazione con motore astronomico: {e}")
            return self._cielo_fallback()

    def _cielo_fallback(self):
        """Restituisce un cielo generico se il servizio esterno è offline."""
        return {
            "corpi": [
                {
                    "nome": "Luna", 
                    "posizione_testuale": "alta nel cielo", 
                    "costellazione": "Sconosciuta",
                    "stelle_vicine": []
                }
            ],
            "fase_giorno": "notte",
            "fase_luna": "crescente",
            "ora_locale": "ora non pervenuta",
            "status": "fallback"
        }
