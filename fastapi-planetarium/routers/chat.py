from fastapi import APIRouter
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field
import asyncio, uuid
from typing import Any, Literal

from agent.emitter import SseEmitter
from agent.graph import run_graph
from langgraph.checkpoint.memory import MemorySaver
from services.conversations import conversations

router = APIRouter()

class ChatRequest(BaseModel):
    prompt:     str          = Field(..., min_length=1, max_length=2000)
    provider: Literal['ionos', 'anthropic', 'openai', 'gemini'] = 'ionos'
    session_id: str | None = Field(default=None, max_length=128)
    request_id: str | None = Field(default=None, max_length=80)
    data:       str | None   = None
    lat: float | None = Field(default=None, ge=-90, le=90, allow_inf_nan=False)
    lon: float | None = Field(default=None, ge=-180, le=180, allow_inf_nan=False)
    motore_astronomico: Any | None = None

@router.post("/api/chat")
async def chat(req: ChatRequest) -> StreamingResponse:
    session_id  = req.session_id or str(uuid.uuid4())
    emitter = SseEmitter()
    emitter.attach_loop(asyncio.get_running_loop())
    try:
        history = conversations.acquire(session_id)
    except RuntimeError as error:
        emitter.emit_sync('error', text=str(error))
        emitter.close_sync()
        return StreamingResponse(emitter.stream(), media_type='application/x-ndjson')
    state_input = {
        "messages": history + [{"role": "user", "content": req.prompt}],
        "prompt":     req.prompt,
        "provider":   req.provider,
        "session_id": session_id,
        "data":       req.data,
        "lat":        req.lat,
        "lon":        req.lon,
        "motore_astronomico": req.motore_astronomico,
    }
    async def run_conversation():
        result = None
        try:
            result = await run_graph(state_input, emitter, MemorySaver(), session_id)
        except Exception:
            emitter.emit_sync('error', text='Errore interno. Riprova tra un momento.')
        finally:
            conversations.release(session_id, req.prompt,
                result.get('answer_text') if result and not emitter.cancelled else None)
            emitter.close_sync()
    task = asyncio.create_task(run_conversation())
    # Keep a strong reference until completion, including when the consumer disconnects.
    active_tasks.add(task)
    task.add_done_callback(active_tasks.discard)
    return StreamingResponse(
        emitter.stream(),
        media_type="application/x-ndjson",
        headers={"X-Session-Id": session_id, "Cache-Control": "no-cache", "X-Accel-Buffering": "no"},
    )


active_tasks = set()
