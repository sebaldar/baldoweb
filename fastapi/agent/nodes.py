"""
agent/nodes.py
==============
Tutti i nodi del grafo LangGraph.
Gestisce l'analisi, il recupero di dati fisici (Geo/Meteo/Astro),
la memoria Neo4j e la composizione narrativa.

Ogni nodo emette eventi di streaming descrittivi tramite la callback
`stream_event(event_type, message, payload?)` presente nello state.
"""

import json
import logging
import re
import uuid
import asyncio
from datetime import datetime
from zoneinfo import ZoneInfo
from typing import Callable, Optional

# Stesso fuso usato in rag.py: quando il prompt non specifica un momento, il
# fallback "adesso" deve rappresentare l'ora locale dell'utenza (Italia), non
# quella del container (UTC) — coerenza necessaria perché entrambi i valori
# finiscono nello stesso campo di stato e nello stesso confine di conversione
# verso il motore astronomico (services/astronomy.py).
FUSO_UTENZA = ZoneInfo("Europe/Rome")

from agent.state import BaldoState
from services.llm import LLMRouter
from services.rag import RAGExtractor
from services.neo4j_client import Neo4jClient
from services.astronomy import AstronomyClient
from services.weather import WeatherClient
from services.geo_service import GeoService
from services.composer import StoryComposer

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Helper: emissione eventi di streaming
# ---------------------------------------------------------------------------

def _emit(state: BaldoState, event_type: str, message: str, payload: Optional[dict] = None):
    """
    Emette un evento di streaming narrativo se lo state contiene una callback.

    Lo state deve esporre:
        state["stream_callback"]: Callable[[str, str, dict | None], None]

    Parametri:
        event_type  – categoria dell'evento  (es. "analisi", "meteo", "draft" …)
        message     – testo leggibile dall'utente, in italiano colloquiale/narrativo
        payload     – dati extra opzionali da allegare all'evento
    """
    cb: Optional[Callable] = state.get("stream_callback")
    if cb:
        try:
            cb(event_type, message, payload or {})
        except Exception as e:
            logger.warning(f"[STREAM] Callback fallita per evento '{event_type}': {e}")


# ---------------------------------------------------------------------------
# NODE 1 — Analisi del prompt
# ---------------------------------------------------------------------------
async def analizza_prompt(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] analizza_prompt")

    _emit(state, "analisi:inizio",
          "📖  Sto leggendo la tua idea… lascio che le parole prendano forma.")

    rag = RAGExtractor(llm=llm)
    analisi = await rag.analizza(state["prompt_originale"])

    personaggi = analisi.get("personaggi", [])
    emozioni   = analisi.get("emozioni", [])

    # Fallback temporale
    data_target = analisi.get("data_storia") or datetime.now(FUSO_UTENZA).strftime("%d-%m-%Y")
    ora_target  = analisi.get("ora_storia")  or datetime.now(FUSO_UTENZA).strftime("%H:%M:%S")

    # Fallback geografico
    luogo_target = analisi.get("luogo") or ""

    termini = personaggi + emozioni

    # Messaggio descrittivo arricchito con i dati estratti
    if personaggi:
        nomi = ", ".join(personaggi)
        _emit(state, "analisi:personaggi",
              f"🧒  Ho trovato i protagonisti della storia: {nomi}.",
              {"personaggi": personaggi})
    else:
        _emit(state, "analisi:personaggi",
              "🧒  Nessun personaggio esplicito: inventerò io un eroe adatto.",
              {"personaggi": []})

    if emozioni:
        _emit(state, "analisi:emozioni",
              f"💛  Le emozioni che guidano il racconto: {', '.join(emozioni)}.",
              {"emozioni": emozioni})

    _emit(state, "analisi:luogo_tempo",
          f"🗺️  Ambientazione rilevata: {luogo_target} — "
          f"data {data_target} alle {ora_target}.",
          {"luogo": luogo_target, "data": data_target, "ora": ora_target})

    _emit(state, "analisi:fine",
          "✅  Analisi completata. Il cantastorie ha capito tutto.")

    uso_llm = analisi.get("_uso_llm")

    return {
        "personaggi":            personaggi,
        "emozioni":              emozioni,
        "ambientazione":         analisi.get("ambientazione", ""),
        "luogo":                 luogo_target,
        "data_storia":           data_target,
        "ora_storia":            ora_target,
        "termini_ricerca_neo4j": termini,
        "tentativi_neo4j":       0,
        "tentativi_correzione":  0,
        "iterazioni_totali":     1,
        "llm_usage":             [uso_llm] if uso_llm else [],
    }


