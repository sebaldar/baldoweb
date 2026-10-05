"""
spectral.py
Decodifica il tipo spettrale MKK dal formato grezzo BS5.
Produce campi strutturati pronti per i metadati Qdrant.
"""

import re
from typing import Optional

# ── Mappings ────────────────────────────────────────────────────────────────

CLASS_DESC = {
    "O": "stella blu caldissima",
    "B": "stella blu-bianca calda",
    "A": "stella bianca",
    "F": "stella giallo-bianca",
    "G": "stella gialla (tipo solare)",
    "K": "stella arancione",
    "M": "stella rossa fredda",
    "C": "stella carbonio",
    "S": "stella a ossido di zirconio",
    "W": "stella Wolf-Rayet",
    "L": "nana bruna calda",
    "T": "nana bruna fredda",
}

# Colori percepiti — lista per MatchAny in Qdrant
CLASS_COLORS = {
    "O": ["blu", "azzurra"],
    "B": ["blu-bianca", "azzurra"],
    "A": ["bianca"],
    "F": ["giallo-bianca", "bianca"],
    "G": ["gialla"],
    "K": ["arancione"],
    "M": ["rossa", "rosso-arancione"],
    "C": ["rossa", "arancione"],
    "S": ["rossa"],
    "W": ["blu", "azzurra"],
    "L": ["rossa"],
    "T": ["rossa"],
}

# Colore hex per rendering
CLASS_COLOR_HEX = {
    "O": "#9bb0ff",
    "B": "#aabfff",
    "A": "#cad7ff",
    "F": "#f8f7ff",
    "G": "#fff4ea",
    "K": "#ffd2a1",
    "M": "#ffcc6f",
    "C": "#ff9966",
    "S": "#ffaa88",
    "W": "#bbccff",
}

# Temperatura range (K) per classe: (min, max) con sub=5 come centro
CLASS_TEMP_RANGE = {
    "O": (30000, 60000),
    "B": (10000, 30000),
    "A": (7500,  10000),
    "F": (6000,   7500),
    "G": (5200,   6000),
    "K": (3700,   5200),
    "M": (2400,   3700),
    "C": (2500,   3500),
    "S": (2500,   3500),
    "W": (25000,  50000),
    "L": (1300,   2400),
    "T": (700,    1300),
}

LUMINOSITY_DESC = {
    "Ia":  "supergigante luminosa",
    "Iab": "supergigante intermedia",
    "Ib":  "supergigante meno luminosa",
    "II":  "gigante brillante",
    "III": "gigante",
    "IV":  "subgigante",
    "V":   "stella di sequenza principale (nana)",
    "VI":  "subnana",
    "VII": "nana bianca",
}

# ── Parser ───────────────────────────────────────────────────────────────────

def decode_spectral_type(sptype: str) -> dict:
    """
    Decodifica un tipo spettrale MKK grezzo in un dizionario strutturato.
    Gestisce formati come: G8III, M1-M2 Ia-Iab, B3Ve, K0IIIb, ecc.
    Ritorna sempre un dict, anche per input malformati.
    """
    result = {
        "spettro_grezzo":          sptype or "",
        "classe_spettrale":        None,
        "sottoclasse":             None,
        "classe_luminosita":       None,
        "descrizione_classe":      None,
        "descrizione_luminosita":  None,
        "colori":                  [],
        "colore_hex":              None,
        "temperatura_stimata_k":   None,
    }

    if not sptype or not sptype.strip():
        return result

    s = sptype.strip()

    # Gestisce range tipo "M1-M2": prende il primo
    s = s.split("-")[0]

    # Classe spettrale principale
    class_match = re.match(r'^([OBAFGKMCSWRLT])', s, re.IGNORECASE)
    if not class_match:
        return result

    cls = class_match.group(1).upper()
    result["classe_spettrale"]   = cls
    result["descrizione_classe"] = CLASS_DESC.get(cls)
    result["colori"]             = CLASS_COLORS.get(cls, [])
    result["colore_hex"]         = CLASS_COLOR_HEX.get(cls)

    # Sottoclasse numerica 0-9 (opzionale decimale)
    sub_match = re.search(r'(\d(?:\.\d)?)', s)
    sub = float(sub_match.group(1)) if sub_match else None
    result["sottoclasse"] = sub

    # Temperatura stimata per interpolazione lineare
    result["temperatura_stimata_k"] = _estimate_temperature(cls, sub)

    # Classe di luminosità (Ia, Iab, Ib, II, III, IV, V, VI, VII)
    lum_match = re.search(r'\b(I{1,3}ab?|IV|V?I{0,2})\b', s)
    if lum_match:
        lum = lum_match.group(1)
        result["classe_luminosita"]      = lum
        result["descrizione_luminosita"] = LUMINOSITY_DESC.get(lum)

    return result


def _estimate_temperature(cls: str, sub: Optional[float]) -> Optional[int]:
    if cls not in CLASS_TEMP_RANGE:
        return None
    lo, hi = CLASS_TEMP_RANGE[cls]
    if sub is None:
        return int((lo + hi) / 2)
    # sub=0 → hi, sub=9 → lo
    return int(hi - (hi - lo) * (sub / 9.0))


def build_spectral_text(decoded: dict) -> str:
    """
    Produce una frase in linguaggio naturale da includere nel testo
    da embeddare, garantendo sempre la presenza di colore e tipo.
    """
    parts = []

    if decoded["descrizione_luminosita"] and decoded["descrizione_classe"]:
        parts.append(
            f"È una {decoded['descrizione_luminosita']} "
            f"classificata come {decoded['descrizione_classe']}."
        )
    elif decoded["descrizione_classe"]:
        parts.append(f"È classificata come {decoded['descrizione_classe']}.")

    colori = decoded.get("colori", [])
    if colori:
        parts.append(f"Il suo colore è {colori[0]}.")

    if decoded["temperatura_stimata_k"]:
        parts.append(
            f"La temperatura superficiale stimata è circa "
            f"{decoded['temperatura_stimata_k']:,} kelvin."
        )

    return " ".join(parts)


# ── Sinonimi per il classificatore LangGraph ─────────────────────────────────
# Usato da node_classify per mappare termini linguistici → filtri Qdrant

SINONIMI_COLORE: dict[str, list[str]] = {
    "rossa":         ["M", "C", "S"],
    "rosse":         ["M", "C", "S"],
    "arancione":     ["K"],
    "arancioni":     ["K"],
    "gialla":        ["G"],
    "gialle":        ["G"],
    "bianca":        ["A", "F"],
    "bianche":       ["A", "F"],
    "azzurra":       ["O", "B"],
    "azzurre":       ["O", "B"],
    "blu":           ["O", "B"],
}

SINONIMI_LUMINOSITA: dict[str, list[str]] = {
    "nana":          ["V", "VI"],
    "nane":          ["V", "VI"],
    "subgigante":    ["IV"],
    "gigante":       ["III", "II"],
    "giganti":       ["III", "II"],
    "supergigante":  ["Ia", "Iab", "Ib"],
    "supergiganti":  ["Ia", "Iab", "Ib"],
    "nana bianca":   ["VII"],
    "nane bianche":  ["VII"],
    "tipo solare":   ["V"],   # + classe G nel classify
}
