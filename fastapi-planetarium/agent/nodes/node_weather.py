"""
nodes/node_weather.py
Recupera le condizioni meteo da OpenMeteo (gratuito, no API key).
Viene ignorato se la data/ora simulata si discosta di oltre 30 min dal tempo reale.
"""

from __future__ import annotations
import logging
import httpx
from datetime import datetime, timedelta, timezone # <-- Aggiunto timezone

from agent.state import PlanetariumState
from agent.emitter import SseEmitter, emit_thinking, emit_weather
from services.config import get_settings

logger = logging.getLogger(__name__)

TIMEOUT = 6.0   # secondi — OpenMeteo è veloce, timeout aggressivo

def _is_time_close_to_now(simulated_data: str) -> bool:
    """Verifica se la data/ora simulata è entro 30 minuti da 'adesso' in UTC."""
    if not simulated_data:
        return True

    try:
        # La data che arriva dal frontend è in formato UT (UTC)
        sim_time = datetime.fromisoformat(simulated_data).replace(tzinfo=None)

        # Recuperiamo l'ora attuale rigorosamente in UTC
        now_utc = datetime.now(timezone.utc).replace(tzinfo=None)

        delta = abs(now_utc - sim_time)
        return delta <= timedelta(minutes=30)
    except ValueError:
        return True

def node_weather(state, emitter: SseEmitter) -> dict:

    # ── 1. Controllo del Viaggio nel Tempo ──
    simulated_data = state.get("data", "")
    if not _is_time_close_to_now(simulated_data):
        logger.info(f"[WEATHER] Data simulata ({simulated_data}) oltre i 30 min. Skip meteo.")
        return {"weather_data": None}

    # ── 2. Controllo delle Coordinate ──
    emit_thinking(emitter, "Controllo le condizioni meteo…")
    lat = state.get("lat")
    lon = state.get("lon")
    if lat is None or lon is None:
        logger.warning("[WEATHER] Coordinate mancanti, skip")
        return {"weather_data": None}

    s   = get_settings()
    url = s.openmeteo_url

    params = {
        "latitude":                lat,
        "longitude":               lon,
        "hourly":                  "cloud_cover,visibility,precipitation_probability",
        # FIX: rimossa "visibility" da current (non supportata da OpenMeteo)
        "current":                 "cloud_cover,temperature_2m,wind_speed_10m",
        "forecast_days":           1,
        "timezone":                "auto",
    }

    try:
        with httpx.Client(timeout=TIMEOUT) as client:
            resp = client.get(url, params=params)
            resp.raise_for_status()
            raw = resp.json()

        current = raw.get("current", {})
        hourly  = raw.get("hourly", {})

        cloud_cover  = current.get("cloud_cover", 100)              # %

        # Visibility e precipitazioni le prendiamo da 'hourly' (indice 0 = ora attuale)
        visibility_list = hourly.get("visibility", [])
        precip_list     = hourly.get("precipitation_probability", [])

        visibility   = visibility_list[0] if visibility_list else 0
        precip_prob  = precip_list[0] if precip_list else 0

        temperature  = current.get("temperature_2m")
        wind_speed   = current.get("wind_speed_10m")
        # Valutazione qualità osservazione
        score, label = _observation_score(cloud_cover, visibility, precip_prob)

        weather = {
            "cloud_cover":   cloud_cover,
            "visibility_m":  visibility,
            "precip_prob":   precip_prob,
            "temperature":   temperature,
            "wind_speed":    wind_speed,
            "obs_score":     score,       # 0–100
            "obs_label":     label,       # "ottima" | "buona" | "discreta" | "scarsa" | "impossibile"
            "ok":            score >= 50,
        }

        emit_weather(emitter, {
            "cloud_cover": cloud_cover,
            "obs_label":   label,
            "label":       f"Meteo: {label} per l'osservazione ({cloud_cover}% nuvoloso)",
        })

        logger.info(f"[WEATHER] cloud={cloud_cover}% vis={visibility}m score={score} → {label}")
        return {"weather_data": weather}

    except httpx.TimeoutException:
        logger.warning("[WEATHER] Timeout OpenMeteo")
        return {"weather_data": None}
    except Exception as e:
        logger.error(f"[WEATHER] Errore: {e}")
        return {"weather_data": None}


def _observation_score(cloud_cover: float, visibility_m: float, precip_prob: float) -> tuple[int, str]:
    """
    Score 0–100 per la qualità dell'osservazione astronomica.
    Pesi: nuvole (60%), visibilità (25%), pioggia (15%)
    """
    # Nuvole: 0% → 100 punti, 100% → 0 punti
    cloud_score = max(0, 100 - cloud_cover)

    # Visibilità: >10km → 100, <1km → 0
    vis_km = visibility_m / 1000
    vis_score = min(100, max(0, (vis_km - 1) / 9 * 100))

    # Pioggia: 0% → 100, 100% → 0
    precip_score = max(0, 100 - precip_prob)

    score = int(cloud_score * 0.60 + vis_score * 0.25 + precip_score * 0.15)

    if score >= 85:
        label = "ottima"
    elif score >= 65:
        label = "buona"
    elif score >= 50:
        label = "discreta"
    elif score >= 30:
        label = "scarsa"
    else:
        label = "impossibile"

    return score, label
