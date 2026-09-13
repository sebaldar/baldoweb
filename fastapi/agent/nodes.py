"""
agent/nodes.py
==============
Tutti i nodi del grafo LangGraph. 
Gestisce l'analisi, il recupero di dati fisici (Geo/Meteo/Astro),
la memoria Neo4j e la composizione narrativa.
"""

import json
import logging
import re
import uuid
import asyncio
from datetime import datetime
from zoneinfo import ZoneInfo

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
# NODE 1 — Analisi del prompt
# ---------------------------------------------------------------------------
async def analizza_prompt(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] analizza_prompt")
    rag = RAGExtractor(llm=llm)
    
    # Estrazione entità: personaggi, emozioni, ambientazione, luogo, data
    analisi = await rag.analizza(state["prompt_originale"])

    # Gestione Fallback Temporale (Data e Ora attuali se non specificate)
    data_target = analisi.get("data_storia") or datetime.now(FUSO_UTENZA).strftime("%d-%m-%Y")
    ora_target = analisi.get("ora_storia") or datetime.now(FUSO_UTENZA).strftime("%H:%M:%S")
    
    # Gestione Fallback Geografico (Default: Roma)
    luogo_target = analisi.get("luogo") or "Roma"

    termini = analisi.get("personaggi", []) + analisi.get("emozioni", [])
    uso_llm = analisi.get("_uso_llm")

    return {
        "personaggi": analisi.get("personaggi", []),
        "emozioni": analisi.get("emozioni", []),
        "ambientazione": analisi.get("ambientazione", ""),
        "luogo": luogo_target,
        "data_storia": data_target,
        "ora_storia": ora_target,
        "termini_ricerca_neo4j": termini,
        "tentativi_neo4j": 0,
        "tentativi_correzione": 0,
        "iterazioni_totali": 1,
        "llm_usage": [uso_llm] if uso_llm else [],
    }

