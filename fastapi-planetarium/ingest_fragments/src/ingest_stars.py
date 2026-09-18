"""
ingest_stars.py
Ingestion stelle dal catalogo BSC5.

Schema JSON reale:
  HR, HD, SAO, DM, RA, Dec, Vmag, SpectralCls, LuminosityCls,
  K (temperatura in Kelvin), B-V, U-B, pmRA, pmDE, RadVel, RotVel,
  GLON, GLAT, Name (Bayer/Flamsteed), Notes (lista dict con Category/Remark)

Nomi propri: estratti da Notes[Category="Star names"].Remark
  - Tutto MAIUSCOLO → nome IAU ufficiale (es. "ALPHERATZ" → "Alpheratz")
  - Title Case ≤3 parole senza numeri → nome proprio storico (es. "Caph")
  - Resto → scartato (es. "4 Cet in Psc", "Called CPD...")

Wikipedia: chiamata SOLO per le ~271 stelle con nome proprio estratto.
Tutte le altre (~5000) vengono indicizzate da dati BSC5 puri.
"""

import asyncio
import json
import logging
import re
from pathlib import Path
from typing import Optional

from .spectral import decode_spectral_type, build_spectral_text
from .wikipedia import WikipediaClient
from .qdrant_store import PlanetariumStore

logger = logging.getLogger(__name__)

MAG_LIMIT  = 6.5
BATCH_SIZE = 5

# Parole da escludere nel filtro Title Case
_SKIP_WORDS = {
    'in', 'of', 'the', 'al', 'el', 'called', 'according',
    'name', 'also', 'formerly', 'shared', 'misnumbering',
}


# ── Estrazione nome proprio da Notes ─────────────────────────────────────────

def _extract_proper_name(notes: list) -> Optional[str]:
    """
    Estrae il nome proprio da Notes[Category='Star names'].Remark.
    Strategia:
      1. Prima parola/frase prima di ';' o '.'
      2. Se tutto MAIUSCOLO e len > 2 → nome IAU (converti in Title Case)
      3. Se Title Case, ≤3 parole, no numeri, no parole funzione → accetta
      4. Altrimenti → None
    """
    for note in (notes or []):
        if note.get("Category") != "Star names":
            continue
        remark = note.get("Remark", "").strip()
        if not remark:
            continue

        first = re.split(r'[;.]', remark)[0].strip()
        if not first:
            continue

        # Caso 1: nome IAU tutto maiuscolo (es. "ALPHERATZ")
        if first.isupper() and len(first) > 2:
            return first.title()

        # Caso 2: Title Case con filtri
        words = first.split()
        if (
            1 <= len(words) <= 3
            and not any(c.isdigit() for c in first)
            and not any(w.lower() in _SKIP_WORDS for w in words)
            and words[0][0].isupper()
        ):
            return first

    return None


# ── Parsing coordinate ────────────────────────────────────────────────────────

def _parse_coordinate(coord_str: str, is_ra: bool = True) -> float:
    """Converte RA (h m s) o Dec (° ′ ″) in gradi decimali."""
    try:
        nums = re.findall(r"[-+]?\d*\.?\d+", str(coord_str))
        if len(nums) < 3:
            return 0.0

        # Usiamo abs() sul primo numero per calcolare la magnitudine assoluta
        val = abs(float(nums[0])) + float(nums[1]) / 60.0 + float(nums[2]) / 3600.0

        # Riapplichiamo il segno solo se la stringa originale era negativa
        if str(coord_str).strip().startswith('-'):
            val = -val

        return round(val * 15.0, 6) if is_ra else round(val, 6)
    except Exception:
        return 0.0

# ── Parsing record BSC5 ───────────────────────────────────────────────────────

