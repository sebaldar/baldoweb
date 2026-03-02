"""
agent/nodes.py
==============
Tutti i nodi del grafo LangGraph. 
Gestisce l'analisi, il recupero di dati fisici (Geo/Meteo/Astro),
la memoria Neo4j e la composizione narrativa.
"""

import json
import logging
import uuid
import asyncio
from datetime import datetime

from agent.state import BaldoState
from services.llm import LLMRouter
from services.rag import RAGExtractor
from services.neo4j_client import Neo4jClient
from services.astronomy import AstronomyClient
from services.weather import WeatherClient
from services.geo_service import GeoService
from services.composer import StoryComposer

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# NODE 1 — Analisi del prompt
# ---------------------------------------------------------------------------
async def analizza_prompt(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] analizza_prompt")
    rag = RAGExtractor(llm=llm)
    
    # Estrazione entità: personaggi, emozioni, ambientazione, luogo, data
    analisi = await rag.analizza(state["prompt_originale"])

    # Gestione Fallback Temporale (Data e Ora attuali se non specificate)
    data_target = analisi.get("data_storia") or datetime.now().strftime("%d-%m-%Y")
    ora_target = analisi.get("ora_storia") or datetime.now().strftime("%H:%M:%S")
    
    # Gestione Fallback Geografico (Default: Roma)
    luogo_target = analisi.get("luogo") or "Roma"

    termini = analisi.get("personaggi", []) + analisi.get("emozioni", [])

    return {
        "personaggi": analisi.get("personaggi", []),
        "emozioni": analisi.get("emozioni", []),
        "ambientazione": analisi.get("ambientazione", ""),
        "luogo": luogo_target,
        "data_storia": data_target,
        "ora_storia": ora_target,
        "termini_ricerca_neo4j": termini,
        "tentativi_neo4j": 0,
        "tentativi_correzione": 0,
        "iterazioni_totali": 1,
    }

