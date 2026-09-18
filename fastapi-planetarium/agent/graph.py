"""
graph.py
Definizione del grafo LangGraph per il planetario.

Flusso:
  START
    └─> node_classify
          ├─> node_emit_cmds   (se ci sono comandi planetario)
          ├─> node_rag         (se intento info o compound)
          └─> node_weather     (se intento weather o compound)
                │
              node_answer
                │
              END

node_emit_cmds, node_rag, node_weather girano in parallelo
dopo node_classify tramite branch condizionali.
node_answer aspetta tutti e tre prima di partire.
"""

from __future__ import annotations
import asyncio
import logging
from concurrent.futures import ThreadPoolExecutor
from functools import partial

from langgraph.graph import StateGraph, START, END
from langgraph.checkpoint.memory import MemorySaver

from .state import PlanetariumState
from .emitter import SseEmitter
from .nodes.node_classify import node_classify
from .nodes.node_emit_cmds import node_emit_cmds
from .nodes.node_rag import node_rag
from .nodes.node_weather import node_weather
from .nodes.node_answer import node_answer

logger = logging.getLogger(__name__)

_executor = ThreadPoolExecutor(max_workers=8)


# ── Router condizionale ───────────────────────────────────────────────────────

def route_after_classify(state) -> list[str]:
    if not isinstance(state, dict):
        state = dict(state)
    intenti = state.get("intenti", [])
    cmds    = state.get("cmds_planetario", [])
    targets = []
    if cmds:
        targets.append("node_emit_cmds")
    if "info" in intenti or "compound" in intenti or "visual" in intenti:
        targets.append("node_rag")
    if "weather" in intenti or "compound" in intenti:
        targets.append("node_weather")
    if not targets:
        targets.append("node_answer")
    return targets

# ── Wrapper per iniettare l'emitter nei nodi ──────────────────────────────────

def _make_node(fn, emitter: SseEmitter):
    def wrapped(state) -> dict:
        # LangGraph passa lo stato come dict — lo accediamo direttamente
        # senza conversione a oggetto, usando get() con default
        if not isinstance(state, dict):
            state = dict(state)
        return fn(state, emitter)
    wrapped.__name__ = fn.__name__
    return wrapped

# ── Builder del grafo ─────────────────────────────────────────────────────────

def build_graph(emitter: SseEmitter) -> StateGraph:
    """
    Costruisce il grafo per una singola richiesta, iniettando l'emitter.
    Viene chiamato una volta per ogni richiesta /api/chat.
    """
    builder = StateGraph(PlanetariumState)

    # Nodi con emitter iniettato
    builder.add_node("node_classify",   _make_node(node_classify,   emitter))
    builder.add_node("node_emit_cmds",  _make_node(node_emit_cmds,  emitter))
    builder.add_node("node_rag",        _make_node(node_rag,        emitter))
    builder.add_node("node_weather",    _make_node(node_weather,    emitter))
    builder.add_node("node_answer",     _make_node(node_answer,     emitter))

    # Edges
    builder.add_edge(START, "node_classify")

    # Branch parallelo dopo classify
    builder.add_conditional_edges(
        "node_classify",
        route_after_classify,
        {
            "node_emit_cmds": "node_emit_cmds",
            "node_rag":       "node_rag",
            "node_weather":   "node_weather",
            "node_answer":    "node_answer",
        }
    )

    # Tutti i rami convergono su node_answer
    builder.add_edge("node_emit_cmds", "node_answer")
    builder.add_edge("node_rag",       "node_answer")
    builder.add_edge("node_weather",   "node_answer")
    builder.add_edge("node_answer",    END)

    return builder


def build_compiled_graph(emitter: SseEmitter, checkpointer: MemorySaver):
    """
    Compila il grafo con MemorySaver per la history della conversazione.
    """
    builder = build_graph(emitter)
    return builder.compile(checkpointer=checkpointer)


# ── Esecuzione asincrona del grafo ────────────────────────────────────────────

async def run_graph(
    state_input: dict,
    emitter: SseEmitter,
    checkpointer: MemorySaver,
    session_id: str,
) -> None:
    """
    Esegue il grafo in un thread separato (i nodi sono sincroni)
    mentre FastAPI streamma gli eventi SSE in parallelo.
    """
    loop = asyncio.get_event_loop()
    emitter.attach_loop(loop)

    graph = build_compiled_graph(emitter, checkpointer)

    config = {
        "configurable": {
            "thread_id": session_id or "anonymous",
        }
    }

    def _run():
        try:
            graph.invoke(state_input, config=config)
        except Exception as e:
            import traceback
            logger.error(f"[GRAPH] Errore esecuzione: {e}")
            logger.error(traceback.format_exc())    # ← aggiunto
            from agent.emitter import emit_error
            emit_error(emitter, "Errore interno. Riprova tra un momento.")
        finally:
            emitter.close_sync()

    await loop.run_in_executor(_executor, _run)
