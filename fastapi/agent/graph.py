"""
agent/graph.py
==============
Definisce il grafo LangGraph con il corretto ordine di esecuzione:
Decide Tools -> Fetch Fisica -> Query Neo4j.
"""

import logging
from functools import partial

from langgraph.graph import StateGraph, END
from langgraph.checkpoint.memory import MemorySaver

from agent.state import BaldoState
from agent.nodes import (
    analizza_prompt,
    valuta_prompt,
    decide_tools,
    query_neo4j,
    fetch_contesto_fisico,
    valuta_frammenti,
    componi_prompt,
    genera_draft,
    valuta_draft,
    correggi_draft,
    rifinisci,
    salva_memoria,
    nodo_errore,
)
from services.llm import LLMRouter
from services.neo4j_client import Neo4jClient
from services.astronomy import AstronomyClient
from services.weather import WeatherClient
from services.geo_service import GeoService

logger = logging.getLogger(__name__)

MAX_ITERAZIONI = 8
MAX_RETRY_NEO4J = 3
MAX_RETRY_CORREZIONE = 2

def build_graph(
    llm: LLMRouter,
    neo4j: Neo4jClient,
    astronomy: AstronomyClient,
    weather: WeatherClient,
    geo: GeoService
) -> StateGraph:
    
    # -- Wrap nodi con dipendenze iniettate --
    _analizza = partial(analizza_prompt, llm=llm)
    _valuta_prompt = partial(valuta_prompt, llm=llm)
    _decide_tools = partial(decide_tools, llm=llm)
    _query_neo4j = partial(query_neo4j, neo4j=neo4j)
    _fetch_fisica = partial(fetch_contesto_fisico, astronomy=astronomy, weather=weather, geo=geo)
    _valuta_frammenti = partial(valuta_frammenti, llm=llm)
    _genera_draft = partial(genera_draft, llm=llm)
    _valuta_draft = partial(valuta_draft, llm=llm)
    _correggi_draft = partial(correggi_draft, llm=llm)
    _rifinisci = partial(rifinisci, llm=llm)
    _salva_memoria = partial(salva_memoria, neo4j=neo4j)

    grafo = StateGraph(BaldoState)

    # Aggiunta nodi
    grafo.add_node("analizza_prompt", _analizza)
    grafo.add_node("valuta_prompt", _valuta_prompt)
    grafo.add_node("decide_tools", _decide_tools)
    grafo.add_node("fetch_contesto_fisico", _fetch_fisica)
    grafo.add_node("query_neo4j", _query_neo4j)
    grafo.add_node("valuta_frammenti", _valuta_frammenti)
    grafo.add_node("componi_prompt", componi_prompt)
    grafo.add_node("genera_draft", _genera_draft)
    grafo.add_node("valuta_draft", _valuta_draft)
    grafo.add_node("correggi_draft", _correggi_draft)
    grafo.add_node("rifinisci", _rifinisci)
    grafo.add_node("salva_memoria", _salva_memoria)
    grafo.add_node("nodo_errore", nodo_errore)

    # -- FLUSSO DI ESECUZIONE --
    
    # Entry point
    grafo.set_entry_point("analizza_prompt")
    grafo.add_edge("analizza_prompt", "valuta_prompt")

    # Routing iniziale
    grafo.add_conditional_edges(
        "valuta_prompt",
        _route_valuta_prompt,
        {"decide_tools": "decide_tools", "nodo_errore": "nodo_errore"}
    )

    # DOPO DECIDE_TOOLS -> VAI SEMPRE AL CONTESTO FISICO
    # (Così carichiamo Meteo/Astro prima di cercare nel DB)
    grafo.add_edge("decide_tools", "fetch_contesto_fisico")

    # DOPO FISICA -> VAI A CERCARE NEL DB
    grafo.add_edge("fetch_contesto_fisico", "query_neo4j")

    # DOPO NEO4J -> VALUTA I FRAMMENTI
    grafo.add_edge("query_neo4j", "valuta_frammenti")

    # LOOP DI RICERCA (Se non trova nulla in Neo4j, torna a query_neo4j)
    grafo.add_conditional_edges(
        "valuta_frammenti",
        _route_valuta_frammenti,
        {"query_neo4j": "query_neo4j", "componi_prompt": "componi_prompt"}
    )

    # GENERAZIONE NARRATIVA
    grafo.add_edge("componi_prompt", "genera_draft")
    grafo.add_edge("genera_draft", "valuta_draft")

    # LOOP DI CORREZIONE DRAFT
    grafo.add_conditional_edges(
        "valuta_draft",
        _route_valuta_draft,
        {"correggi_draft": "correggi_draft", "rifinisci": "rifinisci"}
    )

    grafo.add_edge("correggi_draft", "valuta_draft")
    grafo.add_edge("rifinisci", "salva_memoria")
    grafo.add_edge("salva_memoria", END)
    grafo.add_edge("nodo_errore", END)

    return grafo.compile(checkpointer=MemorySaver())

# --- Router Functions ---

def _route_valuta_prompt(state: BaldoState) -> str:
    if not state.get("prompt_chiaro", True): return "nodo_errore"
    return "decide_tools"

def _route_valuta_frammenti(state: BaldoState) -> str:
    # Torna a query_neo4j solo se mancano frammenti e abbiamo ancora tentativi
    if state.get("qualita_frammenti") == "assente" and state.get("tentativi_neo4j", 0) < MAX_RETRY_NEO4J:
        return "query_neo4j"
    return "componi_prompt"

def _route_valuta_draft(state: BaldoState) -> str:
    if state.get("valutazione_draft") != "ok" and state.get("tentativi_correzione", 0) < MAX_RETRY_CORREZIONE:
        return "correggi_draft"
    return "rifinisci"