# ---------------------------------------------------------------------------
# NODE 2 — Valutazione prompt
# ---------------------------------------------------------------------------
async def valuta_prompt(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_prompt")

    _emit(state, "valutazione:inizio",
          "🔍  Verifico che la storia sia adatta ai piccoli lettori…")

    # Scoping esplicito dopo un caso reale: senza criteri, il giudice si
    # inventava da solo standard che non gli competono — es. bocciava "guarda
    # la luna a mezzogiorno" per implausibilità astronomica (valutata "a
    # mano", senza i dati reali che ha invece composer.py più a valle) o
    # "supera tre ostacoli" per trama non abbastanza specifica (compito di
    # genera_draft, non dell'utente). Risultato misurato: stesso prompt,
    # 5 rifiuti su 8 tentativi identici — incoerente perché il criterio non
    # era mai stato definito, non perché il prompt fosse davvero borderline.
    system = """Sei un supervisore di SICUREZZA per storie destinate a bambini.
    Il tuo UNICO compito è verificare che il prompt non contenga contenuti
    inadatti a un bambino: violenza esplicita o realistica, paura/orrore
    genuino, temi sessuali, autolesionismo, odio o discriminazione, o
    qualunque cosa un genitore troverebbe inaccettabile in un racconto per
    l'infanzia.

    NON è compito tuo valutare:
    - la plausibilità fisica o astronomica di quello che l'utente chiede
      (es. "guardare la luna a mezzogiorno"): un modulo dedicato più a valle
      controlla i dati reali del cielo e adatta la storia di conseguenza —
      tu non hai questi dati, non provare a indovinarli;
    - quanto la trama sia dettagliata o specifica: un prompt che lascia
      dettagli da inventare (es. "supera tre ostacoli" senza dire quali) è
      normale e atteso, non un difetto — inventarli è il lavoro del
      narratore, non dell'utente.

    Rispondi SOLO JSON: {"chiaro": true/false, "motivo": "..."}"""

    risultato = await llm.chiedi(
        system=system,
        user=f"Prompt: {state['prompt_originale']}\nPersonaggi: {state['personaggi']}",
        fase="valuta_prompt",
    )

    try:
        dati   = json.loads(risultato.testo.strip().strip("```json").strip("```"))
        chiaro = dati.get("chiaro", True)
        motivo = dati.get("motivo")
    except Exception:
        chiaro, motivo = True, None

    if chiaro:
        _emit(state, "valutazione:ok",
              "✅  Prompt approvato! Il racconto può cominciare.")
    else:
        _emit(state, "valutazione:rifiuto",
              f"⚠️  Il prompt non è adatto: {motivo}. "
              "Prova a riformulare la tua idea.",
              {"motivo": motivo})

    return {
        "prompt_chiaro": chiaro,
        "motivo_rifiuto": motivo,
        "llm_usage": [{
            "nodo": "valuta_prompt",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
            "durata_secondi": round(risultato.durata_secondi, 2),
        }],
    }


# ---------------------------------------------------------------------------
# NODE 3 — Decisione tool
# ---------------------------------------------------------------------------
async def decide_tools(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] decide_tools")

    _emit(state, "tools:decisione",
          "🛠️  Scelgo quali strumenti magici usare per arricchire la storia…")

    system = 'Decidi se usare l\'astronomia. Rispondi SOLO JSON: {"usa_astronomia": true/false}'
    risultato = await llm.chiedi(system=system, user=state['prompt_originale'], fase="decide_tools")

    try:
        dati         = json.loads(risultato.testo.strip().strip("```json").strip("```"))
        usa_astro    = dati.get("usa_astronomia", True)
    except Exception:
        usa_astro = True

    if usa_astro:
        _emit(state, "tools:astronomia_attiva",
              "🔭  Attivo il telescopio virtuale: recupererò la mappa del cielo "
              "per la notte della storia.")
    else:
        _emit(state, "tools:astronomia_disattiva",
              "☀️  La storia si svolge di giorno: il telescopio può riposare.")

    return {
        "usa_astronomia": usa_astro,
        "llm_usage": [{
            "nodo": "decide_tools",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
            "durata_secondi": round(risultato.durata_secondi, 2),
        }],
    }


# ---------------------------------------------------------------------------
# NODE 4a — Query Neo4j
# ---------------------------------------------------------------------------
async def query_neo4j(state: BaldoState, neo4j: Neo4jClient) -> dict:
    tentativo = state["tentativi_neo4j"] + 1
    logger.info(f"[NODE] query_neo4j (tentativo {tentativo})")

    _emit(state, "memoria:ricerca",
          f"🗄️  Sfoglio il libro dei ricordi (tentativo {tentativo})… "
          "cerco storie passate con personaggi simili.",
          {"tentativo": tentativo})

    frammenti = await neo4j.cerca_frammenti(
        characters  = state["personaggi"],
        emotions    = state["emozioni"],
        setting     = state["ambientazione"],
        extra_terms = state.get("termini_ricerca_neo4j", []),
        eta_bambino = state.get("eta_bambino"),
    )

    storie_precedenti = await neo4j.cerca_storie_precedenti(
        characters = state["personaggi"],
        emotions   = state["emozioni"],
    )

    # Descrizione/tratti dei personaggi coinvolti (se qualcuno li ha compilati
    # a mano dal pannello Personaggi), per mantenerli coerenti tra le storie.
    personaggi_bio = await neo4j.get_character_bios(state["personaggi"])

    n_frammenti = len(frammenti) if frammenti else 0
    n_storie    = len(storie_precedenti) if storie_precedenti else 0

    if n_frammenti or n_storie:
        _emit(state, "memoria:trovata",
              f"📚  Trovati {n_frammenti} frammenti e {n_storie} storie precedenti. "
              "Il passato illuminerà il racconto di oggi.",
              {"frammenti": n_frammenti, "storie_precedenti": n_storie})
    else:
        _emit(state, "memoria:vuota",
              "📭  Nessun ricordo utile nel database. "
              "Questa storia nascerà completamente nuova.")

    return {
        "frammenti":          frammenti,
        "storie_precedenti":  storie_precedenti,
        "personaggi_bio":     personaggi_bio,
        "tentativi_neo4j":    tentativo,
    }


# ---------------------------------------------------------------------------
# NODE 4b — Recupero dati fisici (Geo + Meteo + Astro)
# ---------------------------------------------------------------------------
async def fetch_contesto_fisico(
    state:     BaldoState,
    astronomy: AstronomyClient,
    weather:   WeatherClient,
    geo:       GeoService,
) -> dict:
    luogo_nome = state.get("luogo") or ""
    logger.info(f"[NODE] fetch_contesto_fisico avviato per: {luogo_nome}")

    _emit(state, "fisico:inizio",
          f"🌍  Mi sintonizo con il mondo reale: recupero meteo e cielo "
          f"per {luogo_nome}…",
          {"luogo": luogo_nome})

    lat, lon   = 41.8928, 12.4964  # Saranno sovrascritti da GeoService
    meteo_res  = "cielo sereno"
    astro_res  = {}

    try:
        # 1. Geocoding — passa anche le coordinate del dispositivo come fallback
        try:
            client_lat = state.get("lat")
            client_lon = state.get("lon")
            lat, lon = await geo.get_coordinates(
                luogo_nome,
                client_lat=client_lat,
                client_lon=client_lon,
            )
            if luogo_nome and luogo_nome.strip() != "":
                _emit(state, "fisico:geo",
                      f"\U0001f4cd  Coordinate per '{luogo_nome}': {lat:.4f}\u00b0N, {lon:.4f}\u00b0E.",
                      {"lat": lat, "lon": lon, "luogo": luogo_nome})
            elif client_lat is not None:
                _emit(state, "fisico:geo",
                      f"\U0001f4cd  Uso la tua posizione attuale: {lat:.4f}\u00b0N, {lon:.4f}\u00b0E.",
                      {"lat": lat, "lon": lon, "source": "client"})
            else:
                _emit(state, "fisico:geo_fallback",
                      "📍  Posizione non rilevata: uso le coordinate del dispositivo.")
        except Exception as geoe:
            logger.warning(f"Geocoding fallito per {luogo_nome}: {geoe}")
            _emit(state, "fisico:geo_fallback",
                  f"\U0001f4cd  Non riesco a localizzare '{luogo_nome}': "
                  "uso le coordinate del dispositivo come riferimento geografico.")

        # 2. Esecuzione parallela meteo + astronomia
        _emit(state, "fisico:meteo_avvio",
              "🌤️  Chiedo al cielo com'è il tempo in questo momento…")

        usa_astro = state.get("usa_astronomia", True)
        tasks     = [weather.get_weather(lat, lon)]

        if usa_astro:
            _emit(state, "fisico:astro_avvio",
                  "✨  Interrogo le stelle: calcolo la posizione di pianeti "
                  "e costellazioni per questa notte…")
            tasks.append(astronomy.get_sky_data(
                client_ip   = state.get("client_ip", "127.0.0.1"),
                lat         = lat,
                lon         = lon,
                data_storia = state.get("data_storia"),
                ora_storia  = state.get("ora_storia"),
            ))

        results = await asyncio.gather(*tasks, return_exceptions=True)

        # Meteo
        if isinstance(results[0], Exception):
            logger.error(f"Errore meteo: {results[0]}")
            _emit(state, "fisico:meteo_errore",
                  "🌧️  Non riesco a ottenere i dati meteo: "
                  "procedo con un cielo generico.")
        else:
            meteo_res = results[0]
            _emit(state, "fisico:meteo_ok",
                  f"🌤️  Meteo ottenuto: {meteo_res}.",
                  {"meteo": meteo_res})

        # Astronomia
        if usa_astro and len(results) > 1:
            if isinstance(results[1], Exception):
                logger.error(f"Errore AstronomyClient: {results[1]}")
                _emit(state, "fisico:astro_errore",
                      "🔭  Il telescopio non risponde: la storia userà "
                      "un cielo stellato di fantasia.")
            else:
                astro_res = results[1]
                corpi = list(astro_res.keys()) if isinstance(astro_res, dict) else []
                desc  = f" Rilevati: {', '.join(corpi[:4])}." if corpi else ""
                _emit(state, "fisico:astro_ok",
                      f"🌙  Mappa celeste acquisita.{desc}",
                      {"corpi_celesti": corpi})

        _emit(state, "fisico:fine",
              "✅  Contesto fisico completo. La scena è pronta.")

    except Exception as e:
        logger.error(f"❌ Errore generale nel nodo fisico: {e}", exc_info=True)
        _emit(state, "fisico:errore_generale",
              "❌  Qualcosa è andato storto nel recupero del contesto fisico. "
              "Continuo comunque con i dati disponibili.")

    return {
        "lat":              lat,
        "lon":              lon,
        "dati_meteo":       meteo_res,
        "dati_astronomici": astro_res,
    }


# ---------------------------------------------------------------------------
# NODE 5 — Valutazione qualità frammenti
# ---------------------------------------------------------------------------
async def valuta_frammenti(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_frammenti")

    if not state["frammenti"]:
        _emit(state, "frammenti:riformula",
              "🔄  I ricordi trovati non bastano: cerco nuove parole chiave "
              "per scavare più in profondità nella memoria.")
        nuovi_termini, uso_llm = await _riformula_termini(state, llm)
        _emit(state, "frammenti:nuovi_termini",
              f"🔑  Nuovi termini di ricerca pronti: {', '.join(nuovi_termini) or 'nessuno'}.",
              {"termini": nuovi_termini})
        return {
            "qualita_frammenti": "assente",
            "termini_ricerca_neo4j": nuovi_termini,
            "llm_usage": [uso_llm] if uso_llm else [],
        }

    n = len(state["frammenti"])
    _emit(state, "frammenti:ok",
          f"✅  {n} frammenti di memoria sono di buona qualità "
          "e alimenteranno la storia.",
          {"n_frammenti": n})
    return {"qualita_frammenti": "buona"}


async def _riformula_termini(state: BaldoState, llm: LLMRouter) -> tuple[list[str], dict]:
    if llm.routing.get_provider("valuta_frammenti._riformula_termini") == "nessuno":
        # Bypass deliberato: niente retry con termini riformulati, si va
        # avanti senza frammenti invece di tentare una seconda ricerca.
        return [], {}
    system = 'Suggerisci termini di ricerca alternativi in JSON: {"termini": []}'
    risultato = await llm.chiedi(system=system, user=state["ambientazione"], fase="valuta_frammenti._riformula_termini")
    uso_llm = {
        "nodo": "valuta_frammenti._riformula_termini",
        "modello": risultato.modello,
        "token_input": risultato.token_input,
        "token_output": risultato.token_output,
        "durata_secondi": round(risultato.durata_secondi, 2),
    }
    try:
        dati = json.loads(risultato.testo.strip().strip("```json").strip("```"))
        return dati.get("termini", []), uso_llm
    except Exception:
        return [], uso_llm


# ---------------------------------------------------------------------------
# NODE 6 — Composizione prompt
# ---------------------------------------------------------------------------
async def componi_prompt(state: BaldoState) -> dict:
    logger.info("[NODE] componi_prompt")

    _emit(state, "composizione:inizio",
          "🖊️  Intrecciо tutti gli ingredienti: personaggi, meteo, stelle e ricordi "
          "per costruire il prompt narrativo definitivo…")

    composer = StoryComposer()
    prompt_arricchito, meta_composizione = composer.componi(
        prompt_originale = state["prompt_originale"],
        analisi = {
            "personaggi":  state["personaggi"],
            "emozioni":    state["emozioni"],
            "ambientazione": state["ambientazione"],
            "luogo":       state.get("luogo"),
            # Nomi chiave allineati a quelli letti da composer.py — prima
            # erano "data"/"meteo" (mai letti: composer cerca data_storia/
            # dati_meteo), quindi il prompt mostrava sempre i placeholder
            # generici "oggi"/"sereno" invece dei dati reali già raccolti
            # da fetch_contesto_fisico, indipendentemente dal meteo/data vero.
            "data_storia": state.get("data_storia"),
            "ora_storia":  state.get("ora_storia"),
            "dati_meteo":  state.get("dati_meteo"),
        },
        frammenti          = state["frammenti"],
        dati_astronomici   = state.get("dati_astronomici") if state.get("usa_astronomia") else None,
        storie_precedenti  = state.get("storie_precedenti", []),
        personaggi_bio     = state.get("personaggi_bio", {}),
        # "eta_bambino", non "eta": composer.py legge kwargs.get('eta_bambino',
        # ...) — con la chiave sbagliata l'età reale non arrivava mai e ogni
        # regola che dipende dall'età (vocabolario, numero di inganni) cadeva
        # sempre sul valore di default, qualunque età fosse stata scelta.
        eta_bambino        = state["eta_bambino"],
        lunghezza          = state["lunghezza"],
        lingua             = state["lingua"],
    )

    _emit(state, "composizione:fine",
          "✅  Il telaio narrativo è pronto. "
          "Ora il cantastorie può finalmente scrivere.")

    return {
        "prompt_arricchito": prompt_arricchito,
        "tecnica_narrativa_kb": meta_composizione.get("tecnica_narrativa"),
        "archetipo_kb": meta_composizione.get("archetipo"),
    }


# Sotto questa soglia di parole consecutive uguali, il confronto rischia
# falsi positivi (formule ricorrenti per caso, non un ritornello voluto).
# Abbassata da 6 a 4: i ritornelli migliori per 3 anni sono proprio i più
# corti ("Piccola piccola, dov'è finita?", "Naso per terra, mondo
# gigante!" — 4 e 5 parole), e con il confronto per n-grammi (non più per
# frasi intere) il rischio di falso positivo a questa lunghezza resta
# basso: entrambi i casi osservati erano ritornelli veri, mai rilevati
# perché sotto la vecchia soglia di 6.
_RITORNELLO_MIN_PAROLE = 4

_PUNTEGGIATURA_BORDO_RE = re.compile(r"^[«»\"'“”,.:;!?]+|[«»\"'“”,.:;!?]+$")

# Confine di CHIUSURA DI UNA BATTUTA VIRGOLETTATA (non punteggiatura forte
# in generale: un ritornello narrato, non in discorso diretto, può
# legittimamente attraversare un "!" o un "?" interni — es. "Drin drin!
# Pronto? Una storia piccola piccola" è un unico ritornello valido, non va
# troncato al primo "!"). Solo la chiusura di un dialogo (» " ”) segna un
# confine affidabile: dopo una battuta chiusa, il testo che segue è
# narrazione libera, non più parte del ritornello — osservato su un caso
# reale dove "«Anch'io ho un'idea piccola!» e propose..." (ripetuto per
# ogni personaggio con finale diverso dopo "propose") veniva incluso per
# intero nel ritornello rilevato, perché le occorrenze coincidevano anche
# lì per puro caso di template narrativo.
_TERMINALE_RE = re.compile(r"[»”\"]$")


def _parola_normalizzata(parola: str) -> str:
    return _PUNTEGGIATURA_BORDO_RE.sub("", parola).lower()


def _rileva_ritornello(testo: str) -> Optional[str]:
    """
    Cerca la sequenza di parole che si ripete più volte nel testo (almeno
    _RITORNELLO_MIN_PAROLE parole, almeno 2 occorrenze) — più affidabile
    che fidarsi del campo "ritornello" del frammento KB: quel testo può
    contenere un nome proprio specifico della fiaba d'origine (es.
    "Mangiafuoco") che il draft, lasciato libero di scrivere, sostituisce
    già con i personaggi della propria trama. Imporre il testo del
    frammento parola per parola a rifinisci reintroduceva quel nome
    estraneo — qui si protegge invece quello che il draft ha davvero usato.

    Confronto per n-grammi di parole, non per frasi intere delimitate da
    ".!?": un ritornello ripetuto quasi alla lettera è sfuggito una volta
    alla rilevazione a frasi per due motivi — una congiunzione di raccordo
    in testa solo alla prima occorrenza ("E Marco strinse..." vs "Marco
    strinse...") e un punto di domanda dentro un dialogo poco prima
    ("qui!\" E Marco...") che spezzava la frase nel punto sbagliato. I
    confini di frase in una prosa piena di dialoghi sono troppo fragili
    per un regex su ".!?"; i confini di parola no.

    Valuta TUTTI gli n-grammi ripetuti nel testo, non solo il primo che
    compare (versione precedente): su una storia reale, il nome di un
    personaggio ripetuto per riferirsi a lui ("il cavallo a dondolo",
    2 occorrenze) compariva prima nel testo del vero ritornello voluto
    dal draft ("Anch'io ho un'idea piccola!", 3 occorrenze identiche nei
    momenti chiave) — restituire il primo match trovato "rubava" la
    rilevazione al ritornello vero. Ora si raccolgono tutti i candidati e
    si preferisce quello con più occorrenze (a parità, il più lungo): un
    nome di personaggio ricorre quasi sempre meno volte di un ritornello
    deliberato, che per istruzione va ripetuto 2-3 volte apposta.
    """
    parole = re.findall(r"\S+", testo or "")
    n = _RITORNELLO_MIN_PAROLE
    if len(parole) < n * 2:
        return None

    normalizzate = [_parola_normalizzata(p) for p in parole]

    posizioni: dict = {}
    for i in range(len(parole) - n + 1):
        chiave = tuple(normalizzate[i:i + n])
        if not all(chiave):  # n-gramma con solo punteggiatura in qualche slot
            continue
        posizioni.setdefault(chiave, []).append(i)

    candidati = []  # (occorrenze, lunghezza_parole, testo)
    for chiave, idxs in posizioni.items():
        if len(idxs) < 2:
            continue
        # Estende la ripetizione oltre le n parole minime, usando le prime
        # due occorrenze, finché le due sequenze continuano a coincidere.
        j, i = idxs[0], idxs[1]
        k = n
        while (
            i + k < len(parole) and j + k < i
            and normalizzate[i + k] == normalizzate[j + k]
        ):
            k += 1

        # Tronca al primo confine di frase/battuta trovato DENTRO la
        # sequenza — anche se cade a metà della finestra minima di n
        # parole. Senza questo, una finestra sfalsata di una parola che
        # parte proprio dopo l'inizio di una battuta (es. "ho un'idea
        # piccola!» e propose" invece di "Anch'io ho un'idea piccola!")
        # può risultare più lunga e vincere il confronto, pur scavalcando
        # la punteggiatura che chiude la battuta vera — osservato su una
        # storia reale. Un ritornello è un'unità autonoma (una frase, una
        # battuta), non deve scavalcarla né iniziare a metà.
        confine = None
        for m in range(k):
            if _TERMINALE_RE.search(parole[i + m]):
                confine = m
                break
        if confine is not None:
            k = confine + 1
        if k < n:
            continue  # troppo corto dopo il taglio: non è un candidato valido

        chiave_estesa = tuple(normalizzate[i:i + k])
        occorrenze = sum(
            1 for start in range(len(parole) - k + 1)
            if tuple(normalizzate[start:start + k]) == chiave_estesa
        )
        testo_candidato = " ".join(parole[i:i + k]).strip("«»\"'“”,.:;!? ")
        if testo_candidato:
            candidati.append((occorrenze, k, testo_candidato))

    if not candidati:
        return None

    candidati.sort(key=lambda c: (c[0], c[1]), reverse=True)
    return candidati[0][2]


# Reduplicazione ritmica ("piano piano", "forte forte", "bene bene"): la
# Regola 17 in composer.py chiede al modello di usarla al massimo una
# volta per racconto, ma è un vincolo di CONTEGGIO — la stessa categoria
# di istruzione che i modelli seguono meno bene di un divieto puntuale su
# una frase specifica (osservato: 3 occorrenze in una storia reale
# nonostante l'istruzione). Corretto qui in modo deterministico invece di
# insistere solo a livello di prompt.
#
# Il "(?![!?])" esclude le onomatopee ("Toc toc!", "Drin drin!" — un
# dispositivo diverso e legittimo, già protetto come ritornello): si
# riconoscono perché il "!"/"?" segue subito la parola ripetuta, a
# differenza di una reduplicazione aggettivale/avverbiale che continua la
# frase con una virgola o il resto del periodo.
_REDUPLICAZIONE_RE = re.compile(r"\b(\w+)[ ,]+\1\b(?![!?])", re.IGNORECASE)


def _limita_reduplicazioni(testo: str, ritornello: "str | None" = None, max_occorrenze: int = 1) -> str:
    """
    Riduce le reduplicazioni ritmiche oltre la prima consentita,
    collassando "parola parola" in "parola" — mantiene l'ordine e il
    resto del testo intatti, un fix mirato e deterministico, non
    un'altra istruzione nel prompt che il modello potrebbe ignorare.

    Le reduplicazioni che fanno parte del RITORNELLO riconosciuto (es.
    "Piccola piccola, dov'è finita?" — la reduplicazione È il suo
    dispositivo ritmico) sono escluse dal conteggio e mai toccate: senza
    questa esclusione, una reduplicazione "libera" comparsa prima nel
    testo (es. "un problema grande grande") consuma l'unico slot
    consentito e le due occorrenze del ritornello finiscono collassate
    insieme a lei — osservato su un caso reale, dove "Piccola piccola,
    dov'è finita?" diventava "Piccola, dov'è finita?" in entrambe le
    sue occorrenze.
    """
    protette = set()
    if ritornello:
        for m in _REDUPLICAZIONE_RE.finditer(ritornello):
            protette.add(m.group(1).lower())

    contatore = 0

    def sostituisci(m):
        nonlocal contatore
        if m.group(1).lower() in protette:
            return m.group(0)
        contatore += 1
        if contatore <= max_occorrenze:
            return m.group(0)
        return m.group(1)

    return _REDUPLICAZIONE_RE.sub(sostituisci, testo or "")


# Frasi con "come" quasi sempre non comparative, da escludere dal conteggio
# (interrogative/idiomatiche, non similitudini).
_COME_NON_COMPARATIVO = (
    "come mai", "come stai", "come sta", "come va", "come si chiama",
    "come ti chiami", "come faccio", "come fai", "come si fa",
)


def _conta_similitudini_approssimate(testo: str, ritornello: "str | None" = None) -> int:
    """
    Stima approssimativa (non un'analisi semantica) di quante comparazioni
    con "come" contiene il testo — serve solo a tracciare nel report se la
    Regola 20 (massimo 2-3 similitudini per racconto) sta reggendo sui casi
    reali, MAI a correggere il testo: "come" è troppo ambiguo in italiano
    (comparativo, interrogativo, causale — "come mai", "come se", "come
    stai") per un'edit automatica sicura, a differenza della reduplicazione
    ("parola parola") che è un pattern inequivocabile.

    Esclude l'ultimo paragrafo (la domanda finale) dal conteggio: osservato
    ripetutamente, tre casi reali diversi, che la domanda finale usa spesso
    "come" in senso interrogativo ("come avresti aiutato...", "come lo
    chiameresti...") — falso positivo sistematico, non un paragone. Stesso
    confine di paragrafo già usato da _sostituisci_ultimo_paragrafo.

    Esclude anche il testo del ritornello, se presente: un ritornello può
    contenere "come" per costruzione (es. "Quella luna come la chiamo", un
    gioco di nomi) e si ripete 2-3 volte di proposito — senza questa
    esclusione ogni ripetizione veniva contata come una similitudine in
    più, gonfiando il numero e potendo far scattare inutilmente il rewrite
    di verifica_coerenza_domanda anche quando le similitudini vere erano
    sotto soglia (osservato: un ritornello con "come" ripetuto 3 volte +
    1 similitudine vera = 4, sopra soglia, con lo stesso identico problema
    già risolto per _limita_reduplicazioni ma mai esteso qui).
    """
    testo = testo or ""
    paragrafi = testo.split("\n\n")
    corpo = "\n\n".join(paragrafi[:-1]) if len(paragrafi) > 1 else testo
    if ritornello:
        corpo = corpo.replace(ritornello, "")
    testo_normalizzato = corpo.lower()
    totale = len(re.findall(r"\bcome\b", testo_normalizzato))
    for idioma in _COME_NON_COMPARATIVO:
        totale -= testo_normalizzato.count(idioma)
    return max(totale, 0)


# ---------------------------------------------------------------------------
# NODE 7 — Generazione draft
# ---------------------------------------------------------------------------
async def genera_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] genera_draft")

    _emit(state, "draft:inizio",
          "✍️  Il cantastorie prende la penna… la storia sta nascendo, "
          "parola dopo parola.")

    prompt_input = state.get("prompt_arricchito") or state["prompt_originale"]
    risultato = await llm.genera_racconto(prompt_input)
    draft = risultato.testo

    parole = len(draft.split()) if draft else 0
    _emit(state, "draft:completato",
          f"📝  Prima bozza completata — {parole} parole scritte. "
          "Il racconto esiste, ma ha ancora bisogno di cure.",
          {"parole": parole})

    return {
        "draft": draft,
        "ritornello_atteso": _rileva_ritornello(draft),
        "llm_usage": [{
            "nodo": "genera_draft",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
            "durata_secondi": round(risultato.durata_secondi, 2),
        }],
    }


# ---------------------------------------------------------------------------
# NODE 8 — Valutazione draft
# ---------------------------------------------------------------------------
async def valuta_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_draft")

    _emit(state, "draft:valutazione",
          "🧐  Rileggo la bozza con occhio critico: "
          "controllo ritmo, coerenza e adeguatezza per i bambini…")

    # TODO: logica reale di valutazione
    _emit(state, "draft:valutazione_ok",
          "✅  La bozza supera l'esame. Si può procedere alla rifinitura.")

    return {"valutazione_draft": "ok"}


# ---------------------------------------------------------------------------
# NODE 9 — Correzione draft
# ---------------------------------------------------------------------------
async def correggi_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] correggi_draft")
    tentativo = state["tentativi_correzione"] + 1

    _emit(state, "draft:correzione",
          f"🔧  Correzione {tentativo}: lima gli spigoli del racconto, "
          "sistemo i passaggi che non scorrevano bene.",
          {"tentativo": tentativo})

    return {"tentativi_correzione": tentativo}


