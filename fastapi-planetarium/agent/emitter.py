"""
emitter.py
Gestione della coda SSE per lo streaming verso Node.JS.
"""

from __future__ import annotations
import asyncio
import json
import logging
from typing import Any, AsyncIterator

logger = logging.getLogger(__name__)

SENTINEL = object()

class SseEmitter:
    def __init__(self):
        self._queue: asyncio.Queue = asyncio.Queue()
        self._loop:  asyncio.AbstractEventLoop | None = None

    def attach_loop(self, loop: asyncio.AbstractEventLoop) -> None:
        self._loop = loop

    def emit_sync(self, tipo: str, **kwargs: Any) -> None:
        event = {"tipo": tipo, **kwargs}
        if self._loop and self._loop.is_running():
            self._loop.call_soon_threadsafe(self._queue.put_nowait, event)
        else:
            try:
                self._queue.put_nowait(event)
            except Exception as e:
                logger.warning(f"[EMITTER] emit_sync fallito: {e}")

    def close_sync(self) -> None:
        if self._loop and self._loop.is_running():
            self._loop.call_soon_threadsafe(self._queue.put_nowait, SENTINEL)
        else:
            try:
                self._queue.put_nowait(SENTINEL)
            except Exception:
                pass

    async def stream(self) -> AsyncIterator[str]:
        while True:
            item = await self._queue.get()
            if item is SENTINEL:
                break
            try:
                yield json.dumps(item, ensure_ascii=False) + "\n"
            except Exception as e:
                logger.error(f"[EMITTER] Serializzazione fallita: {e}")

# ── Helper per i nodi (Sincronizzati con node_classify e node_weather) ────────

def emit_thinking(emitter: SseEmitter, text: str) -> None:
    emitter.emit_sync("thinking", text=text)

def emit_command(emitter: SseEmitter, cmd: str, label: str = "", extra: dict = {}) -> None:
    emitter.emit_sync("cmd", cmd=cmd, label=label, extra=extra)

def emit_cmd(emitter: SseEmitter, cmd: str, label: str = "", extra: dict = {}) -> None:
    """Alias di emit_command"""
    emit_command(emitter, cmd, label, extra)

def emit_weather(emitter: SseEmitter, weather: dict) -> None:
    """Invia i dati meteo al frontend"""
    emitter.emit_sync("weather", **weather)

def emit_rag_result(emitter: SseEmitter, found: int, label: str = "") -> None:
    emitter.emit_sync("rag_result", found=found, label=label)

def emit_token(emitter: SseEmitter, text: str) -> None:
    emitter.emit_sync("token", text=text)

def emit_final(emitter: SseEmitter, text: str, sources: list = [], extra: dict = {}) -> None:
    emitter.emit_sync("final", text=text, sources=sources, extra=extra)

def emit_error(emitter: SseEmitter, text: str) -> None:
    emitter.emit_sync("error", text=text)
