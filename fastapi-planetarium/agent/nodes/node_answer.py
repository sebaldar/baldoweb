"""
nodes/node_answer.py
Sintetizza RAG, meteo e dati del motore astronomico.
"""
from __future__ import annotations
import logging
import json
import math
from datetime import datetime

from langchain_core.messages import SystemMessage, HumanMessage

from agent.state import PlanetariumState
from agent.emitter import SseEmitter, emit_thinking, emit_token, emit_final, emit_error
from services.config import get_llm

logger = logging.getLogger(__name__)

MAX_SUMMARY_CHARS = 600

ANSWER_SYSTEM = """Sei l'assistente esclusivo di questo planetario 3D. Rispondi in italiano in modo discorsivo, conciso (3-5 frasi) e appassionante.

REGOLE DI RISPOSTA:
1. Usa SEMPRE i "DATI IN TEMPO REALE DAL MOTORE ASTRONOMICO".
2. OGGETTI SOTTO L'ORIZZONTE: Se l'altezza (Alt) è negativa, l'oggetto è sotto i piedi dell'osservatore ed è INVISIBILE.
3. EFFETTO GIORNO: Se ti viene indicato che un oggetto è "INVISIBILE per la luce del giorno", significa che il Sole è sorto. Spiega all'utente che l'oggetto è fisicamente sopra l'orizzonte, ma il cielo azzurro diurno ne impedisce totalmente l'osservazione.
4. L'orario indicato è in UT. Adattalo all'ora locale italiana.
5. Usa SOLO la rosa dei venti (es. "verso Sud-Ovest"). Evita di nominare la parola "Azimut".
"""