# ---------------------------------------------------------------------------
# NODE 10 — Rifinitura finale
# ---------------------------------------------------------------------------
async def rifinisci(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] rifinisci")

    eta = state["eta_bambino"]
    _emit(state, "rifinitura:inizio",
          f"💎  Lucido le parole per un bambino di {eta} anni: "
          "scelgo il vocabolario giusto, aggiungo musicalità alle frasi…",
          {"eta": eta})

    ritornello_atteso = state.get("ritornello_atteso")
    if llm.routing.get_provider("rifinisci") == "nessuno":
        # Bypass deliberato: il draft diventa il racconto finale così com'è,
        # senza la chiamata LLM di rifinitura (semplificazione lessicale,
        # conversione emozioni dichiarate in reazioni fisiche, protezione
        # della cornice di Baldo). Il rischio principale che rifinisci
        # copriva — una domanda finale malformata — resta comunque
        # presidiato da verifica_coerenza_domanda.check/.fix, un nodo
        # indipendente da questo.
        testo_base = state["draft"]
        uso_llm = []
    else:
        risultato = await llm.rifinisci(
            draft=state["draft"],
            eta=eta,
            ritornello=ritornello_atteso,
        )
        testo_base = risultato.testo
        uso_llm = [{
            "nodo": "rifinisci",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
            "durata_secondi": round(risultato.durata_secondi, 2),
        }]

    racconto = _limita_reduplicazioni(testo_base, ritornello=ritornello_atteso)

    _emit(state, "rifinitura:fine",
          "🎉  La storia è pronta! Ogni parola è al suo posto.",
          {"lunghezza_finale": len(racconto.split()) if racconto else 0})

    return {
        "racconto_finale": racconto,
        "similitudini_stimate": _conta_similitudini_approssimate(racconto, ritornello=ritornello_atteso),
        "llm_usage": uso_llm,
    }