# ---------------------------------------------------------------------------
# NODE 2 — Valutazione prompt
# ---------------------------------------------------------------------------
async def valuta_prompt(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_prompt")
    system = """Sei un supervisore di storie per bambini. Valuta se il prompt è adeguato.
    Rispondi SOLO JSON: {"chiaro": true/false, "motivo": "..."}"""
    
    risultato = await llm.chiedi(
        system=system,
        user=f"Prompt: {state['prompt_originale']}\nPersonaggi: {state['personaggi']}"
    )
    uso_llm = {
        "nodo": "valuta_prompt",
        "modello": risultato.modello,
        "token_input": risultato.token_input,
        "token_output": risultato.token_output,
    }
    try:
        dati = json.loads(risultato.testo.strip().strip("```json").strip("```"))
        return {
            "prompt_chiaro": dati.get("chiaro", True),
            "motivo_rifiuto": dati.get("motivo"),
            "llm_usage": [uso_llm],
        }
    except Exception:
        return {"prompt_chiaro": True, "motivo_rifiuto": None, "llm_usage": [uso_llm]}

# ---------------------------------------------------------------------------
# NODE 3 — Decisione tool
# ---------------------------------------------------------------------------
async def decide_tools(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] decide_tools")
    system = 'Decidi se usare l\'astronomia. Rispondi SOLO JSON: {"usa_astronomia": true/false}'
    risultato = await llm.chiedi(system=system, user=state['prompt_originale'])
    uso_llm = {
        "nodo": "decide_tools",
        "modello": risultato.modello,
        "token_input": risultato.token_input,
        "token_output": risultato.token_output,
    }
    try:
        dati = json.loads(risultato.testo.strip().strip("```json").strip("```"))
        return {"usa_astronomia": dati.get("usa_astronomia", True), "llm_usage": [uso_llm]}
    except:
        return {"usa_astronomia": True, "llm_usage": [uso_llm]}

# ---------------------------------------------------------------------------
# NODE 4a — Query Neo4j
# ---------------------------------------------------------------------------
async def query_neo4j(state: BaldoState, neo4j: Neo4jClient) -> dict:
    logger.info(f"[NODE] query_neo4j (tentativo {state['tentativi_neo4j'] + 1})")

    frammenti = await neo4j.cerca_frammenti(
        characters=state["personaggi"],
        emotions=state["emozioni"],
        setting=state["ambientazione"],
        extra_terms=state.get("termini_ricerca_neo4j", []),
        eta_bambino=state.get("eta_bambino"),
    )

    storie_precedenti = await neo4j.cerca_storie_precedenti(
        characters=state["personaggi"],
        emotions=state["emozioni"],
    )

    # Descrizione/tratti dei personaggi coinvolti (se qualcuno li ha compilati
    # a mano dal pannello Personaggi), per mantenerli coerenti tra le storie.
    personaggi_bio = await neo4j.get_character_bios(state["personaggi"])

    return {
        "frammenti": frammenti,
        "storie_precedenti": storie_precedenti,
        "personaggi_bio": personaggi_bio,
        "tentativi_neo4j": state["tentativi_neo4j"] + 1,
    }

# ---------------------------------------------------------------------------
# NODE 4b — Recupero dati fisici (Geo + Meteo + Astro) - CORRETTO
# ---------------------------------------------------------------------------
async def fetch_contesto_fisico(
    state: BaldoState, 
    astronomy: AstronomyClient, 
    weather: WeatherClient,
    geo: GeoService
) -> dict:
    luogo_nome = state.get("luogo") or "Roma"
    logger.info(f"[NODE] fetch_contesto_fisico avviato per: {luogo_nome}")

    lat, lon = 41.8928, 12.4964  # Default Roma
    meteo_res, astro_res = "cielo sereno", {}

    try:
        # 1. Geocoding (Verifica il nome del metodo nel tuo GeoService!)
        # Se il tuo servizio usa get_coords invece di get_coordinates, correggi qui:
        try:
            coords = await geo.get_coordinates(luogo_nome)
            if coords:
                lat, lon = coords
                logger.info(f"📍 Coordinate ottenute: {lat}, {lon}")
        except Exception as geoe:
            logger.warning(f"Geocoding fallito per {luogo_nome}, uso Roma. Errore: {geoe}")

        # 2. Esecuzione parallela
        # Creiamo i task solo se usa_astronomia è True
        tasks = [weather.get_weather(lat, lon)]
        
        usa_astro = state.get("usa_astronomia", True)
        if usa_astro:
            logger.info(f"🔭 Preparazione chiamata Astronomy per {luogo_nome}...")
            tasks.append(astronomy.get_sky_data(
                client_ip=state.get("client_ip", "127.0.0.1"),
                lat=lat,
                lon=lon,
                data_storia=state.get("data_storia"),
                ora_storia=state.get("ora_storia")
            ))

        # Attesa risultati
        results = await asyncio.gather(*tasks, return_exceptions=True)
        
        meteo_res = results[0] if not isinstance(results[0], Exception) else "meteo non disponibile"
        if usa_astro and len(results) > 1:
            astro_res = results[1] if not isinstance(results[1], Exception) else {}
            if isinstance(results[1], Exception):
                logger.error(f"Errore specifico AstronomyClient: {results[1]}")
        
        logger.info(f"✅ Dati recuperati: Meteo={meteo_res}, Astro={'Sì' if astro_res else 'No'}")

    except Exception as e:
        logger.error(f"❌ Errore generale nel nodo fisico: {e}", exc_info=True)

    return {
        "lat": lat,
        "lon": lon,
        "dati_meteo": meteo_res,
        "dati_astronomici": astro_res
    }
# ---------------------------------------------------------------------------
# NODE 5 — Valutazione qualità frammenti
# ---------------------------------------------------------------------------
async def valuta_frammenti(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_frammenti")
    if not state["frammenti"]:
        nuovi_termini, uso_llm = await _riformula_termini(state, llm)
        return {
            "qualita_frammenti": "assente",
            "termini_ricerca_neo4j": nuovi_termini,
            "llm_usage": [uso_llm] if uso_llm else [],
        }
    return {"qualita_frammenti": "buona"}

async def _riformula_termini(state: BaldoState, llm: LLMRouter) -> tuple[list[str], dict]:
    system = 'Suggerisci termini di ricerca alternativi in JSON: {"termini": []}'
    risultato = await llm.chiedi(system=system, user=state['ambientazione'])
    uso_llm = {
        "nodo": "valuta_frammenti._riformula_termini",
        "modello": risultato.modello,
        "token_input": risultato.token_input,
        "token_output": risultato.token_output,
    }
    try:
        dati = json.loads(risultato.testo.strip().strip("```json").strip("```"))
        return dati.get("termini", []), uso_llm
    except:
        return [], uso_llm

# ---------------------------------------------------------------------------
# NODE 6 — Composizione prompt
# ---------------------------------------------------------------------------
async def componi_prompt(state: BaldoState) -> dict:
    logger.info("[NODE] componi_prompt")
    composer = StoryComposer()
    
    prompt_arricchito, meta_composizione = composer.componi(
        prompt_originale=state["prompt_originale"],
        analisi={
            "personaggi": state["personaggi"],
            "emozioni": state["emozioni"],
            "ambientazione": state["ambientazione"],
            "luogo": state.get("luogo"),
            # Nomi chiave allineati a quelli letti da composer.py — prima
            # erano "data"/"meteo" (mai letti: composer cerca data_storia/
            # dati_meteo), quindi il prompt mostrava sempre i placeholder
            # generici "oggi"/"sereno" invece dei dati reali già raccolti
            # da fetch_contesto_fisico, indipendentemente dal meteo/data vero.
            "data_storia": state.get("data_storia"),
            "ora_storia": state.get("ora_storia"),
            "dati_meteo": state.get("dati_meteo"),
        },
        frammenti=state["frammenti"],
        dati_astronomici=state.get("dati_astronomici") if state.get("usa_astronomia") else None,
        storie_precedenti=state.get("storie_precedenti", []),
        personaggi_bio=state.get("personaggi_bio", {}),
        # "eta_bambino", non "eta": composer.py legge kwargs.get('eta_bambino',
        # ...) — con la chiave sbagliata l'età reale non arrivava mai e ogni
        # regola che dipende dall'età (vocabolario, numero di inganni) cadeva
        # sempre sul valore di default, qualunque età fosse stata scelta.
        eta_bambino=state["eta_bambino"],
        lunghezza=state["lunghezza"],
        lingua=state["lingua"],
    )
    return {
        "prompt_arricchito": prompt_arricchito,
        "tecnica_narrativa_kb": meta_composizione.get("tecnica_narrativa"),
        "archetipo_kb": meta_composizione.get("archetipo"),
    }

# Sotto questa soglia di parole consecutive uguali, il confronto rischia
# falsi positivi (formule ricorrenti per caso, non un ritornello voluto).
_RITORNELLO_MIN_PAROLE = 6

_PUNTEGGIATURA_BORDO_RE = re.compile(r"^[«»\"'“”,.:;!?]+|[«»\"'“”,.:;!?]+$")


def _parola_normalizzata(parola: str) -> str:
    return _PUNTEGGIATURA_BORDO_RE.sub("", parola).lower()


def _rileva_ritornello(testo: str) -> "str | None":
    """
    Cerca la sequenza di parole più lunga che si ripete almeno 2 volte nel
    testo (almeno _RITORNELLO_MIN_PAROLE parole) — più affidabile che
    fidarsi del campo "ritornello" del frammento KB: quel testo può
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
    """
    parole = re.findall(r"\S+", testo or "")
    n = _RITORNELLO_MIN_PAROLE
    if len(parole) < n * 2:
        return None

    visti = {}
    for i in range(len(parole) - n + 1):
        chiave = tuple(_parola_normalizzata(p) for p in parole[i:i + n])
        if not all(chiave):  # n-gramma con solo punteggiatura in qualche slot
            continue
        if chiave in visti:
            j = visti[chiave]
            # Estende la ripetizione oltre le n parole minime, finché le due
            # occorrenze continuano a coincidere parola per parola.
            k = n
            while (
                i + k < len(parole) and j + k < i
                and _parola_normalizzata(parole[i + k]) == _parola_normalizzata(parole[j + k])
            ):
                k += 1
            return " ".join(parole[i:i + k]).strip("«»\"'“”,.:;!? ")
        visti.setdefault(chiave, i)
    return None


# ---------------------------------------------------------------------------
# NODE 7-10 — Generazione e Rifinitura (Draft, Correzione, Rifinitura)
# ---------------------------------------------------------------------------
async def genera_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] genera_draft")
    prompt_input = state.get("prompt_arricchito") or state["prompt_originale"]
    risultato = await llm.genera_racconto(prompt_input)
    return {
        "draft": risultato.testo,
        "ritornello_atteso": _rileva_ritornello(risultato.testo),
        "llm_usage": [{
            "nodo": "genera_draft",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
        }],
    }

async def valuta_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] valuta_draft")
    return {"valutazione_draft": "ok"}