def node_answer(state: PlanetariumState, emitter: SseEmitter) -> dict:
    emit_thinking(emitter, "Compongo la risposta…")

    context_parts = []

    provider        = state.get("provider", "ionos")
    prompt          = state.get("prompt", "")
    data            = state.get("data")
    lat             = state.get("lat")
    lon             = state.get("lon")
    rag_docs        = state.get("rag_docs", [])
    weather_data    = state.get("weather_data")
    cmds_planetario = state.get("cmds_planetario", [])
    extra           = state.get("extra", {})

    # ── Lettura Dati Motore Astronomico ──────────────────────────────
    dati_motore_raw = state.get("motore_astronomico")

    if data:
        context_parts.append(f"Data simulazione richiesta: {data}")
    if lat and lon:
        context_parts.append(f"Posizione osservatore: lat={lat:.2f}°, lon={lon:.2f}°")

    # 1. Documenti RAG (Messi per primi così non rubano l'attenzione)
    if rag_docs:
        context_parts.append("\n### Fonti dalla knowledge base:")
        for i, doc in enumerate(rag_docs, 1):
            nome = doc.get("nome_italiano") or doc.get("nome", "")
            tipo = doc.get("tipo", "")
            summary = (doc.get("wikipedia_summary") or doc.get("text") or "")[:MAX_SUMMARY_CHARS]

            info_tecnica = ""
            if tipo == "STARS": info_tecnica = _star_extra(doc)
            elif tipo == "SOLARSYSTEM": info_tecnica = _solarsystem_extra(doc)
            elif tipo == "CONSTELLATIONS": info_tecnica = _constellation_extra(doc)

            context_parts.append(f"{i}. **{nome}** ({tipo})\n{summary}\n{info_tecnica}".strip())

    # 2. Meteo
    if weather_data:
        w = weather_data
        context_parts.append(
            f"\n### Condizioni meteo attuali:\nCopertura: {w['cloud_cover']}% | "
            f"Visibilità: {w['visibility_m']/1000:.1f} km"
        )

    # 3. Comandi eseguiti
    if cmds_planetario:
        labels = [c.get("label", c.get("cmd", "")) for c in cmds_planetario]
        context_parts.append(f"\n### Azioni eseguite:\n" + "\n".join(f"- {l}" for l in labels))

    # 4. DATI MOTORE ASTRONOMICO e CALCOLO STELLE FISSE
    jd_attuale = None
    sun_alt = _get_sun_altitude(dati_motore_raw) # <--- Ricaviamo subito il Sole!

    if dati_motore_raw:
        testo_motore = _formatta_dati_motore(dati_motore_raw, sun_alt) # <--- Passiamo il sole
        if testo_motore:
            context_parts.append(testo_motore)

        if isinstance(dati_motore_raw, str):
            try: jd_attuale = json.loads(dati_motore_raw).get("julian_day")
            except: pass
        elif isinstance(dati_motore_raw, dict):
            jd_attuale = dati_motore_raw.get("julian_day")

    # 5. IL TRUCCO MAGICO: Calcolo Alt/Az per Stelle e Costellazioni
    target_ra = extra.get("ra")
    target_dec = extra.get("dec")
    target_name = extra.get("target")

    if not target_ra or not target_dec:
        if rag_docs:
            main_doc = rag_docs[0]
            target_ra = main_doc.get("ra") or main_doc.get("ascensione_retta") or main_doc.get("RA")
            target_dec = main_doc.get("dec") or main_doc.get("declinazione") or main_doc.get("DEC")
            if not target_name:
                target_name = main_doc.get("nome_italiano") or main_doc.get("nome")

    if not target_name:
        target_name = extra.get("category") or "L'oggetto richiesto"

    if target_ra is not None and target_dec is not None and jd_attuale and lat is not None and lon is not None:
        try:
            alt, az = _calcola_alt_az(float(target_ra), float(target_dec), lat, lon, jd_attuale)
            direzione = _azimut_to_cardinale(az)

            # APPLICHIAMO LA FISICA DEL GIORNO ANCHE ALLE STELLE CALCOLATE:
            if alt < 0:
                visibile = "NON VISIBILE (sotto l'orizzonte)"
            elif sun_alt > 0:
                visibile = "NON VISIBILE (oscurato dalla luce del giorno)"
            else:
                visibile = "VISIBILE in cielo"

            info_calcolata = f"\n### POSIZIONE ESATTA CALCOLATA PER {str(target_name).upper()}\n"
            info_calcolata += f"- Altezza: {alt:.1f} gradi ({visibile})\n"
            info_calcolata += f"- Direzione: verso {direzione}\n"

            context_parts.append(info_calcolata)
        except Exception as e:
            logger.error(f"[CALCOLO ALTAZ] Errore: {e}")



    context = "\n\n".join(context_parts)

    # Prompt strutturato per forzare i modelli più piccoli a leggere i dati freschi
    user_prompt = f"Contesto disponibile:\n{context}\n\nDomanda dell'utente: {prompt}\n\nRispondi in italiano in modo naturale basandoti SOLO SUI DATI APPENA FORNITI."

    # ── Streaming ──────────────────────────────────────────────────
    llm = get_llm(provider, streaming=True)
    full_text = ""
    sources = _build_sources(rag_docs)

    try:
        for chunk in llm.stream([
            SystemMessage(content=ANSWER_SYSTEM),
            HumanMessage(content=user_prompt),
        ]):
            if chunk.content:
                emit_token(emitter, chunk.content)
                full_text += chunk.content

    except Exception as e:
        logger.error(f"[ANSWER] Errore: {e}")
        emit_error(emitter, "Errore nella generazione.")
        return {"answer_text": "", "extra": extra}

    # Invio finale con l'extra!
    emit_final(emitter, text=full_text, sources=sources, extra=extra)
    logger.info(f"[ANSWER] Finito. Extra: {extra}")

    # Restituisce l'extra allo state globale
    return {
        "answer_text": full_text,
        "extra": extra
    }

# ── Helpers contestuali ────────────────────────────────────────────────────────

def _get_sun_altitude(dati_raw) -> float:
    """Cerca il Sole nei dati del motore e restituisce la sua altezza."""
    if isinstance(dati_raw, str):
        try: dati_raw = json.loads(dati_raw)
        except: return -90.0
    if isinstance(dati_raw, dict):
        for c in dati_raw.get('bodies', []):
            if c.get('name', '').lower() in ['sole', 'sun']:
                return c.get('altitude_deg', -90.0)
    return -90.0

