"""
Baldo - Sistema Agentico per Racconto Astronomico & Meteorologico
==================================================================
Entrypoint FastAPI con supporto a Geocoding, Meteo e Astronomia.
"""

import logging
import json
import httpx
import time
import uuid
from contextlib import asynccontextmanager
from typing import Optional

from fastapi import FastAPI, Request, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import StreamingResponse
from pydantic import BaseModel

from config import settings
from services.llm import LLMRouter
from services.neo4j_client import Neo4jClient
from services.astronomy import AstronomyClient
from services.weather import WeatherClient    # <--- NUOVO
from services.geo_service import GeoService  # <--- NUOVO
from services.report import salva_report_storia

from routers.admin import router as admin_router

from agent.graph import build_graph

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Dipendenze globali
# ---------------------------------------------------------------------------
neo4j_client: Optional[Neo4jClient] = None
http_client:  Optional[httpx.AsyncClient] = None
llm_router:   Optional[LLMRouter] = None
astronomy_client: Optional[AstronomyClient] = None
weather_client:   Optional[WeatherClient] = None # <--- NUOVO
geo_service:      Optional[GeoService] = None    # <--- NUOVO
baldo_graph = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    global neo4j_client, http_client, llm_router, baldo_graph, \
           astronomy_client, weather_client, geo_service

    logger.info("Avvio servizi Baldo...")
    
    # Inizializzazione Client
    llm_router = LLMRouter()
    neo4j_client = Neo4jClient(
        uri=settings.NEO4J_URI,
        user=settings.NEO4J_USER,
        password=settings.NEO4J_PASSWORD,
    )
    
    http_client = httpx.AsyncClient(timeout=120.0)
    
    # Inizializzazione Servizi Esterni
    astronomy_client = AstronomyClient(http_client=http_client)
    weather_client = WeatherClient(http_client=http_client) # <--- NUOVO
    geo_service = GeoService()                             # <--- NUOVO

    # COSTRUZIONE GRAFO (Risolve il TypeError aggiungendo weather e geo)
    baldo_graph = build_graph(
        llm=llm_router,
        neo4j=neo4j_client,
        astronomy=astronomy_client,
        weather=weather_client,
        geo=geo_service
    )

    logger.info("Agente Baldo pronto con supporto Geo/Meteo.")
    yield

    logger.info("Chiusura servizi...")
    await neo4j_client.close()
    await http_client.aclose()