def parse_bs5_record(raw: dict) -> Optional[dict]:
    hr = raw.get("HR")
    if not hr:
        return None
    try:
        hr_int = int(hr)
    except (ValueError, TypeError):
        return None

    try:
        vmag = float(raw.get("Vmag", 99) or 99)
    except (ValueError, TypeError):
        vmag = 99.0

    # Temperatura: campo K già in Kelvin nel catalogo
    try:
        temp_k = int(raw.get("K", 0) or 0)
        temp_k = temp_k if temp_k > 0 else None
    except (ValueError, TypeError):
        temp_k = None

    # Indice colore B-V
    try:
        bv_str = str(raw.get("B-V", "")).replace("+", "").strip()
        bv = float(bv_str) if bv_str else None
    except (ValueError, TypeError):
        bv = None

    # Velocità radiale
    try:
        rad_vel = float(str(raw.get("RadVel", "")).strip() or 0)
    except (ValueError, TypeError):
        rad_vel = None

    # Velocità di rotazione
    try:
        rot_val = str(raw.get("RotVel", "")).strip()
        rot_vel = float(rot_val) if rot_val else None
    except (ValueError, TypeError):
        rot_vel = None

    # Moto proprio
    try:
        pm_ra = float(raw.get("pmRA", 0) or 0)
        pm_de = float(raw.get("pmDE", 0) or 0)
    except (ValueError, TypeError):
        pm_ra = pm_de = None

    # Spettro: SpectralCls + LuminosityCls
    spettro_grezzo = (
        (raw.get("SpectralCls") or "").strip() +
        (raw.get("LuminosityCls") or "").strip()
    ).strip() or None

    # Nome proprio estratto dalle Note
    proper_name = _extract_proper_name(raw.get("Notes", []))

    return {
        "hr":               hr_int,
        "hd":               str(raw.get("HD", "")).strip() or None,
        "sao":              str(raw.get("SAO", "")).strip() or None,
        "dm":               str(raw.get("DM", "")).strip() or None,
        "bayer_flamsteed":  str(raw.get("Name", "")).strip() or None,
        "proper_name":      proper_name,
        "ra":               _parse_coordinate(raw.get("RA", "0"), is_ra=True),
        "dec":              _parse_coordinate(raw.get("Dec", "0"), is_ra=False),
        "magnitudine_v":    vmag,
        "distanza_ly":      None,       # non presente nel JSON
        "luminosita_solare": None,      # non presente nel JSON
        "spettro_grezzo":   spettro_grezzo,
        "temperatura_k_bs5": temp_k,   # temperatura diretta dal catalogo
        "bv_index":         bv,
        "rad_vel":          rad_vel,
        "rot_vel":          rot_vel,
        "pm_ra":            pm_ra,
        "pm_de":            pm_de,
        "glon":             float(raw.get("GLON", 0) or 0),
        "glat":             float(raw.get("GLAT", 0) or 0),
    }


# ── Nome da visualizzare ──────────────────────────────────────────────────────

def _display_name(star: dict) -> str:
    return (
        star["proper_name"]
        or star["bayer_flamsteed"]
        or f"HR {star['hr']}"
    )


# ── Testo per embedding ───────────────────────────────────────────────────────

def build_star_text(star: dict, wiki_summary: Optional[str], spettro: dict) -> str:
    parts = []
    nome = _display_name(star)

    # Base narrativa: Wikipedia se disponibile, altrimenti sintetica
    if wiki_summary:
        parts.append(wiki_summary[:1000])
    else:
        parts.append(f"{nome} è una stella del catalogo BSC5 (HR {star['hr']}).")

    # Arricchimento spettrale garantito — colore sempre presente
    spettro_text = build_spectral_text(spettro)
    if spettro_text:
        parts.append(spettro_text)

    # Temperatura: BS5 (K) ha priorità sulla stima da spettro
    temp = star.get("temperatura_k_bs5") or spettro.get("temperatura_stimata_k")
    if temp:
        parts.append(f"La temperatura superficiale è circa {temp:,} kelvin.")

    # Magnitudine
    if star["magnitudine_v"] < 99:
        vis = "visibile a occhio nudo" if star["magnitudine_v"] < MAG_LIMIT else "telescopica"
        parts.append(
            f"Ha magnitudine visuale {star['magnitudine_v']:.2f}, {vis}."
        )

    # Indice B-V (utile per ricerche per colore)
    if star.get("bv_index") is not None:
        parts.append(f"Indice di colore B-V: {star['bv_index']:+.2f}.")

    return " ".join(parts)


# ── Builder documento Qdrant ──────────────────────────────────────────────────

