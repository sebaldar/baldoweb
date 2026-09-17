"""
state.py
Stato condiviso del grafo LangGraph.
Ogni campo è accumulabile (Annotated + operator) o sovrascrivibile.
"""

from __future__ import annotations
from typing import Annotated, Any
import operator
from dataclasses import dataclass, field
from langgraph.graph import MessagesState


# ── Evento SSE verso il client ────────────────────────────────────────────────

@dataclass
class SseEvent:
    """
    Unità atomica di output verso il client WebSocket.
    Il Node.JS gateway la serializza e la invia as-is.
    """
    tipo:  str                      # thinking | cmd | rag_result | weather | token | final | error
    data:  dict[str, Any]           # payload specifico per tipo

    extra: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {"tipo": self.tipo, **self.data}


# ── Stato del grafo ───────────────────────────────────────────────────────────

class PlanetariumState(MessagesState):
    # ── Input ─────────────────────────────────────────────────────────────────
    prompt:     str   = ""
    provider:   str   = "ionos"
    session_id: str | None = None
    data:       str | None = None
    lat:        float | None = None
    lon:        float | None = None

    # IL LASCIAPASSARE NELLO STATO:
    motore_astronomico: Any | None = None

    # ── Classificazione ───────────────────────────────────────────────────────
    intenti:    list[str] = field(default_factory=list)
    cmds_planetario: Annotated[list[dict], operator.add] = field(default_factory=list)
    structured_query: dict | None = None

    # ── Risultati nodi paralleli ───────────────────────────────────────────────
    rag_docs:       list[dict] = field(default_factory=list)
    weather_data:   dict | None = None

    # ── Output streaming e Dati Tecnici ───────────────────────────────────────
    events: Annotated[list[SseEvent], operator.add] = field(default_factory=list)
    answer_text: str = ""

    # è il "contenitore" che salva i dati per Node.js!
    extra: dict[str, Any] = field(default_factory=dict)