async def correggi_draft(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] correggi_draft")
    return {"tentativi_correzione": state["tentativi_correzione"] + 1}

async def rifinisci(state: BaldoState, llm: LLMRouter) -> dict:
    logger.info("[NODE] rifinisci")
    risultato = await llm.rifinisci(
        draft=state["draft"],
        eta=state["eta_bambino"],
        ritornello=state.get("ritornello_atteso"),
    )
    return {
        "racconto_finale": risultato.testo,
        "llm_usage": [{
            "nodo": "rifinisci",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
        }],
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
    stesso principio della
    domanda ma senza tentativo di riscrittura automatica — un ritornello,
    a differenza della domanda finale, è ripetuto più volte nel testo: una
    riscrittura automatica rischierebbe di introdurre incoerenza tra le
    ripetizioni invece di risolverla).

    In due passaggi, non uno, per la domanda: prima un classificatore SI/NO
    economico, e solo se la risposta è NO si chiede la riscrittura mirata.
    Un'unica chiamata "verifica-e-se-serve-correggi" è stata scartata: anche
    istruita a restituire il testo invariato quando già coerente, un LLM
    tende comunque a "migliorarlo" — riscriveva domande già valide,
    vanificando lo scopo di un controllo mirato.
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
        llm.chiedi(system=system_ritornello, user=racconto),
        llm.chiedi(system=system_check, user=racconto),
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
        })
        if not risultato_ritornello.testo or not risultato_ritornello.testo.strip().upper().startswith("SI"):
            logger.warning(
                "[verifica_coerenza_domanda] Il ritornello sembra nominare "
                "qualcosa non presente altrove nella storia."
            )

    if isinstance(risultato_check, Exception):
        logger.warning(f"[verifica_coerenza_domanda] Controllo fallito, mantengo il racconto originale: {risultato_check}")
        return {"llm_usage": uso_llm, "nome_presente_in_output": nome_presente, "termini_kb_trapelati": termini_trapelati}

    esito = risultato_check.testo
    uso_llm.append({
        "nodo": "verifica_coerenza_domanda.check",
        "modello": risultato_check.modello,
        "token_input": risultato_check.token_input,
        "token_output": risultato_check.token_output,
    })

    if not esito or esito.strip().upper().startswith("SI"):
        # coerente (o controllo ambiguo): non tocco nulla
        return {"llm_usage": uso_llm, "nome_presente_in_output": nome_presente, "termini_kb_trapelati": termini_trapelati}

    # Chiediamo SOLO la nuova domanda, non l'intera favola riscritta: la
    # versione precedente chiedeva la favola completa "copiando ogni
    # paragrafo esattamente com'è" tranne l'ultima frase — in pratica una
    # seconda riscrittura integrale (osservato: token in uscita quasi pari
    # al draft e a rifinisci), con relativo costo e un'occasione in più di
    # deriva sul testo che rifinisci aveva già sistemato. La domanda finale
    # è quasi sempre il suo paragrafo — la sostituzione avviene qui, per
    # codice, non chiedendo all'LLM di riprodurre tutto il resto.
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
        risultato_fix = await llm.chiedi(system=system_fix, user=racconto)
        nuova_domanda = (risultato_fix.testo or "").strip()
    except Exception as e:
        logger.warning(f"[verifica_coerenza_domanda] Riscrittura fallita, mantengo il racconto originale: {e}")
        return {"llm_usage": uso_llm, "nome_presente_in_output": nome_presente, "termini_kb_trapelati": termini_trapelati}

    uso_llm.append({
        "nodo": "verifica_coerenza_domanda.fix",
        "modello": risultato_fix.modello,
        "token_input": risultato_fix.token_input,
        "token_output": risultato_fix.token_output,
    })

    # Rete di sicurezza adattata al nuovo formato: ci aspettiamo una singola
    # frase breve, non più un testo lungo quanto l'originale. Se il modello
    # ha comunque provato a restituire un testo lungo o multi-paragrafo,
    # ha ignorato l'istruzione — meglio tenere l'originale con la domanda
    # scorrelata che rischiare di incollare un frammento non affidabile.
    if not nuova_domanda or "\n\n" in nuova_domanda or len(nuova_domanda) > 400:
        logger.warning(
            f"[verifica_coerenza_domanda] Risposta del fix non è la domanda "
            f"breve richiesta (lunghezza {len(nuova_domanda)}), mantengo il "
            f"racconto originale."
        )
        return {"llm_usage": uso_llm, "nome_presente_in_output": nome_presente, "termini_kb_trapelati": termini_trapelati}

    corretto = _sostituisci_ultimo_paragrafo(racconto, nuova_domanda)
    return {"racconto_finale": corretto, "llm_usage": uso_llm, "nome_presente_in_output": nome_presente, "termini_kb_trapelati": termini_trapelati}

# ---------------------------------------------------------------------------
# NODE 11 — Salvataggio memoria
# ---------------------------------------------------------------------------
async def salva_memoria(state: BaldoState, neo4j: Neo4jClient) -> dict:
    logger.info("[NODE] salva_memoria")
    storia_id = str(uuid.uuid4())
    ids_frammenti = [f["id"] for f in state.get("frammenti", []) if f.get("id")]

    await neo4j.salva_storia(
        story_id=storia_id,
        text=state["racconto_finale"],
        characters=state["personaggi"],
        emotions=state["emozioni"],
        setting=state["ambientazione"],
        original_prompt=state["prompt_originale"],
        used_fragments=ids_frammenti,
        timestamp=datetime.utcnow().isoformat(),
    )
    return {"storia_id": storia_id, "frammenti_usati": ids_frammenti}

# ---------------------------------------------------------------------------
# NODE — Errore
# ---------------------------------------------------------------------------
async def nodo_errore(state: BaldoState) -> dict:
    return {"racconto_finale": "", "errore": state.get("motivo_rifiuto")}
