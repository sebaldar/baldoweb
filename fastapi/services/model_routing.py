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
from typing import Literal, Optional

logger = logging.getLogger(__name__)

Provider = Literal["anthropic", "openai", "deepseek", "nessuno"]

PROVIDER_DEFAULT: Provider = "anthropic"
PROVIDER_VALIDI: tuple[Provider, ...] = ("anthropic", "openai", "deepseek")

# Valore speciale: non instrada verso un provider diverso, salta del tutto
# la chiamata LLM di quella fase (il nodo in nodes.py sostituisce un
# comportamento di default sensato — es. il draft diventa il racconto
# finale senza rifinitura). Non è un "provider" in senso proprio, quindi
# resta fuori da PROVIDER_VALIDI: ogni chiamante che instrada verso un
# vero LLM (services/llm.py) deve continuare a trattarlo come invalido.
BYPASS: Provider = "nessuno"

# Solo le fasi dove saltare la chiamata ha un sostituto sicuro e ovvio:
# rifinisci (il draft resta com'è, la coerenza della domanda finale è
# comunque presidiata da verifica_coerenza_domanda.check/.fix, un nodo
# indipendente), il rewrite delle similitudini (si accetta il rischio di
# qualche similitudine in più) e il retry di ricerca KB (si va avanti
# senza frammenti invece di riprovare). Le altre fasi — in particolare
# valuta_prompt, il controllo di sicurezza sul prompt — restano escluse
# deliberatamente: un bypass lì va deciso con più attrito di un menu a
# tendina, non aggiunto qui come le altre.
FASI_CON_BYPASS: frozenset[str] = frozenset({
    "rifinisci",
    "verifica_coerenza_domanda.similitudini",
    "valuta_frammenti._riformula_termini",
})

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
    """
    Il processo FastAPI gira con più worker uvicorn (--workers 2): sono
    processi separati, senza memoria condivisa. Una scrittura da un worker
    (via set_provider) aggiorna il file ma NON la copia in RAM degli altri
    worker — senza qualcosa che se ne accorga, una fase impostata da admin
    può restare silenziosamente sul valore vecchio su metà delle richieste,
    a seconda di quale worker le serve (osservato: una fase appena spostata
    su DeepSeek è comparsa su Claude nel report di una generazione reale).
    Rimedio economico: invece di tenere una cache che si crede sempre
    valida, ricontrolliamo il mtime del file a ogni lettura e ricarichiamo
    solo se è cambiato — il file è pochi byte, il controllo è quasi gratis,
    e ogni worker si allinea al più tardi alla lettura successiva.
    """

    def __init__(self, path: Path = _PATH_DEFAULT):
        self._path = path
        self._lock = threading.Lock()
        self._config: dict[str, Provider] = {}
        self._mtime: Optional[float] = None
        self._carica()

    @staticmethod
    def _valore_valido(fase: str, valore) -> bool:
        if valore in PROVIDER_VALIDI:
            return True
        return valore == BYPASS and fase in FASI_CON_BYPASS

    def _carica(self) -> None:
        with self._lock:
            if self._path.exists():
                try:
                    dati = json.loads(self._path.read_text())
                    self._config = {
                        fase: dati.get(fase)
                        if self._valore_valido(fase, dati.get(fase))
                        else PROVIDER_DEFAULT
                        for fase in FASI
                    }
                    self._mtime = self._path.stat().st_mtime
                    logger.info(f"📋 Routing modelli caricato da {self._path}.")
                    return
                except Exception as e:
                    logger.warning(
                        f"⚠️ {self._path} illeggibile ({e}), uso i default "
                        "(tutte le fasi su Claude, comportamento invariato)."
                    )
            self._config = {fase: PROVIDER_DEFAULT for fase in FASI}

    def _ricarica_se_cambiato(self) -> None:
        try:
            mtime_attuale = self._path.stat().st_mtime
        except FileNotFoundError:
            return
        if mtime_attuale != self._mtime:
            self._carica()

    def _salva(self) -> None:
        with self._lock:
            self._path.parent.mkdir(parents=True, exist_ok=True)
            self._path.write_text(
                json.dumps(self._config, ensure_ascii=False, indent=2)
            )
            self._mtime = self._path.stat().st_mtime

    def get_provider(self, fase: str) -> Provider:
        """Provider configurato per la fase, o il default se la fase non è
        (ancora) nota — fase='generico' delle chiamate senza etichetta usa
        sempre e solo il default."""
        self._ricarica_se_cambiato()
        return self._config.get(fase, PROVIDER_DEFAULT)

    def get_tutti(self) -> dict[str, Provider]:
        self._ricarica_se_cambiato()
        return dict(self._config)

    def set_provider(self, fase: str, provider: str) -> None:
        if fase not in FASI:
            raise ValueError(f"Fase sconosciuta: '{fase}'")
        if not self._valore_valido(fase, provider):
            validi = PROVIDER_VALIDI + ((BYPASS,) if fase in FASI_CON_BYPASS else ())
            raise ValueError(
                f"Provider non valido per la fase '{fase}': '{provider}'. "
                f"Validi: {', '.join(validi)}"
            )
        # _salva() riscrive l'intero dizionario: partire da una copia in RAM
        # non aggiornata (un altro worker ha scritto sul file nel frattempo,
        # ma questo worker non l'ha ancora ricaricata) cancellerebbe in
        # silenzio le fasi che quell'altra scrittura aveva appena impostato
        # — osservato in produzione: una PUT su 'genera_draft' ha fatto
        # tornare 'decide_tools' al valore di qualche minuto prima. Rileggere
        # subito prima di mutare non elimina la finestra di corsa fra
        # processi (nessun lock in-process la copre), ma la riduce al minimo.
        self._ricarica_se_cambiato()
        with self._lock:
            self._config[fase] = provider  # type: ignore[assignment]
        self._salva()
        logger.info(f"📋 Fase '{fase}' → provider '{provider}'.")