def build_star_document(
    star: dict,
    wiki: Optional[dict],
    spettro: dict,
) -> dict:
    nome       = _display_name(star)
    text       = build_star_text(star, wiki["summary"] if wiki else None, spettro)
    temp_finale = star.get("temperatura_k_bs5") or spettro.get("temperatura_stimata_k")

    return {
        # Qdrant obbligatori
        "tipo":  "STARS",
        "nome":  nome,
        "text":  text,

        # Identificatori
        "hr":               star["hr"],
        "hd":               star["hd"],
        "sao":              star["sao"],
        "dm":               star["dm"],
        "bayer_flamsteed":  star["bayer_flamsteed"],
        "proper_name":      star["proper_name"],

        # Posizione
        "ra":               star["ra"],
        "dec":              star["dec"],
        "glon":             star["glon"],
        "glat":             star["glat"],

        # Fotometria
        "magnitudine_v":    star["magnitudine_v"],
        "bv_index":         star["bv_index"],
        "distanza_ly":      None,

        # Cinematica
        "rad_vel":          star["rad_vel"],
        "rot_vel":          star["rot_vel"],
        "pm_ra":            star["pm_ra"],
        "pm_de":            star["pm_de"],

        # Spettro decodificato (tutti filtrabili in Qdrant)
        "spettro_grezzo":           spettro["spettro_grezzo"],
        "classe_spettrale":         spettro["classe_spettrale"],
        "sottoclasse":              spettro["sottoclasse"],
        "classe_luminosita":        spettro["classe_luminosita"],
        "descrizione_classe":       spettro["descrizione_classe"],
        "descrizione_luminosita":   spettro["descrizione_luminosita"],
        "colori":                   spettro["colori"],        # list → MatchAny
        "colore_hex":               spettro["colore_hex"],
        "temperatura_stimata_k":    temp_finale,

        # Wikipedia
        "has_wikipedia":        wiki is not None,
        "wikipedia_summary":    wiki["summary"] if wiki else None,
        "wikipedia_url":        wiki["url"] if wiki else None,
        "wikipedia_thumbnail":  wiki["thumbnail"] if wiki else None,
    }


# ── Entry point ───────────────────────────────────────────────────────────────

async def ingest_stars(
    bs5_path: Path,
    store: PlanetariumStore,
    wiki_cache_dir: Path,
    mag_limit: float = MAG_LIMIT,
    batch_size: int = BATCH_SIZE,
) -> dict:

    logger.info(f"[STARS] Caricamento BS5 da {bs5_path}…")
    raw_catalog = json.loads(bs5_path.read_text(encoding="utf-8"))

    stats = {
        "totale_bs5":       len(raw_catalog),
        "filtrate_mag":     0,
        "record_invalidi":  0,
        "con_wikipedia":    0,
        "senza_wikipedia":  0,
        "inserite":         0,
    }

    # ── Filtra e normalizza ───────────────────────────────────────────────────
    stars: list[dict] = []
    for raw in raw_catalog:
        parsed = parse_bs5_record(raw)
        if parsed is None:
            stats["record_invalidi"] += 1
            continue
        if parsed["magnitudine_v"] > mag_limit:
            stats["filtrate_mag"] += 1
            continue
        stars.append(parsed)

    logger.info(
        f"[STARS] {len(stars)} stelle da processare "
        f"(scartate: {stats['filtrate_mag']} per mag, "
        f"{stats['record_invalidi']} invalide)"
    )

    # ── Wikipedia solo per stelle con nome proprio estratto ───────────────────
    named   = [s for s in stars if s["proper_name"]]
    unnamed = [s for s in stars if not s["proper_name"]]

    logger.info(
        f"[STARS] {len(named)} con nome proprio → Wikipedia; "
        f"{len(unnamed)} senza nome → BS5 only (0 chiamate API)"
    )

    wiki_map: dict[str, Optional[dict]] = {}

    if named:
        async with WikipediaClient(wiki_cache_dir) as wiki_client:
            requests = [
                (s["proper_name"], [f"{s['proper_name']} star"])
                for s in named
            ]
            results = await wiki_client.get_batch(requests)
            for s, wiki in zip(named, results):
                wiki_map[s["proper_name"]] = wiki

    # ── Build documenti e upsert in batch ─────────────────────────────────────
    batch: list[dict] = []

    def flush() -> None:
        if batch:
            store.upsert(batch)
            stats["inserite"] += len(batch)
            batch.clear()

    for star in stars:
        spettro = decode_spectral_type(star["spettro_grezzo"] or "")
        wiki    = wiki_map.get(star["proper_name"]) if star["proper_name"] else None

        if wiki:
            stats["con_wikipedia"] += 1
        else:
            stats["senza_wikipedia"] += 1

        batch.append(build_star_document(star, wiki, spettro))

        if len(batch) >= batch_size:
            flush()

    flush()

    logger.info(
        f"[STARS] ✅ {stats['inserite']} stelle inserite "
        f"({stats['con_wikipedia']} con Wikipedia, "
        f"{stats['senza_wikipedia']} da BS5)"
    )
    return stats
