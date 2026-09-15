"""
services/model_routing.py
==========================
Configurazione, persistita su file, di quale provider LLM usare per ciascuna
fase della pipeline agentica.

Perché esiste: le fasi della pipeline non sono tutte uguali. I controlli
SI/NO (valuta_prompt, decide_tools, i due check di verifica_coerenza_domanda)
sono classificazioni economiche a pochi token di risposta — buoni candidati
per un provider più economico. genera_draft, rifinisci e il rewrite delle
similitudini dipendono invece da un ampio lavoro di prompt-engineering
verificato specificamente su Claude (decine di regole di stile italiane
fini — apertura di Baldo, ritornello, reduplicazioni, morale prima della
domanda finale...): spostarle su un altro modello senza una verifica di
tenuta dedicata rischia di riaprire in silenzio problemi già chiusi. Questo
modulo permette all'amministratore di scegliere provider per provider, fase
per fase, senza toccare codice — ma non decide da solo quali spostare.

Persistenza: un file JSON su un volume montato (stesso pattern già in uso
per fastapi/stories), non un DB dedicato — la configurazione è un piccolo
oggetto piatto, non giustifica una nuova dipendenza.
"""

import json
import logging
import threading
from pathlib import Path
from typing import Literal

logger = logging.getLogger(__name__)

Provider = Literal["anthropic", "openai", "deepseek"]

PROVIDER_DEFAULT: Provider = "anthropic"
PROVIDER_VALIDI: tuple[Provider, ...] = ("anthropic", "openai", "deepseek")

# Stessi nomi usati nel campo "nodo" di dettaglio_chiamate_llm nei report
# YAML delle storie — così l'amministratore riconosce le fasi guardando i
# report che già consulta, senza un secondo vocabolario da imparare.
FASI: tuple[str, ...] = (
    "analizza_prompt",
    "valuta_prompt",
    "decide_tools",
    "valuta_frammenti._riformula_termini",
    "genera_draft",
    "rifinisci",
    "verifica_coerenza_domanda.ritornello",
    "verifica_coerenza_domanda.check",
    "verifica_coerenza_domanda.fix",
    "verifica_coerenza_domanda.similitudini",
)

_PATH_DEFAULT = Path("/app/config_data/model_routing.json")


class ModelRoutingConfig:
    def __init__(self, path: Path = _PATH_DEFAULT):
        self._path = path
        self._lock = threading.Lock()
        self._config: dict[str, Provider] = {}
        self._carica()

    def _carica(self) -> None:
        with self._lock:
            if self._path.exists():
                try:
                    dati = json.loads(self._path.read_text())
                    self._config = {
                        fase: dati.get(fase, PROVIDER_DEFAULT)
                        if dati.get(fase) in PROVIDER_VALIDI
                        else PROVIDER_DEFAULT
                        for fase in FASI
                    }
                    logger.info(f"📋 Routing modelli caricato da {self._path}.")
                    return
                except Exception as e:
                    logger.warning(
                        f"⚠️ {self._path} illeggibile ({e}), uso i default "
                        "(tutte le fasi su Claude, comportamento invariato)."
                    )
            self._config = {fase: PROVIDER_DEFAULT for fase in FASI}

    def _salva(self) -> None:
        with self._lock:
            self._path.parent.mkdir(parents=True, exist_ok=True)
            self._path.write_text(
                json.dumps(self._config, ensure_ascii=False, indent=2)
            )

    def get_provider(self, fase: str) -> Provider:
        """Provider configurato per la fase, o il default se la fase non è
        (ancora) nota — fase='generico' delle chiamate senza etichetta usa
        sempre e solo il default."""
        return self._config.get(fase, PROVIDER_DEFAULT)

    def get_tutti(self) -> dict[str, Provider]:
        return dict(self._config)

    def set_provider(self, fase: str, provider: str) -> None:
        if fase not in FASI:
            raise ValueError(f"Fase sconosciuta: '{fase}'")
        if provider not in PROVIDER_VALIDI:
            raise ValueError(f"Provider sconosciuto: '{provider}'")
        with self._lock:
            self._config[fase] = provider  # type: ignore[assignment]
        self._salva()
        logger.info(f"📋 Fase '{fase}' → provider '{provider}'.")