# ---------------------------------------------------------------------------
# App & Middleware
# ---------------------------------------------------------------------------
app = FastAPI(title="Baldo API", version="2.1.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(admin_router)

# ---------------------------------------------------------------------------
# Modelli Pydantic
# ---------------------------------------------------------------------------
class StoryStreamRequest(BaseModel):
    prompt:      str
    lingua:      str = "it"
    eta_bambino: int = 4
    lunghezza:   str = "media"
    session_id:  Optional[str] = None
    lat:         Optional[float] = None
    lon:         Optional[float] = None
    data_storia: Optional[str] = None
    ora_storia:  Optional[str] = None   # <--- Aggiunta ora
    source_geo:  Optional[str] = None   # "device" | "ip"
    # Dati di personalizzazione dal form: oggi il frontend li intreccia già
    # nel testo del prompt, ma servono anche come campi distinti per il
    # report YAML amministrativo di ogni storia.
    nome:              Optional[str] = None
    colore_preferito:  Optional[str] = None
    animale_preferito: Optional[str] = None

class StoryResponse(BaseModel):
    racconto:        str
    personaggi:      list[str]
    emozioni:        list[str]
    ambientazione:   str
    usa_astronomia:  bool
    frammenti_usati: list[str]
    storia_id:       Optional[str]
    iterations:      int
    errore:          Optional[str] = None

# ---------------------------------------------------------------------------
# Endpoint Streaming SSE
# ---------------------------------------------------------------------------
@app.post("/racconto/stream")
async def genera_racconto_stream(request: Request, body: StoryStreamRequest):
    client_ip = request.headers.get("x-forwarded-for", request.client.host)
    logger.info(f"Stream racconto | IP={client_ip} | prompt='{body.prompt[:50]}...'")

    t0 = time.monotonic()

    # Stato iniziale coerente con agent/state.py aggiornato
    stato_iniziale = {
        "prompt_originale": body.prompt,
        "client_ip": client_ip,
        "eta_bambino": body.eta_bambino,
        "lunghezza": body.lunghezza,
        "lingua": body.lingua,
        "lat": body.lat,
        "lon": body.lon,
        "data_storia": body.data_storia,
        "ora_storia": body.ora_storia,
        "nome": body.nome,
        "colore_preferito": body.colore_preferito,
        "animale_preferito": body.animale_preferito,
        "luogo": "Roma", # Default che verrà sovrascritto dal Nodo 1
        "iterazioni_totali": 0,
        "frammenti": [],
        "storie_precedenti": [],
        "frammenti_usati": [],
        "tentativi_neo4j": 0,
        "tentativi_correzione": 0,
        "usa_astronomia": True,
        "prompt_chiaro": True,
        "dati_meteo": None,
        "dati_astronomici": None,
        "llm_usage": [],
    }
    
    # Sempre un thread_id nuovo, mai body.session_id: quel valore resta lo
    # stesso per l'intera sessione del browser (sessionStorage), quindi
    # riusarlo come thread_id di LangGraph fa sì che MemorySaver applichi il
    # nuovo input come AGGIORNAMENTO del checkpoint precedente invece che
    # come stato pulito. Per i canali con reducer additivo (llm_usage,
    # iterazioni_totali) questo significa non "sovrascrivere con []/1", ma
    # "sommare [] /1 al valore già salvato" — il vecchio valore resta la base
    # e ogni nodo continua ad accumularci sopra. Osservato: la seconda
    # generazione nella stessa sessione portava con sé, intatte, tutte le
    # voci di llm_usage della prima (stessi token al bit, raddoppiando il
    # totale riportato) — nessun'altra parte del codice legge lo stato di
    # un thread_id al di fuori della stessa richiesta che l'ha creato,
    # quindi non c'è continuità da preservare tra una storia e l'altra.
    config = {"configurable": {"thread_id": f"racconto_{uuid.uuid4().hex}"}}

    NODI_INTERNI = {"LangGraph", "", "_route_valuta_prompt", "_route_dopo_neo4j", 
                    "_route_valuta_frammenti", "_route_valuta_draft"}

    async def event_generator():
        try:
            async for event in baldo_graph.astream_events(stato_iniziale, config=config, version="v2"):
                event_type = event.get("event")
                name = event.get("name", "")

                if name in NODI_INTERNI: continue

                if event_type == "on_chain_start":
                    yield f"data: {json.dumps({'tipo': 'nodo_start', 'nodo': name})}\n\n"

                elif event_type == "on_chain_end":
                    output = event.get("data", {}).get("output", {})
                    if isinstance(output, dict):
                        payload = {
                            "tipo": "nodo_end",
                            "nodo": name,
                            "stato": {k: output.get(k) for k in ("iterazioni_totali", "usa_astronomia", "errore") if k in output}
                        }
                        yield f"data: {json.dumps(payload)}\n\n"

                elif event_type == "on_chat_model_stream":
                    chunk = event["data"]["chunk"]
                    token = chunk.content if hasattr(chunk, "content") else ""
                    if token:
                        yield f"data: {json.dumps({'tipo': 'token', 'testo': token})}\n\n"

            # Snapshot Finale
            snapshot = baldo_graph.get_state(config)
            v = snapshot.values if hasattr(snapshot, "values") else {}

            # Report YAML amministrativo — solo se è stata prodotta una
            # storia vera (non su un prompt rifiutato o un errore a metà,
            # dove i dati sarebbero incompleti o fuorvianti).
            if v.get("racconto_finale") and not v.get("errore"):
                salva_report_storia(v, time.monotonic() - t0)

            yield f"data: {json.dumps({
                'tipo': 'fine',
                'racconto': v.get('racconto_finale', ''),
                'personaggi': v.get('personaggi', []),
                'emozioni': v.get('emozioni', []),
                'ambientazione': v.get('ambientazione', ''),
                'usa_astronomia': v.get('usa_astronomia', False),
                'storia_id': v.get('storia_id'),
                'meteo': v.get('dati_meteo') # Inviato al frontend
            })}\n\n"

        except Exception as e:
            logger.error(f"Errore streaming: {e}", exc_info=True)
            yield f"data: {json.dumps({'tipo': 'errore', 'messaggio': str(e)})}\n\n"

    return StreamingResponse(event_generator(), media_type="text/event-stream")

@app.get("/health")
async def health():
    return {"status": "ok", "agente": "Baldo 2.1 (Full Context)"}