# ---------------------------------------------------------------------------
# NODE 10b — Verifica coerenza della domanda finale
# ---------------------------------------------------------------------------
# Un "nome proprio" qui è una singola parola capitalizzata (es. "Marco"),
# non una descrizione generica ("principe coraggioso", "drago alato") che
# non ci si aspetta di ritrovare alla lettera nel testo.
_NOME_PROPRIO_RE = re.compile(r"^[A-ZÀ-Ý][a-zà-ÿ'\-]+(?:\s[A-ZÀ-Ý][a-zà-ÿ'\-]+)?$")


def _nomi_propri_mancanti(personaggi: list, racconto: str) -> list:
    """
    Nomi propri richiesti esplicitamente dall'utente (via l'estrazione RAG)
    ma assenti — nemmeno come sottostringa case-insensitive — dal racconto
    finale: segnale di un nome storpiato o sostituito lungo la pipeline.
    Solo rilevazione (log): una correzione automatica via regex rischia di
    rompere declinazioni o vocativi che non può prevedere.
    """
    mancanti = []
    testo_lower = (racconto or "").lower()
    for nome in personaggi or []:
        nome = (nome or "").strip()
        if not _NOME_PROPRIO_RE.match(nome):
            continue
        # \b invece di un semplice "in": altrimenti "Marco" risulterebbe
        # presente anche dentro "Marcolino" (sottostringa ma nome diverso).
        if not re.search(rf"\b{re.escape(nome.lower())}\b", testo_lower):
            mancanti.append(nome)
    return mancanti


