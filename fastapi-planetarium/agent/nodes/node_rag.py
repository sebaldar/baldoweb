"""
nodes/node_rag.py — self-contained, no cross-project imports
"""
from __future__ import annotations
import logging
import re

from qdrant_client import QdrantClient
from qdrant_client.models import (
    FieldCondition, MatchValue, MatchAny, Range, OrderBy, Filter
)
from sentence_transformers import SentenceTransformer

from agent.state import PlanetariumState
from agent.emitter import SseEmitter, emit_thinking, emit_rag_result
from services.config import get_settings

logger = logging.getLogger(__name__)

COLLECTION_NAME = "planetarium_kb"
EMBEDDING_MODEL = "all-MiniLM-L6-v2"

# ── Dizionari e Costanti ─────────────────────────────────────────────────────
COSTELLAZIONI: dict[str, str] = {
    "andromeda": "Andromeda", "antlia": "Antlia", "macchina pneumatica": "Antlia",
    "apus": "Apus", "uccello del paradiso": "Apus", "aquarius": "Aquarius", "acquario": "Aquarius",
    "aquila": "Aquila", "ara": "Ara", "altare": "Ara", "aries": "Aries", "ariete": "Aries",
    "auriga": "Auriga", "bootes": "Boötes", "boötes": "Boötes", "boote": "Boötes", "bovaro": "Boötes",
    "caelum": "Caelum", "bulino": "Caelum", "cesello": "Caelum", "camelopardalis": "Camelopardalis", "giraffa": "Camelopardalis",
    "cancer": "Cancer", "cancro": "Cancer", "canes venatici": "Canes Venatici", "cani da caccia": "Canes Venatici",
    "canis major": "Canis Major", "cane maggiore": "Canis Major", "canis minor": "Canis Minor", "cane minore": "Canis Minor",
    "capricornus": "Capricornus", "capricorno": "Capricornus", "carina": "Carina", "carena": "Carina",
    "cassiopeia": "Cassiopeia", "cassiopea": "Cassiopeia", "centaurus": "Centaurus", "centauro": "Centaurus",
    "cepheus": "Cepheus", "cefeo": "Cepheus", "cetus": "Cetus", "balena": "Cetus", "cetaceo": "Cetus",
    "chamaeleon": "Chamaeleon", "camaleonte": "Chamaeleon", "circinus": "Circinus", "compasso": "Circinus",
    "columba": "Columba", "colomba": "Columba", "coma berenices": "Coma Berenices", "chioma di berenice": "Coma Berenices",
    "corona australis": "Corona Australis", "corona australe": "Corona Australis",
    "corona borealis": "Corona Borealis", "corona boreale": "Corona Borealis",
    "corvus": "Corvus", "corvo": "Corvus", "crater": "Crater", "coppa": "Crater", "cratere": "Crater",
    "crux": "Crux", "croce del sud": "Crux", "cygnus": "Cygnus", "cigno": "Cygnus",
    "delphinus": "Delphinus", "delfino": "Delphinus", "dorado": "Dorado", "doradus": "Dorado", "pesce spada": "Dorado",
    "draco": "Draco", "dragone": "Draco", "drago": "Draco", "equuleus": "Equuleus", "cavallino": "Equuleus",
    "eridanus": "Eridanus", "eridano": "Eridanus", "fornax": "Fornax", "fornace": "Fornax",
    "gemini": "Gemini", "gemelli": "Gemini", "grus": "Grus", "gru": "Grus",
    "hercules": "Hercules", "ercole": "Hercules", "horologium": "Horologium", "orologio": "Horologium",
    "hydra": "Hydra", "idra": "Hydra", "hydrus": "Hydrus", "idra maschio": "Hydrus", "idro": "Hydrus",
    "indus": "Indus", "indiano": "Indus", "lacerta": "Lacerta", "lucertola": "Lacerta",
    "leo": "Leo", "leone": "Leo", "leo minor": "Leo Minor", "leone minore": "Leo Minor",
    "lepus": "Lepus", "lepre": "Lepus", "libra": "Libra", "bilancia": "Libra",
    "lupus": "Lupus", "lupo": "Lupus", "lynx": "Lynx", "lince": "Lynx",
    "lyra": "Lyra", "lira": "Lyra", "mensa": "Mensa", "monte mensa": "Mensa",
    "microscopium": "Microscopium", "microscopio": "Microscopium", "monoceros": "Monoceros", "unicorno": "Monoceros",
    "musca": "Musca", "mosca": "Musca", "norma": "Norma", "squadra": "Norma",
    "octans": "Octans", "ottante": "Octans", "ophiuchus": "Ophiuchus", "ofiuco": "Ophiuchus", "serpentario": "Ophiuchus",
    "orion": "Orion", "orione": "Orion", "pavo": "Pavo", "pavone": "Pavo",
    "pegasus": "Pegasus", "pegaso": "Pegasus", "perseus": "Perseus", "perseo": "Perseus",
    "phoenix": "Phoenix", "fenice": "Phoenix", "pictor": "Pictor", "pittore": "Pictor",
    "pisces": "Pisces", "pesci": "Pisces", "piscis austrinus": "Piscis Austrinus", "pesce australe": "Piscis Austrinus",
    "puppis": "Puppis", "poppa": "Puppis", "pyxis": "Pyxis", "bussola": "Pyxis",
    "reticulum": "Reticulum", "reticolo": "Reticulum", "sagitta": "Sagitta", "freccia": "Sagitta",
    "sagittarius": "Sagittarius", "sagittario": "Sagittarius", "scorpius": "Scorpius", "scorpione": "Scorpius", "scorpio": "Scorpius",
    "sculptor": "Sculptor", "scultore": "Sculptor", "scutum": "Scutum", "scudo": "Scutum",
    "serpens": "Serpens", "serpente": "Serpens", "sextans": "Sextans", "sestante": "Sextans",
    "taurus": "Taurus", "toro": "Taurus", "telescopium": "Telescopium", "telescopio": "Telescopium",
    "triangulum": "Triangulum", "triangolo": "Triangulum",
    "triangulum australe": "Triangulum Australe", "triangolo australe": "Triangulum Australe",
    "tucana": "Tucana", "tucano": "Tucana",
    "ursa major": "Ursa Major", "orsa maggiore": "Ursa Major", "grande orsa": "Ursa Major",
    "ursa minor": "Ursa Minor", "orsa minore": "Ursa Minor", "piccola orsa": "Ursa Minor",
    "vela": "Vela", "virgo": "Virgo", "vergine": "Virgo",
    "volans": "Volans", "pesce volante": "Volans", "vulpecula": "Vulpecula", "volpetta": "Vulpecula", "volpe": "Vulpecula",
}

