"""
nodes/node_classify.py
Classifica l'intento e genera i comandi diretti.
"""
from __future__ import annotations
import json
import logging
import re

from langchain_core.messages import SystemMessage, HumanMessage

from services.config import get_llm
from agent.state import PlanetariumState
from agent.emitter import SseEmitter, emit_thinking

logger = logging.getLogger(__name__)

BODY_TO_CMD: dict[str, str] = {
    "sole": "sun", "sun": "sun",
    "mercurio": "mercury", "mercury": "mercury",
    "venere": "venus", "venus": "venus",
    "marte": "mars", "mars": "mars",
    "giove": "jupiter", "jupiter": "jupiter",
    "saturno": "saturn", "saturn": "saturn",
    "urano": "uranus", "uranus": "uranus",
    "nettuno": "neptune", "neptune": "neptune",
    "luna": "moon", "moon": "moon",
    "sirio": "Sirius", "sirius": "Sirius",
    "orione": "Orion", "orion": "Orion",
    "scorpione": "Scorpius", "scorpius": "Scorpius",
    "cassiopea": "Cassiopeia", "cassiopeia": "Cassiopeia",
    "andromeda": "Andromeda",
    "perseo": "Perseus", "perseus": "Perseus",
    "cigno": "Cygnus", "cygnus": "Cygnus",
    "lira": "Lyra", "lyra": "Lyra",
    "aquila": "Aquila",
    "toro": "Taurus", "taurus": "Taurus",
    "gemelli": "Gemini", "gemini": "Gemini",
    "leone": "Leo", "leo": "Leo",
    "vergine": "Virgo", "virgo": "Virgo",
    "grande orsa": "UMa", "orsa maggiore": "UMa",
    "piccola orsa": "UMi", "orsa minore": "UMi",
}

CLASSIFY_SYSTEM = """Sei un classificatore astronomico per un planetario 3D. Analizza la domanda e rispondi SOLO con JSON.

REGOLE DI COMPORTAMENTO:
1. SPOSTAMENTO RELATIVO: Se l'utente specifica GRADI (es. "15 gradi nord"), usa "spostamento": {"alt": 15, "az": 0}.
2. PUNTAMENTO FISSO: Se chiede una direzione senza gradi (es. "guarda a Nord"), usa "direzione": "N".
3. CORPI CELESTI: Se l'utente nomina una stella, costellazione o pianeta (es. "Orione", "Sirio", "Luna"), DEVI inserirla nell'array "corpi_celesti".

Schema JSON RIGOROSO:
{
  "intenti": ["visual"],
  "corpi_celesti": [],
  "direzione": null,
  "spostamento": {"alt": 0, "az": 0},
  "thinking_text": "Elaboro il comando..."
}
"""

def node_classify(state: PlanetariumState, emitter: SseEmitter) -> dict:
    emit_thinking(emitter, "Analizzo la domanda…")

    provider = state.get("provider", "ionos")
    prompt   = state.get("prompt", "")

    intenti = ["info"]
    extra = {}
    parsed = {}
    cmds: list[dict] = []

    try:
        llm = get_llm(provider, streaming=False)
        response = llm.invoke([
            SystemMessage(content=CLASSIFY_SYSTEM),
            HumanMessage(content=prompt),
        ])
        raw = response.content.strip()
        json_match = re.search(r'\{.*\}', raw, re.DOTALL)

        if json_match:
            parsed = json.loads(json_match.group())
            intenti = parsed.get("intenti") or ["info"]
            emit_thinking(emitter, parsed.get("thinking_text", "Elaboro…"))

            has_moved = False

            # 1. Movimenti
            spostamento = parsed.get("spostamento")
            if isinstance(spostamento, dict):
                d_alt = spostamento.get("alt", 0) or 0
                d_az  = spostamento.get("az", 0) or 0
                if d_alt != 0 or d_az != 0:
                    cmds.append({"cmd": f"move {d_alt} {d_az}", "label": f"Sposto alt:{d_alt} az:{d_az}"})
                    has_moved = True

            # 2. Direzioni
            direzione = parsed.get("direzione")
            if direzione in {"N", "S", "E", "W", "Z"} and not has_moved:
                cmds.append({"cmd": f"view {direzione}", "label": f"Guardo a {direzione}"})
                has_moved = True

            # Salviamo l'informazione che l'utente ha mosso la telecamera
            if has_moved:
                extra["is_movement"] = True

            # 3. Parametri
            for key, cmd_base in [("zoom", "zoom"), ("fov", "fov")]:
                val = parsed.get(key)
                if val is not None:
                    cmds.append({"cmd": f"{cmd_base} {val}", "label": ""})

            # 4. Corpi Celesti
            corpi_grezzi = parsed.get("corpi_celesti") or []
            if corpi_grezzi:
                # Estraiamo il nome originale e una versione pulita
                nome_originale = corpi_grezzi[0]
                nome_pulito = nome_originale.lower().replace("l'", "").replace("il ", "").strip()

                # Se è nel dizionario usiamo la traduzione, ALTRIMENTI usiamo il nome originale!
                cmd_target = BODY_TO_CMD.get(nome_pulito, nome_originale)

                extra["target"] = cmd_target
                extra["needs_realtime"] = nome_pulito in {"sole", "sun", "luna", "moon", "marte", "mars", "giove", "jupiter", "saturno", "saturn", "venere", "venus", "mercurio", "mercury", "urano", "uranus", "nettuno", "neptune"}
                extra["category"] = "pianeta" if extra["needs_realtime"] else "stella"

    except Exception as e:
        logger.error(f"[CLASSIFY] Errore critico: {e}")

    return {
        "intenti": intenti,
        "prompt": prompt,
        "extra": extra,
        "cmds_planetario": cmds,
    }