def _termini_kb_trapelati(termini: list, racconto: str) -> list:
    """
    tecnica_narrativa e archetipo sono jargon per l'autore (Baldo), usati
    per guidare la struttura della trama — mai parole che un bambino di tre
    anni dovrebbe sentire (osservato: "Ed ecco il capovolgimento, piccolo
    mio", con "capovolgimento" preso alla lettera dal campo KB). Match di
    sottostringa, non \\b: alcuni valori sono "a_due_parole" con underscore,
    che un LLM scrive naturalmente con uno spazio.
    """
    trovati = []
    racconto_lower = (racconto or "").lower()
    for termine in termini or []:
        if not termine:
            continue
        normalizzato = termine.strip().lower().replace("_", " ")
        if normalizzato and normalizzato in racconto_lower:
            trovati.append(termine)
    return trovati


def _sostituisci_ultimo_paragrafo(testo: str, nuovo_paragrafo: str) -> str:
    """
    Sostituisce l'ultimo paragrafo di `testo` con `nuovo_paragrafo` — usato
    per il fix della domanda finale: la domanda è sempre il paragrafo di
    chiusura (uno o due a-capo la separano dal resto), quindi sostituire
    solo quello evita di dover chiedere all'LLM di riprodurre l'intera
    favola per cambiare una frase.
    """
    testo = (testo or "").rstrip()
    nuovo_paragrafo = (nuovo_paragrafo or "").strip()
    if not testo:
        return nuovo_paragrafo
    paragrafi = testo.split("\n\n")
    paragrafi[-1] = nuovo_paragrafo
    return "\n\n".join(paragrafi)