STELLE_IT_EN: dict[str, str] = {
    "sirio": "Sirius", "stella polare": "Polaris", "polare": "Polaris",
    "arturo": "Arcturus", "castore": "Castor", "polluce": "Pollux",
    "procione": "Procyon", "regolo": "Regulus"
}

COSTELLAZIONI_CENTRI: dict[str, dict[str, str]] = {
    "Andromeda": {"ra": "14.4", "dec": "37.4"}, "Aquarius": {"ra": "334.3", "dec": "-11.0"},
    "Aquila": {"ra": "295.5", "dec": "3.4"}, "Aries": {"ra": "41.5", "dec": "20.8"},
    "Auriga": {"ra": "90.4", "dec": "42.0"}, "Boötes": {"ra": "222.0", "dec": "30.5"},
    "Cancer": {"ra": "129.5", "dec": "20.0"}, "Canis Major": {"ra": "105.0", "dec": "-22.0"},
    "Canis Minor": {"ra": "114.0", "dec": "6.4"}, "Capricornus": {"ra": "316.0", "dec": "-19.0"},
    "Cassiopeia": {"ra": "15.0", "dec": "60.0"}, "Centaurus": {"ra": "196.0", "dec": "-48.0"},
    "Crux": {"ra": "186.0", "dec": "-60.0"}, "Cygnus": {"ra": "307.0", "dec": "40.0"},
    "Draco": {"ra": "255.0", "dec": "65.0"}, "Gemini": {"ra": "105.0", "dec": "22.0"},
    "Hercules": {"ra": "255.0", "dec": "30.0"}, "Leo": {"ra": "160.0", "dec": "15.0"},
    "Lyra": {"ra": "282.0", "dec": "36.0"}, "Ophiuchus": {"ra": "258.0", "dec": "-8.0"},
    "Orion": {"ra": "83.8", "dec": "5.3"}, "Pegasus": {"ra": "340.0", "dec": "19.0"},
    "Perseus": {"ra": "48.0", "dec": "43.0"}, "Pisces": {"ra": "15.0", "dec": "15.0"},
    "Sagittarius": {"ra": "285.0", "dec": "-25.0"}, "Scorpius": {"ra": "250.0", "dec": "-38.0"},
    "Taurus": {"ra": "67.5", "dec": "15.0"}, "Ursa Major": {"ra": "172.5", "dec": "55.4"},
    "Ursa Minor": {"ra": "225.0", "dec": "77.5"}, "Virgo": {"ra": "200.0", "dec": "-4.0"}
}