def _formatta_dati_motore(dati_raw, sun_alt: float) -> str:
    """Parsea in sicurezza il JSON e crea un log leggero per l'LLM."""
    if not dati_raw: return ""
    if isinstance(dati_raw, str):
        try: dati_raw = json.loads(dati_raw)
        except: return ""

    if not isinstance(dati_raw, dict) or "bodies" not in dati_raw: return ""

    testo = f"\n### DATI IN TEMPO REALE DAL MOTORE ASTRONOMICO\n"
    testo += f"Data/Ora: {dati_raw.get('date', 'N/A')} UT (Nota: calcola ora locale aggiungendo +1 o +2 ore).\n"
    testo += "POSIZIONE CORPI CELESTI:\n"

    for corpo in dati_raw.get('bodies', []):
        nome = corpo.get('name', '').capitalize()
        alt = corpo.get('altitude_deg', 0)
        az = corpo.get('azimuth_deg', 0)
        costellazione = corpo.get('constellation', 'N/A')
        direzione = _azimut_to_cardinale(az)

        if alt < 0:
            testo += f"- {nome}: Altezza {alt:.1f}° (sotto l'orizzonte, INVISIBILE), direzione {direzione}.\n"
        # LA NUOVA REGOLA DEL GIORNO (Solo Sole, Luna e Venere possono essere intravisti di giorno)
        elif sun_alt > 0 and nome.lower() not in ['sole', 'sun', 'luna', 'moon', 'venere', 'venus']:
            testo += f"- {nome}: Altezza {alt:.1f}° (sopra l'orizzonte ma INVISIBILE per la luce del giorno), direzione {direzione}.\n"
        else:
            testo += f"- {nome}: Altezza {alt:.1f}° (VISIBILE in cielo), direzione {direzione}.\n"

    return testo


def _star_extra(doc: dict) -> str:
    parts = []
    if doc.get("magnitudine_v") is not None and doc["magnitudine_v"] < 99: parts.append(f"Mag: {doc['magnitudine_v']:.2f}")
    if doc.get("distanza_ly"): parts.append(f"Dist: {doc['distanza_ly']:.1f} a.l.")
    return " | ".join(parts)

def _solarsystem_extra(doc: dict) -> str:
    if doc.get("visibilita_occhio_nudo"): return "Visibile a occhio nudo"
    return ""

def _constellation_extra(doc: dict) -> str:
    if doc.get("mese_osservazione"): return f"Mese migliore: {doc['mese_osservazione']}"
    return ""

def _build_sources(docs: list[dict]) -> list[dict]:
    return [{"nome": d.get("nome_italiano", d.get("nome", "")), "url": d["wikipedia_url"]} for d in docs if d.get("wikipedia_url")]

def _azimut_to_cardinale(az_deg: float) -> str:
    """Converte l'azimut in gradi (0-360) in un punto cardinale."""
    val = az_deg % 360
    if val < 22.5 or val >= 337.5: return "Nord"
    elif val < 67.5: return "Nord-Est"
    elif val < 112.5: return "Est"
    elif val < 157.5: return "Sud-Est"
    elif val < 202.5: return "Sud"
    elif val < 247.5: return "Sud-Ovest"
    elif val < 292.5: return "Ovest"
    else: return "Nord-Ovest"

def _calcola_alt_az(ra_deg: float, dec_deg: float, lat: float, lon: float, julian_day: float) -> tuple[float, float]:
    """
    Converte Ascensione Retta e Declinazione in Altezza e Azimut locali
    utilizzando il Giorno Giuliano e le coordinate dell'osservatore.
    """
    # 1. Calcolo del Tempo Siderale a Greenwich (GMST) in gradi
    d = julian_day - 2451545.0
    gmst = (280.46061837 + 360.98564736629 * d) % 360.0

    # 2. Tempo Siderale Locale (LST)
    lst = (gmst + lon) % 360.0

    # 3. Angolo orario (HA)
    ha = (lst - ra_deg) % 360.0

    # Conversioni in radianti per la trigonometria
    ha_rad = math.radians(ha)
    dec_rad = math.radians(dec_deg)
    lat_rad = math.radians(lat)

    # 4. Altezza
    sin_alt = math.sin(dec_rad) * math.sin(lat_rad) + math.cos(dec_rad) * math.cos(lat_rad) * math.cos(ha_rad)
    alt_rad = math.asin(sin_alt)
    alt_deg = math.degrees(alt_rad)

    # 5. Azimut
    cos_az = (math.sin(dec_rad) - math.sin(lat_rad) * math.sin(alt_rad)) / (math.cos(lat_rad) * math.cos(alt_rad))
    # Clamp per evitare errori di precisione floating point (es. 1.0000000002)
    cos_az = max(-1.0, min(1.0, cos_az))
    az_rad = math.acos(cos_az)
    az_deg = math.degrees(az_rad)

    if math.sin(ha_rad) > 0:
        az_deg = 360.0 - az_deg

    return alt_deg, az_deg