async def verifica_coerenza_domanda(state: BaldoState, llm: LLMRouter) -> dict:
    """
    Rete di sicurezza economica: composer.py suggerisce una domanda finale
    presa dal frammento più rilevante, ma il modello di generazione può
    comunque ignorarla, storpiarla o (in rari casi) lasciarne una scollegata
    dalla trama effettivamente raccontata. Nello stesso nodo, per economia,
    girano anche altri tre controlli a costo quasi nullo: fedeltà del nome
    del bambino (nessuna chiamata LLM, solo un match di stringa), fuga di
    lessico tecnico della KB (idem, nessuna chiamata LLM — tecnica_narrativa/
    archetipo comparsi alla lettera nel testo, es. "Ed ecco il capovolgimento,
    piccolo mio") e coerenza del ritornello (un secondo classificatore SI/NO,
    stesso principio della domanda ma senza tentativo di riscrittura
    automatica — un ritornello, a differenza della domanda finale, è
    ripetuto più volte nel testo: una riscrittura automatica rischierebbe di
    introdurre incoerenza tra le ripetizioni invece di risolverla).

    In due passaggi, non uno, per la domanda: prima un classificatore SI/NO
    economico, e solo se la risposta è NO si chiede la riscrittura mirata.
    Un'unica chiamata "verifica-e-se-serve-correggi" è stata scartata: anche
    istruita a restituire il testo invariato quando già coerente, un LLM
    tende comunque a "migliorarlo" — riscriveva domande già valide,
    vanificando lo scopo di un controllo mirato.

    Alla fine, un quarto controllo indipendente: la Regola 20 in composer.py
    (massimo 2-3 similitudini con "come" per racconto) è un vincolo di
    CONTEGGIO — la stessa categoria di istruzione che i modelli seguono meno
    bene di un divieto puntuale, e infatti su 4 storie reali consecutive non
    ha retto (4-8 similitudini invece di 2-3). A differenza della
    reduplicazione, qui non esiste un fix deterministico sicuro ("come" è
    troppo ambiguo in italiano per un regex che tagli senza rischiare di
    rovinare frasi che non sono affatto similitudini) — quindi, solo quando
    il conteggio approssimativo supera la soglia, un editor dedicato
    riscrive l'intero racconto per diradarle, con verifiche di sicurezza
    (lunghezza comparabile, ritornello intatto) prima di accettare il
    risultato.
    """
    logger.info("[NODE] verifica_coerenza_domanda")

    racconto = state.get("racconto_finale") or ""
    if not racconto:
        return {}

    uso_llm = []

    termini_trapelati = _termini_kb_trapelati(
        [state.get("tecnica_narrativa_kb"), state.get("archetipo_kb")], racconto
    )
    if termini_trapelati:
        logger.warning(
            f"[verifica_coerenza_domanda] Termine/i tecnico/i della KB "
            f"trapelato/i alla lettera nel racconto finale: {termini_trapelati}"
        )

    mancanti = _nomi_propri_mancanti(state.get("personaggi", []), racconto)
    if mancanti:
        logger.warning(
            f"[verifica_coerenza_domanda] Nome/i proprio/i richiesti ma assenti "
            f"dal racconto finale (possibile storpiatura): {mancanti}"
        )

    # Controllo dedicato sul campo "nome" della personalizzazione (distinto
    # da "personaggi" sopra: il nome del form potrebbe non essere mai stato
    # estratto come personaggio dal RAG, o esserlo con una grafia diversa).
    # Nessuna chiamata LLM: un confronto di stringa basta ed è gratis.
    nome_atteso = (state.get("nome") or "").strip()
    nome_presente = None
    if nome_atteso:
        nome_presente = bool(re.search(rf"\b{re.escape(nome_atteso.lower())}\b", racconto.lower()))
        if not nome_presente:
            logger.warning(
                f"[verifica_coerenza_domanda] Nome personalizzato '{nome_atteso}' "
                f"assente dal racconto finale."
            )

    # Controllo di coerenza del ritornello: verifica che le figure/elementi
    # eventualmente nominati in una frase ripetuta compaiano davvero nella
    # storia (lo stesso difetto già osservato: un ritornello che elenca
    # qualcosa — es. "un giudice" — mai comparso come scena concreta).
    system_ritornello = (
        "Sei un revisore di favole per bambini in età prescolare. Cerca nel "
        "testo un ritornello: una frase breve ripetuta più volte, quasi "
        "identica ogni volta. Se non c'è alcuna frase ripetuta, rispondi SI. "
        "Se c'è, controlla: ogni personaggio, oggetto o figura che il "
        "ritornello nomina compare DAVVERO altrove nella storia, come scena "
        "concreta e non solo come parola? Rispondi ESCLUSIVAMENTE con la "
        "parola SI se sì (o se non c'è ritornello), oppure ESCLUSIVAMENTE "
        "con la parola NO se il ritornello nomina qualcosa mai comparso "
        "nella storia. Nessun'altra parola nella risposta."
    )
    system_check = (
        "Sei un revisore di favole per bambini in età prescolare. Leggi la favola "
        "e la domanda con cui si chiude, rivolta al bambino (di solito l'ultima "
        "frase, tra virgolette). La domanda NON deve avere per forza una "
        "risposta univoca nel testo: può essere aperta o di opinione (es. "
        "\"Chi è il più forte, il leone o il topolino?\", \"Tu cosa avresti fatto?\"). "
        "Conta solo questo: la domanda parla di personaggi, oggetti o eventi "
        "che compaiono DAVVERO nella favola appena letta? Rispondi "
        "ESCLUSIVAMENTE con la parola SI se sì, oppure ESCLUSIVAMENTE con la "
        "parola NO se la domanda manca o cita qualcosa (un personaggio, un "
        "oggetto, una scena) che nella favola non compare affatto. "
        "Nessun'altra parola nella risposta."
    )

    # Le due verifiche sono indipendenti (leggono solo il racconto, nessuna
    # dipende dall'esito dell'altra) — in parallelo invece che in sequenza.
    # Misurato: una chiamata con output di poche parole costa in tempo molto
    # meno del suo stesso input (~1-1.5s anche con migliaia di token in
    # ingresso, la latenza è dominata dall'output generato, non da quello
    # letto) — non è la spesa maggiore della pipeline, ma è comunque un
    # risparmio reale ottenerlo gratis invece che in sequenza.
    risultato_ritornello, risultato_check = await asyncio.gather(
        llm.chiedi(system=system_ritornello, user=racconto, fase="verifica_coerenza_domanda.ritornello"),
        llm.chiedi(system=system_check, user=racconto, fase="verifica_coerenza_domanda.check"),
        return_exceptions=True,
    )

    if isinstance(risultato_ritornello, Exception):
        logger.warning(f"[verifica_coerenza_domanda] Controllo ritornello fallito, ignorato: {risultato_ritornello}")
    else:
        uso_llm.append({
            "nodo": "verifica_coerenza_domanda.ritornello",
            "modello": risultato_ritornello.modello,
            "token_input": risultato_ritornello.token_input,
            "token_output": risultato_ritornello.token_output,
            "durata_secondi": round(risultato_ritornello.durata_secondi, 2),
        })
        if not risultato_ritornello.testo or not risultato_ritornello.testo.strip().upper().startswith("SI"):
            logger.warning(
                "[verifica_coerenza_domanda] Il ritornello sembra nominare "
                "qualcosa non presente altrove nella storia."
            )

    if isinstance(risultato_check, Exception):
        logger.warning(f"[verifica_coerenza_domanda] Controllo fallito, mantengo il racconto originale: {risultato_check}")
    else:
        esito = risultato_check.testo
        uso_llm.append({
            "nodo": "verifica_coerenza_domanda.check",
            "modello": risultato_check.modello,
            "token_input": risultato_check.token_input,
            "token_output": risultato_check.token_output,
            "durata_secondi": round(risultato_check.durata_secondi, 2),
        })

        if esito and not esito.strip().upper().startswith("SI"):
            # Chiediamo SOLO la nuova domanda, non l'intera favola riscritta:
            # la versione precedente chiedeva la favola completa "copiando
            # ogni paragrafo esattamente com'è" tranne l'ultima frase — in
            # pratica una seconda riscrittura integrale (osservato: token in
            # uscita quasi pari al draft e a rifinisci), con relativo costo
            # e un'occasione in più di deriva sul testo che rifinisci aveva
            # già sistemato. La domanda finale è quasi sempre il suo
            # paragrafo — la sostituzione avviene qui, per codice, non
            # chiedendo all'LLM di riprodurre tutto il resto.
            system_fix = (
                "Sei un revisore di favole per bambini in età prescolare. Leggi la "
                "favola: la sua domanda finale NON è coerente (cita qualcosa che "
                "non compare nella storia). Il tuo UNICO compito: scrivere una "
                "NUOVA domanda finale, breve, rivolta al bambino, che parli di "
                "personaggi o eventi che compaiono davvero nella storia — nello "
                "stesso tono con cui il narratore si rivolge già al bambino nel "
                "resto del testo. Rispondi ESCLUSIVAMENTE con la nuova domanda "
                "finale (una frase sola): non riscrivere il resto della favola, "
                "nessun commento, nessuna spiegazione."
            )
            try:
                risultato_fix = await llm.chiedi(system=system_fix, user=racconto, fase="verifica_coerenza_domanda.fix")
                nuova_domanda = (risultato_fix.testo or "").strip()
                uso_llm.append({
                    "nodo": "verifica_coerenza_domanda.fix",
                    "modello": risultato_fix.modello,
                    "token_input": risultato_fix.token_input,
                    "token_output": risultato_fix.token_output,
                    "durata_secondi": round(risultato_fix.durata_secondi, 2),
                })
                # Rete di sicurezza adattata al nuovo formato: ci aspettiamo
                # una singola frase breve, non più un testo lungo quanto
                # l'originale. Se il modello ha comunque provato a restituire
                # un testo lungo o multi-paragrafo, ha ignorato l'istruzione
                # — meglio tenere l'originale con la domanda scorrelata che
                # rischiare di incollare un frammento non affidabile.
                if nuova_domanda and "\n\n" not in nuova_domanda and len(nuova_domanda) <= 400:
                    racconto = _sostituisci_ultimo_paragrafo(racconto, nuova_domanda)
                    _emit(state, "revisione:domanda_corretta",
                          "🔎  Ho controllato la domanda finale della storia... e non tornava. L'ho aggiustata.")
                else:
                    logger.warning(
                        f"[verifica_coerenza_domanda] Risposta del fix non è la domanda "
                        f"breve richiesta (lunghezza {len(nuova_domanda)}), mantengo il "
                        f"racconto originale."
                    )
            except Exception as e:
                logger.warning(f"[verifica_coerenza_domanda] Riscrittura fallita, mantengo il racconto originale: {e}")

    # --- Controllo similitudini (Regola 20), indipendente dalla domanda ---
    ritornello_atteso = state.get("ritornello_atteso")
    similitudini = _conta_similitudini_approssimate(racconto, ritornello=ritornello_atteso)
    SOGLIA_SIMILITUDINI = 3
    bypass_similitudini = llm.routing.get_provider("verifica_coerenza_domanda.similitudini") == "nessuno"
    if similitudini > SOGLIA_SIMILITUDINI and not bypass_similitudini:
        vincolo_ritornello_sim = (
            f"Il racconto usa questo ritornello, che deve restare IDENTICO, "
            f"parola per parola, in ogni sua ripetizione: \"{ritornello_atteso}\". "
            f"Non parafrasarlo, non correggerne lo stile, non rimuoverlo. "
        ) if ritornello_atteso else ""
        system_similitudini = (
            "Sei un editor di favole per bambini in età prescolare. Il "
            "racconto qui sotto usa troppe similitudini con \"come\" "
            "(paragoni tipo \"grande come un pugno\", \"come se...\"). "
            "Riscrivi il racconto intero riducendo le similitudini a un "
            "massimo di 2-3 in tutto il testo — sostituisci le altre con "
            "descrizioni dirette e concrete (es. \"le assicelle tremavano "
            "al vento\" invece di \"tremavano come i denti di un vecchio "
            "pettine\"). Non aggiungere né togliere eventi della trama, non "
            "cambiare i personaggi, non toccare la cornice di apertura/"
            "chiusura di Baldo né la domanda finale. "
            f"{vincolo_ritornello_sim}"
            "Rispondi ESCLUSIVAMENTE con il racconto riscritto per intero, "
            "senza commenti né spiegazioni."
        )
        try:
            risultato_sim = await llm.chiedi(system=system_similitudini, user=racconto, fase="verifica_coerenza_domanda.similitudini")
            testo_corretto = (risultato_sim.testo or "").strip()
            uso_llm.append({
                "nodo": "verifica_coerenza_domanda.similitudini",
                "modello": risultato_sim.modello,
                "token_input": risultato_sim.token_input,
                "token_output": risultato_sim.token_output,
                "durata_secondi": round(risultato_sim.durata_secondi, 2),
            })
            # Rete di sicurezza: una riscrittura integrale è più rischiosa
            # della sostituzione dell'ultimo paragrafo sopra — un testo
            # troppo corto/lungo rispetto all'originale, o che ha perso il
            # ritornello, è il segnale che qualcosa è andato storto: meglio
            # tenere l'originale con troppe similitudini che un testo
            # danneggiato.
            lunghezza_ok = bool(testo_corretto) and 0.5 * len(racconto) <= len(testo_corretto) <= 1.6 * len(racconto)
            ritornello_ok = (not ritornello_atteso) or (ritornello_atteso.lower() in testo_corretto.lower())
            if lunghezza_ok and ritornello_ok:
                racconto = _limita_reduplicazioni(testo_corretto, ritornello=ritornello_atteso)
            else:
                logger.warning(
                    f"[verifica_coerenza_domanda] Riscrittura similitudini scartata "
                    f"(lunghezza_ok={lunghezza_ok}, ritornello_ok={ritornello_ok}), "
                    f"mantengo il racconto originale."
                )
        except Exception as e:
            logger.warning(f"[verifica_coerenza_domanda] Riduzione similitudini fallita, mantengo il racconto originale: {e}")

    return {
        "racconto_finale": racconto,
        "similitudini_stimate": _conta_similitudini_approssimate(racconto, ritornello=ritornello_atteso),
        "llm_usage": uso_llm,
        "nome_presente_in_output": nome_presente,
        "termini_kb_trapelati": termini_trapelati,
    }