# ---------------------------------------------------------------------------
# NODE 2 — Valutazione prompt
# ---------------------------------------------------------------------------
async def valuta_prompt(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_prompt")
    system = """Sei un supervisore di storie per bambini. Valuta se il prompt è adeguato.
    Rispondi SOLO JSON: {"chiaro": true/false, "motivo": "..."}"""
    
    risposta = await llm.chiedi(
        system=system,
        user=f"Prompt: {state['prompt_originale']}\nPersonaggi: {state['personaggi']}"
    )
    try:
        dati = json.loads(risposta.strip().strip("```json").strip("```"))
        return {"prompt_chiaro": dati.get("chiaro", True), "motivo_rifiuto": dati.get("motivo")}
    except Exception:
        return {"prompt_chiaro": True, "motivo_rifiuto": None}

# ---------------------------------------------------------------------------
# NODE 3 — Decisione tool
# ---------------------------------------------------------------------------
async def decide_tools(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] decide_tools")
    system = 'Decidi se usare l\'astronomia. Rispondi SOLO JSON: {"usa_astronomia": true/false}'
    risposta = await llm.chiedi(system=system, user=state['prompt_originale'])
    try:
        dati = json.loads(risposta.strip().strip("```json").strip("```"))
        return {"usa_astronomia": dati.get("usa_astronomia", True)}
    except:
        return {"usa_astronomia": True}

# ---------------------------------------------------------------------------
# NODE 4a — Query Neo4j
# ---------------------------------------------------------------------------
async def query_neo4j(state: BaldoState, neo4j: Neo4jClient) -> dict:
    logger.info(f"[NODE] query_neo4j (tentativo {state['tentativi_neo4j'] + 1})")

    frammenti = await neo4j.cerca_frammenti(
        characters=state["personaggi"],
        emotions=state["emozioni"],
        setting=state["ambientazione"],
        extra_terms=state.get("termini_ricerca_neo4j", []),
    )

    storie_precedenti = await neo4j.cerca_storie_precedenti(
        characters=state["personaggi"],
        emotions=state["emozioni"],
    )

    return {
        "frammenti": frammenti,
        "storie_precedenti": storie_precedenti,
        "tentativi_neo4j": state["tentativi_neo4j"] + 1,
    }

# ---------------------------------------------------------------------------
# NODE 4b — Recupero dati fisici (Geo + Meteo + Astro) - CORRETTO
# ---------------------------------------------------------------------------
async def fetch_contesto_fisico(
    state: BaldoState, 
    astronomy: AstronomyClient, 
    weather: WeatherClient,
    geo: GeoService
) -> dict:
    luogo_nome = state.get("luogo") or "Roma"
    logger.info(f"[NODE] fetch_contesto_fisico avviato per: {luogo_nome}")

    lat, lon = 41.8928, 12.4964  # Default Roma
    meteo_res, astro_res = "cielo sereno", {}

    try:
        # 1. Geocoding (Verifica il nome del metodo nel tuo GeoService!)
        # Se il tuo servizio usa get_coords invece di get_coordinates, correggi qui:
        try:
            coords = await geo.get_coordinates(luogo_nome)
            if coords:
                lat, lon = coords
                logger.info(f"📍 Coordinate ottenute: {lat}, {lon}")
        except Exception as geoe:
            logger.warning(f"Geocoding fallito per {luogo_nome}, uso Roma. Errore: {geoe}")

        # 2. Esecuzione parallela
        # Creiamo i task solo se usa_astronomia è True
        tasks = [weather.get_weather(lat, lon)]
        
        usa_astro = state.get("usa_astronomia", True)
        if usa_astro:
            logger.info(f"🔭 Preparazione chiamata Astronomy per {luogo_nome}...")
            tasks.append(astronomy.get_sky_data(
                client_ip=state.get("client_ip", "127.0.0.1"),
                lat=lat,
                lon=lon,
                data_storia=state.get("data_storia"),
                ora_storia=state.get("ora_storia")
            ))

        # Attesa risultati
        results = await asyncio.gather(*tasks, return_exceptions=True)
        
        meteo_res = results[0] if not isinstance(results[0], Exception) else "meteo non disponibile"
        if usa_astro and len(results) > 1:
            astro_res = results[1] if not isinstance(results[1], Exception) else {}
            if isinstance(results[1], Exception):
                logger.error(f"Errore specifico AstronomyClient: {results[1]}")
        
        logger.info(f"✅ Dati recuperati: Meteo={meteo_res}, Astro={'Sì' if astro_res else 'No'}")

    except Exception as e:
        logger.error(f"❌ Errore generale nel nodo fisico: {e}", exc_info=True)

    return {
        "lat": lat,
        "lon": lon,
        "dati_meteo": meteo_res,
        "dati_astronomici": astro_res
    }
# ---------------------------------------------------------------------------
# NODE 5 — Valutazione qualità frammenti
# ---------------------------------------------------------------------------
async def valuta_frammenti(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_frammenti")
    if not state["frammenti"]:
        nuovi_termini = await _riformula_termini(state, llm)
        return {"qualita_frammenti": "assente", "termini_ricerca_neo4j": nuovi_termini}
    return {"qualita_frammenti": "buona"}

async def _riformula_termini(state: BaldoState, llm: LLMRouter) -> list[str]:
    system = 'Suggerisci termini di ricerca alternativi in JSON: {"termini": []}'
    risposta = await llm.chiedi(system=system, user=state['ambientazione'])
    try:
        dati = json.loads(risposta.strip().strip("```json").strip("```"))
        return dati.get("termini", [])
    except:
        return []

# ---------------------------------------------------------------------------
# NODE 6 — Composizione prompt
# ---------------------------------------------------------------------------
async def componi_prompt(state: BaldoState) -> dict:
    logger.info("[NODE] componi_prompt")
    composer = StoryComposer()
    
    prompt_arricchito = composer.componi(
        prompt_originale=state["prompt_originale"],
        analisi={
            "personaggi": state["personaggi"],
            "emozioni": state["emozioni"],
            "ambientazione": state["ambientazione"],
            "luogo": state.get("luogo"),
            "data": state.get("data_storia"),
            "meteo": state.get("dati_meteo")
        },
        frammenti=state["frammenti"],
        dati_astronomici=state.get("dati_astronomici") if state.get("usa_astronomia") else None,
        storie_precedenti=state.get("storie_precedenti", []),
        eta=state["eta_bambino"],
        lunghezza=state["lunghezza"],
        lingua=state["lingua"],
    )
    return {"prompt_arricchito": prompt_arricchito}

# ---------------------------------------------------------------------------
# NODE 7-10 — Generazione e Rifinitura (Draft, Correzione, Rifinitura)
# ---------------------------------------------------------------------------
async def genera_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] genera_draft")
    prompt_input = state.get("prompt_arricchito") or state["prompt_originale"]
    draft = await llm.genera_racconto(prompt_input)
    return {"draft": draft}

async def valuta_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_draft")
    return {"valutazione_draft": "ok"}

async def correggi_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] correggi_draft")
    return {"tentativi_correzione": state["tentativi_correzione"] + 1}

async def rifinisci(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] rifinisci")
    racconto = await llm.rifinisci(draft=state["draft"], eta=state["eta_bambino"])
    return {"racconto_finale": racconto}

# ---------------------------------------------------------------------------
# NODE 11 — Salvataggio memoria
# ---------------------------------------------------------------------------
async def salva_memoria(state: BaldoState, neo4j: Neo4jClient) -> dict:
    logger.info("[NODE] salva_memoria")
    storia_id = str(uuid.uuid4())
    ids_frammenti = [f["id"] for f in state.get("frammenti", []) if f.get("id")]

    await neo4j.salva_storia(
        story_id=storia_id,
        text=state["racconto_finale"],
        characters=state["personaggi"],
        emotions=state["emozioni"],
        setting=state["ambientazione"],
        original_prompt=state["prompt_originale"],
        used_fragments=ids_frammenti,
        timestamp=datetime.utcnow().isoformat(),
    )
    return {"storia_id": storia_id, "frammenti_usati": ids_frammenti}

# ---------------------------------------------------------------------------
# NODE — Errore
# ---------------------------------------------------------------------------
async def nodo_errore(state: BaldoState) -> dict:
    return {"racconto_finale": "", "errore": state.get("motivo_rifiuto")}
