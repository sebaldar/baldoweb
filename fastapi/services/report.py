"""
services/report.py
===================
Genera un report YAML ad uso amministrativo per ogni storia generata con
successo: età, personalizzazione (nome/colore/animale preferiti), prompt,
testo finale, tempo di elaborazione, modello/i LLM effettivamente usati,
token consumati e frammenti/personaggi impiegati. Pensato per il debug e
il controllo qualità/costi senza dover incrociare i log applicativi.
"""

import logging
from datetime import datetime, timezone
from pathlib import Path

import yaml

logger = logging.getLogger(__name__)

# Montata come volume Docker (vedi docker-compose.yml), sullo stesso
# modello di ./fastapi/logs:/app/logs, così i report sopravvivono a un
# rebuild del container invece di restare nel filesystem effimero.
DIRECTORY_STORIE = Path(__file__).resolve().parent.parent / "stories"


def _aggrega_uso_llm(llm_usage: list) -> dict:
    """
    Una storia comporta più chiamate LLM (analisi, valutazioni, draft,
    rifinitura, verifica finale) — qui aggreghiamo modello/i e token totali,
    ma teniamo anche il dettaglio per-chiamata per il debug.
    """
    llm_usage = llm_usage or []
    modelli = sorted({u.get("modello") for u in llm_usage if u.get("modello")})
    return {
        "modelli_utilizzati": modelli,
        "token_input": sum(u.get("token_input", 0) for u in llm_usage),
        "token_output": sum(u.get("token_output", 0) for u in llm_usage),
        "dettaglio_chiamate": llm_usage,
    }


def salva_report_storia(state: dict, tempo_elaborazione_secondi: float) -> None:
    """
    Scrive il report YAML della storia appena generata in DIRECTORY_STORIE.
    Non solleva mai eccezioni verso il chiamante: un report mancato è un
    problema di osservabilità, non deve far fallire una storia già
    generata e già inviata al bambino.
    """
    try:
        DIRECTORY_STORIE.mkdir(parents=True, exist_ok=True)

        storia_id = state.get("storia_id") or (
            f"senza-id-{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S')}"
        )
        uso = _aggrega_uso_llm(state.get("llm_usage"))

        report = {
            "storia_id": storia_id,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "eta": state.get("eta_bambino"),
            "nome": state.get("nome"),
            # None se non c'era un nome da verificare, altrimenti esito del
            # controllo di fedeltà (verifica_coerenza_domanda): il nome dato
            # dal form compare parola per parola nel racconto finale?
            "nome_presente_in_output": state.get("nome_presente_in_output"),
            "colore_preferito": state.get("colore_preferito"),
            "animale_preferito": state.get("animale_preferito"),
            "prompt": state.get("prompt_originale"),
            "storia_generata": state.get("racconto_finale"),
            "tempo_elaborazione_secondi": round(tempo_elaborazione_secondi, 2),
            "modelli_utilizzati": uso["modelli_utilizzati"],
            "token_input": uso["token_input"],
            "token_output": uso["token_output"],
            "dettaglio_chiamate_llm": uso["dettaglio_chiamate"],
            "frammenti_usati": state.get("frammenti_usati") or [],
            "personaggi": state.get("personaggi") or [],
            # Rilevato dal draft (frase ripetuta 2+ volte), non il testo
            # grezzo del frammento KB — vedi agent/nodes.py:_rileva_ritornello.
            # None se il draft non ha ripetuto nulla di riconoscibile.
            "ritornello_atteso": state.get("ritornello_atteso"),
            # Valori grezzi di tecnica_narrativa/archetipo dal frammento
            # (jargon per l'autore) e se sono trapelati alla lettera nel
            # racconto finale — non dovrebbero mai esserlo.
            "tecnica_narrativa_kb": state.get("tecnica_narrativa_kb"),
            "archetipo_kb": state.get("archetipo_kb"),
            "termini_kb_trapelati": state.get("termini_kb_trapelati") or [],
            # Contesto fisico usato per la storia: senza luogo/data/ora
            # i risultati meteo/astro da soli non si possono verificare
            # (es. per capire se la fase lunare riportata è plausibile).
            "luogo": state.get("luogo"),
            "data_storia": state.get("data_storia"),
            "ora_storia": state.get("ora_storia"),
            "condizioni_meteo": state.get("dati_meteo"),
            "usa_astronomia": state.get("usa_astronomia"),
            "dati_astronomici": state.get("dati_astronomici") if state.get("usa_astronomia") else None,
        }

        percorso = DIRECTORY_STORIE / f"{storia_id}.yaml"
        with open(percorso, "w", encoding="utf-8") as f:
            yaml.safe_dump(report, f, allow_unicode=True, sort_keys=False, width=100)

        logger.info(f"[report] Report YAML salvato: {percorso}")

    except Exception as e:
        logger.error(f"[report] Impossibile salvare il report della storia: {e}", exc_info=True)