COSTELLAZIONI_SIGLE: dict[str, str] = {
    "Andromeda": "AND", "Aquarius": "AQR", "Aquila": "AQL", "Aries": "ARI", "Auriga": "AUR",
    "Boötes": "BOO", "Cancer": "CNC", "Canis Major": "CMA", "Canis Minor": "CMI",
    "Capricornus": "CAP", "Cassiopeia": "CAS", "Centaurus": "CEN", "Crux": "CRU",
    "Cygnus": "CYG", "Draco": "DRA", "Gemini": "GEM", "Hercules": "HER", "Leo": "LEO",
    "Lyra": "LYR", "Ophiuchus": "OPH", "Orion": "ORI", "Pegasus": "PEG", "Perseus": "PER",
    "Pisces": "PSC", "Sagittarius": "SGR", "Scorpius": "SCO", "Taurus": "TAU",
    "Ursa Major": "UMA", "Ursa Minor": "UMI", "Virgo": "VIR"
}

SIGLE_TO_EN = {v.lower(): k for k, v in COSTELLAZIONI_SIGLE.items()}

# ── Funzioni Helper ──────────────────────────────────────────────────────────
def _normalize_target_name(name: str) -> str:
    if not name: return ""
    nl = name.lower().strip()
    if nl in STELLE_IT_EN: return STELLE_IT_EN[nl]
    if nl in COSTELLAZIONI: return COSTELLAZIONI[nl]
    if nl in SIGLE_TO_EN: return SIGLE_TO_EN[nl]
    return name

_client: QdrantClient | None = None
_model:  SentenceTransformer | None = None

def _get_client() -> QdrantClient:
    global _client
    if _client is None:
        s = get_settings()
        _client = QdrantClient(host=s.qdrant_host, port=s.qdrant_port)
    return _client

def _get_model() -> SentenceTransformer:
    global _model
    if _model is None:
        _model = SentenceTransformer(EMBEDDING_MODEL)
    return _model

def _embed(text: str) -> list[float]:
    return _get_model().encode([text], show_progress_bar=False)[0].tolist()

import re

def _find_constellation_name(prompt: str) -> str | None:
    """Cerca il nome di una costellazione, forzando la corrispondenza a parola intera."""
    p = prompt.lower()

    # Assumo che tu iteri su un dizionario o lista di costellazioni
    # Adatta "COSTELLAZIONI" al nome reale della tua variabile
    for nome_it, nome_en in COSTELLAZIONI.items():

        # Il \b assicura che cerchiamo la parola esatta e non una sottostringa
        # re.escape previene errori se ci sono caratteri strani nel nome
        pattern_it = rf'\b{re.escape(nome_it.lower())}\b'
        pattern_en = rf'\b{re.escape(nome_en.lower())}\b'

        if re.search(pattern_it, p) or re.search(pattern_en, p):
            return nome_en

    return None

