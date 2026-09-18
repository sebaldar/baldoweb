from fastapi import APIRouter
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field
import asyncio, uuid
from typing import Any

from agent.emitter import SseEmitter
from agent.graph import run_graph
from langgraph.checkpoint.memory import MemorySaver

router = APIRouter()

class ChatRequest(BaseModel):
    prompt:     str          = Field(..., min_length=1, max_length=2000)
    provider:   str          = Field(default="ionos")
    session_id: str | None   = None
    data:       str | None   = None
    lat:        float | None = None
    lon:        float | None = None
    motore_astronomico: Any | None = None

@router.post("/api/chat")
async def chat(req: ChatRequest) -> StreamingResponse:
    session_id  = req.session_id or str(uuid.uuid4())
    state_input = {
        "messages":   [{"role": "user", "content": req.prompt}],
        "prompt":     req.prompt,
        "provider":   req.provider,
        "session_id": session_id,
        "data":       req.data,
        "lat":        req.lat,
        "lon":        req.lon,
        "motore_astronomico": req.motore_astronomico,
    }
    emitter = SseEmitter()
    # Each request owns its checkpointer so state is released once the stream ends,
    # instead of accumulating forever in a process-wide MemorySaver.
    asyncio.create_task(run_graph(state_input, emitter, MemorySaver(), session_id))
    return StreamingResponse(
        emitter.stream(),
        media_type="application/x-ndjson",
        headers={"X-Session-Id": session_id, "Cache-Control": "no-cache", "X-Accel-Buffering": "no"},
    )
