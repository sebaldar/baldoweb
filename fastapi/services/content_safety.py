"""Filtro deterministico per contenuti inadatti ai bambini.

Rete di sicurezza che non dipende dal giudice LLM: copre solo termini
inequivocabili (sesso, autolesionismo, insulti discriminatori, droghe). La
violenza da fiaba (lupi, draghi, streghe) non è inclusa di proposito.
"""
import json
import re

_TERMINI = (
    r"sess(?:o|uale|uali)", r"porn\w*", r"nud[oaie]", r"erotic\w*", r"stupr\w*",
    r"molest\w*", r"pedofil\w*", r"masturb\w*", r"orgasm\w*",
    r"suicid\w*", r"ucciders[ie]", r"uccidermi", r"togliermi la vita",
    r"farmi del male", r"tagliarmi", r"autolesion\w*",
    r"cocaina", r"eroina", r"marijuana", r"spinell\w*", r"overdose",
    r"negr[oa]\s+di\s+merda", r"frocio", r"ricchione", r"mongoloide",
)
_RE = re.compile(r"\b(?:" + "|".join(_TERMINI) + r")\b", re.IGNORECASE)


def find_inappropriate(*texts):
    """Restituisce il primo termine inadatto trovato nei testi, o None."""
    for text in texts:
        if isinstance(text, (list, tuple)):
            text = " ".join(str(item) for item in text)
        match = _RE.search(text) if isinstance(text, str) else None
        if match:
            return match.group(0).lower()
    return None


def parse_verdict(raw):
    """Estrae l'oggetto JSON dal verdetto del giudice, anche dentro ```json."""
    if not isinstance(raw, str):
        return None
    start, end = raw.find("{"), raw.rfind("}")
    if start < 0 or end <= start:
        return None
    try:
        data = json.loads(raw[start:end + 1])
    except ValueError:
        return None
    return data if isinstance(data, dict) else None
