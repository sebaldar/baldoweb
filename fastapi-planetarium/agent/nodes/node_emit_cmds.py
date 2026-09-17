from __future__ import annotations
import logging

from agent.state import PlanetariumState
from agent.emitter import SseEmitter, emit_cmd

logger = logging.getLogger(__name__)

def node_emit_cmds(state: PlanetariumState, emitter: SseEmitter) -> dict:
    cmds = state.get("cmds_planetario", [])

    # Peschiamo l'extra generato da node_classify
    extra_data = state.get("extra", {})

    for c in cmds:
        # Passiamo l'extra all'emitter!
        emit_cmd(emitter, cmd=c["cmd"], label=c.get("label", ""), extra=extra_data)

    return {}