def _guess_tipo(prompt: str) -> str | None:
    p = prompt.lower()
    if any(w in p for w in ["stella","star","gigante","nana","brillant","luminos","magnitudine","mag"]):
        return "STARS"
    if any(w in p for w in ["pianeta","luna","asteroide","sole","sun"]):
        return "SOLARSYSTEM"
    if _find_constellation_name(p) or any(w in p for w in ["costellazione","constellation"]):
        return "CONSTELLATIONS"
    if any(w in p for w in ["eclisse","meteore","pioggia","aurora"]):
        return "PHENOMENA"
    return None

def _build_math_query(prompt: str) -> tuple[list[FieldCondition], OrderBy | None, int]:
    """Estrae filtri numerici e ordinamenti matematici dal linguaggio naturale."""
    p = prompt.lower()
    conds = []
    order_by = None
    limit = 6

    if "magnitudine" in p or "mag" in p or "brillant" in p or "luminos" in p:
        match_tra = re.search(r'tra\s+(-?\d+(?:[\.,]\d+)?)\s+e\s+(-?\d+(?:[\.,]\d+)?)', p)
        match_min = re.search(r'(?:minor|sotto|inferior)[^\d\-]+(-?\d+(?:[\.,]\d+)?)', p)
        match_max = re.search(r'(?:maggior|sopra|superior)[^\d\-]+(-?\d+(?:[\.,]\d+)?)', p)

        campo_mag = "magnitudine_v"

        if match_tra:
            v1 = float(match_tra.group(1).replace(',', '.'))
            v2 = float(match_tra.group(2).replace(',', '.'))
            conds.append(FieldCondition(key=campo_mag, range=Range(gte=min(v1,v2), lte=max(v1,v2))))
            limit = 10
        elif match_min:
            val = float(match_min.group(1).replace(',', '.'))
            conds.append(FieldCondition(key=campo_mag, range=Range(lt=val)))
            limit = 10
        elif match_max:
            val = float(match_max.group(1).replace(',', '.'))
            conds.append(FieldCondition(key=campo_mag, range=Range(gt=val)))
            limit = 10

    if ("brillant" in p or "luminos" in p) and not (match_tra or match_min or match_max):
        limit = 3 if "tre" in p else (10 if "dieci" in p else 5)
        order_by = OrderBy(key=campo_mag, direction="asc")
        conds.append(FieldCondition(key=campo_mag, range=Range(lt=50)))

        if "nord" in p or "boreale" in p:
            conds.append(FieldCondition(key="dec", range=Range(gte=0.0)))
        elif "sud" in p or "australe" in p:
            conds.append(FieldCondition(key="dec", range=Range(lte=0.0)))

    return conds, order_by, limit