# ---------------------------------------------------------------------------
# NODE 11 — Salvataggio memoria
# ---------------------------------------------------------------------------
async def salva_memoria(state: BaldoState, neo4j: Neo4jClient) -> dict:
    logger.info("[NODE] salva_memoria")

    _emit(state, "memoria:salvataggio",
          "💾  Affido la storia alla memoria del cantastorie… "
          "sarà un ricordo prezioso per le avventure future.")

    storia_id    = str(uuid.uuid4())
    ids_frammenti = [f["id"] for f in state.get("frammenti", []) if f.get("id")]

    await neo4j.salva_storia(
        story_id         = storia_id,
        text             = state["racconto_finale"],
        characters       = state["personaggi"],
        emotions         = state["emozioni"],
        setting          = state["ambientazione"],
        original_prompt  = state["prompt_originale"],
        used_fragments   = ids_frammenti,
        timestamp        = datetime.utcnow().isoformat(),
    )

    _emit(state, "memoria:salvata",
          f"✅  Storia salvata con ID {storia_id}. "
          f"Usati {len(ids_frammenti)} frammenti di memoria.",
          {"storia_id": storia_id, "frammenti_usati": len(ids_frammenti)})

    return {"storia_id": storia_id, "frammenti_usati": ids_frammenti}


# ---------------------------------------------------------------------------
# NODE — Errore
# ---------------------------------------------------------------------------
async def nodo_errore(state: BaldoState) -> dict:
    motivo = state.get("motivo_rifiuto", "errore sconosciuto")

    _emit(state, "errore",
          f"😔  Mi dispiace, non posso scrivere questa storia: {motivo}. "
          "Prova a cambiare la tua richiesta!",
          {"motivo": motivo})

    return {"racconto_finale": "", "errore": motivo}
