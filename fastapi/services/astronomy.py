"""
AstronomyClient
===============
Interfaccia verso il server Node.js che espone il motore astronomico C++.
Supporta coordinate geografiche e timestamp storici.
"""

import logging
import httpx
from datetime import datetime
from zoneinfo import ZoneInfo
from config import settings

logger = logging.getLogger(__name__)

FUSO_UTENZA = ZoneInfo("Europe/Rome")
UTC = ZoneInfo("UTC")

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
        ora_storia=None,  # <--- Aggiunto parametro ora
        con_telescopio=False,  # Urano/Nettuno: visibili solo se la storia prevede un telescopio
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

            # data_storia/ora_storia sono in ora locale Europe/Rome (l'utenza
            # dell'app), ma il motore C++ tratta l'input come già UTC senza
            # convertirlo (verificato nel sorgente: WSsrv.cpp lo interpreta
            # come UTC a meno di non passargli esplicitamente il flag "DT",
            # cosa che il servizio Node non fa). Va convertito qui: altrimenti
            # il cielo calcolato è sfasato di 1-2 ore (CET/CEST) rispetto a
            # quello reale visto dall'utente.
            if data_storia and ora_storia:
                try:
                    locale_dt = datetime.strptime(
                        f"{data_storia} {ora_storia}", "%d-%m-%Y %H:%M:%S"
                    ).replace(tzinfo=FUSO_UTENZA)
                    utc_dt = locale_dt.astimezone(UTC)
                    data_storia = utc_dt.strftime("%d-%m-%Y")
                    ora_storia = utc_dt.strftime("%H:%M:%S")
                except ValueError as e:
                    logger.warning(
                        f"Formato data/ora inatteso ({data_storia} {ora_storia}), "
                        f"invio senza conversione UTC: {e}"
                    )

            if data_storia:
                params["data"] = data_storia

            if ora_storia:
                params["ora"] = ora_storia # <--- Passiamo l'ora (UTC) al server Node

            if con_telescopio:
                params["telescopio"] = "true"

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
                    # NON è l'ora locale: il motore si limita a echeggiare
                    # indietro l'istante che gli abbiamo inviato, che qui è
                    # già stato convertito in UTC (vedi sopra) — chiamarlo
                    # "ora_locale" ha già generato un falso allarme di un
                    # apparente sfasamento di 2 ore che non esiste (verificato:
                    # le altezze sono corrette per l'istante UTC reale).
                    "ora_utc_motore": info.get("data_osservazione"), # Formattato da Node
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
            "ora_utc_motore": "ora non pervenuta",
            "status": "fallback"
        }