# ── NODO RAG PRINCIPALE ──────────────────────────────────────────────────────
def node_rag(state: PlanetariumState, emitter: SseEmitter) -> dict:
    emit_thinking(emitter, "Cerco nella knowledge base…")
    client = _get_client()
    docs: list[dict] = []

    prompt = state.get("prompt", "")

    # ── 1. Ricerca diretta costellazione per nome
    constellation_found = False
    nome_en = _find_constellation_name(prompt)
    if nome_en:
        try:
            results, _ = client.scroll(
                collection_name=COLLECTION_NAME,
                scroll_filter=Filter(must=[
                    FieldCondition(key="tipo", match=MatchValue(value="CONSTELLATIONS")),
                    FieldCondition(key="nome", match=MatchValue(value=nome_en)),
                ]),
                limit=1, with_payload=True,
            )
            docs = [r.payload for r in results]
            if docs:
                constellation_found = True
        except Exception as e:
            logger.error(f"[RAG] Direct constellation fallita: {e}")

    # ── 2. Ricerca Strutturata Dinamica (Matematica/Numerica)
    math_attempted = False
    if not constellation_found and not docs:
        math_conds, math_order, math_limit = _build_math_query(prompt)

        if math_conds or math_order:
            math_attempted = True
            try:
                math_conds.append(FieldCondition(key="tipo", match=MatchValue(value="STARS")))
                results, _ = client.scroll(
                    collection_name=COLLECTION_NAME,
                    scroll_filter=Filter(must=math_conds),
                    order_by=math_order,
                    limit=math_limit,
                    with_payload=True,
                )
                docs = [r.payload for r in results]
            except Exception as e:
                logger.error(f"[RAG] Math Query fallita: {e}")

    # ── 3. Similarity search
    if not constellation_found and not docs and not math_attempted:
        tipo = _guess_tipo(prompt)
        target_richiesto = state.get("extra", {}).get("target")
        testo_ricerca = _normalize_target_name(target_richiesto) if target_richiesto else prompt

        try:
            conds = []
            if tipo == "STARS":
                conds = [FieldCondition(key="tipo", match=MatchAny(any=["STARS", "STAR", "Stella", "stella"]))]
            elif tipo:
                conds = [FieldCondition(key="tipo", match=MatchValue(value=tipo))]

            hits = client.search(
                collection_name=COLLECTION_NAME,
                query_vector=_embed(testo_ricerca),
                query_filter=Filter(must=conds) if conds else None,
                limit=5, with_payload=True,
            )
            existing = {d.get("nome") for d in docs}
            for h in hits:
                if h.payload.get("nome") not in existing:
                    docs.append(h.payload)
                    existing.add(h.payload.get("nome"))
        except Exception as e:
            logger.error(f"[RAG] Similarity fallita: {e}")

    # ── 4. Popolamento EXTRA Auto-Curante ────────────────────────────────────
    extra_data = state.get("extra", {}).copy()

    if docs:
        # REGOLA SCUDO: Se il classificatore ha già stabilito che è un corpo del sistema solare
        # (es. sole, luna, pianeti), il JS sa già gestirlo con il comando "lookat".
        # VIETATO inquinare il target con risultati RAG casuali!
        if extra_data.get("needs_realtime"):
            pass
        else:
            main_doc = docs[0]

            target_originale = extra_data.get("target", "")
            # Piccolo fix: puliamo la stringa se esiste
            target_norm = target_originale.lower().strip() if target_originale else ""

            if target_norm:
                for d in docs:
                    n_it = str(d.get("nome_italiano") or "").lower()
                    n_en = str(d.get("nome") or "").lower()
                    # Rimosso il controllo _normalize_target_name che faceva saltare il match
                    if target_norm == n_it or target_norm == n_en or target_norm in n_it or target_norm in n_en:
                        main_doc = d
                        break

            tipo_doc = str(main_doc.get("tipo", "")).upper()
            nome_en  = main_doc.get("nome")

            extra_data["target"] = nome_en

            if "STAR" in tipo_doc:
                extra_data["category"] = "stella"
                extra_data["needs_realtime"] = False
            elif "SOLAR" in tipo_doc or "PLANET" in tipo_doc or "MOON" in tipo_doc:
                extra_data["category"] = "pianeta o luna"
                extra_data["needs_realtime"] = True
            elif "CONSTEL" in tipo_doc:
                extra_data["category"] = "costellazione"
                extra_data["needs_realtime"] = False
            else:
                extra_data["category"] = "evento"

            ra = main_doc.get("ra") or main_doc.get("ascensione_retta") or main_doc.get("RA")
            dec = main_doc.get("dec") or main_doc.get("declinazione") or main_doc.get("DEC")

            if extra_data.get("category") == "costellazione":
                if ra is None or dec is None:
                    centro = COSTELLAZIONI_CENTRI.get(nome_en, {})
                    ra = centro.get("ra")
                    dec = centro.get("dec")

                sigla_iau = COSTELLAZIONI_SIGLE.get(nome_en)
                if sigla_iau:
                    extra_data["target"] = sigla_iau.upper()

            # Evitiamo il dirottamento telecamera sui movimenti manuali
            if extra_data.get("is_movement"):
                extra_data.pop("target", None)
                extra_data.pop("category", None)
                extra_data.pop("needs_realtime", None)
            else:
                if ra is not None: extra_data["ra"] = str(ra)
                if dec is not None: extra_data["dec"] = str(dec)

    emit_rag_result(emitter, found=len(docs), label=f"Trovate {len(docs)} fonti" if docs else "Nessuna fonte trovata")

    return {
        "rag_docs": docs[:6],
        "extra": extra_data
    }
