"""
ingest_constellations.py
Ingestion delle 88 costellazioni IAU da Wikipedia.
"""

import asyncio
import logging
from pathlib import Path

from .wikipedia import WikipediaClient
from .qdrant_store import PlanetariumStore

logger = logging.getLogger(__name__)

# ── 88 costellazioni IAU con metadati statici ─────────────────────────────────
# Fonte: IAU — questi dati non cambiano, non serve un file esterno.
# Campi: nome_en, nome_it, abbr, area_deg2, emisfero, mese_migliore, confinanti
# Il mese_migliore è quello in cui la costellazione culmina a mezzanotte (circa).

CONSTELLATIONS = [
    # Costellazioni principali (campione completo a seguire)
    {"nome_en": "Andromeda",     "nome_it": "Andromeda",      "abbr": "And", "area": 722,  "emisfero": "nord",    "mese": "novembre",  "stelle_principali": ["Alpheratz", "Mirach", "Almach"]},
    {"nome_en": "Antlia",        "nome_it": "Macchina Pneumatica", "abbr": "Ant", "area": 239, "emisfero": "sud", "mese": "aprile",    "stelle_principali": []},
    {"nome_en": "Apus",          "nome_it": "Uccello del Paradiso", "abbr": "Aps", "area": 206, "emisfero": "sud", "mese": "luglio",   "stelle_principali": []},
    {"nome_en": "Aquarius",      "nome_it": "Acquario",       "abbr": "Aqr", "area": 980,  "emisfero": "entrambi","mese": "ottobre",   "stelle_principali": ["Sadalsuud", "Sadalmelik"]},
    {"nome_en": "Aquila",        "nome_it": "Aquila",         "abbr": "Aql", "area": 652,  "emisfero": "entrambi","mese": "agosto",    "stelle_principali": ["Altair", "Tarazed", "Alshain"]},
    {"nome_en": "Ara",           "nome_it": "Altare",         "abbr": "Ara", "area": 237,  "emisfero": "sud",    "mese": "luglio",    "stelle_principali": []},
    {"nome_en": "Aries",         "nome_it": "Ariete",         "abbr": "Ari", "area": 441,  "emisfero": "nord",   "mese": "dicembre",  "stelle_principali": ["Hamal", "Sheratan", "Mesarthim"]},
    {"nome_en": "Auriga",        "nome_it": "Auriga",         "abbr": "Aur", "area": 657,  "emisfero": "nord",   "mese": "gennaio",   "stelle_principali": ["Capella", "Menkalinan", "Mahasim"]},
    {"nome_en": "Boötes",        "nome_it": "Boote",          "abbr": "Boo", "area": 907,  "emisfero": "nord",   "mese": "giugno",    "stelle_principali": ["Arcturus", "Izar", "Muphrid"]},
    {"nome_en": "Caelum",        "nome_it": "Bulino",         "abbr": "Cae", "area": 125,  "emisfero": "sud",    "mese": "gennaio",   "stelle_principali": []},
    {"nome_en": "Camelopardalis","nome_it": "Giraffa",        "abbr": "Cam", "area": 757,  "emisfero": "nord",   "mese": "febbraio",  "stelle_principali": []},
    {"nome_en": "Cancer",        "nome_it": "Cancro",         "abbr": "Cnc", "area": 506,  "emisfero": "nord",   "mese": "marzo",     "stelle_principali": ["Tarf", "Asellus Australis", "Acubens"]},
    {"nome_en": "Canes Venatici","nome_it": "Cani da Caccia", "abbr": "CVn", "area": 465,  "emisfero": "nord",   "mese": "maggio",    "stelle_principali": ["Cor Caroli", "Chara"]},
    {"nome_en": "Canis Major",   "nome_it": "Cane Maggiore",  "abbr": "CMa", "area": 380,  "emisfero": "entrambi","mese": "febbraio", "stelle_principali": ["Sirius", "Adhara", "Wezen", "Mirzam"]},
    {"nome_en": "Canis Minor",   "nome_it": "Cane Minore",    "abbr": "CMi", "area": 183,  "emisfero": "entrambi","mese": "marzo",    "stelle_principali": ["Procyon", "Gomeisa"]},
    {"nome_en": "Capricornus",   "nome_it": "Capricorno",     "abbr": "Cap", "area": 414,  "emisfero": "entrambi","mese": "settembre","stelle_principali": ["Deneb Algedi", "Dabih"]},
    {"nome_en": "Carina",        "nome_it": "Carena",         "abbr": "Car", "area": 494,  "emisfero": "sud",    "mese": "marzo",     "stelle_principali": ["Canopus", "Miaplacidus", "Avior"]},
    {"nome_en": "Cassiopeia",    "nome_it": "Cassiopea",      "abbr": "Cas", "area": 598,  "emisfero": "nord",   "mese": "novembre",  "stelle_principali": ["Schedar", "Caph", "Ruchbah", "Segin", "Navi"]},
    {"nome_en": "Centaurus",     "nome_it": "Centauro",       "abbr": "Cen", "area": 1060, "emisfero": "sud",    "mese": "maggio",    "stelle_principali": ["Rigil Kentaurus", "Hadar", "Menkent"]},
    {"nome_en": "Cepheus",       "nome_it": "Cefeo",          "abbr": "Cep", "area": 588,  "emisfero": "nord",   "mese": "ottobre",   "stelle_principali": ["Alderamin", "Alfirk", "Errai"]},
    {"nome_en": "Cetus",         "nome_it": "Balena",         "abbr": "Cet", "area": 1231, "emisfero": "entrambi","mese": "novembre", "stelle_principali": ["Diphda", "Menkar", "Mira"]},
    {"nome_en": "Chamaeleon",    "nome_it": "Camaleonte",     "abbr": "Cha", "area": 132,  "emisfero": "sud",    "mese": "aprile",    "stelle_principali": []},
    {"nome_en": "Circinus",      "nome_it": "Compasso",       "abbr": "Cir", "area": 93,   "emisfero": "sud",    "mese": "giugno",    "stelle_principali": []},
    {"nome_en": "Columba",       "nome_it": "Colomba",        "abbr": "Col", "area": 270,  "emisfero": "sud",    "mese": "febbraio",  "stelle_principali": ["Phact", "Wazn"]},
    {"nome_en": "Coma Berenices","nome_it": "Chioma di Berenice","abbr":"Com","area": 386,  "emisfero": "nord",   "mese": "maggio",    "stelle_principali": ["Diadem"]},
    {"nome_en": "Corona Australis","nome_it":"Corona Australe","abbr": "CrA","area": 128,  "emisfero": "sud",    "mese": "agosto",    "stelle_principali": ["Meridiana"]},
    {"nome_en": "Corona Borealis","nome_it":"Corona Boreale", "abbr": "CrB", "area": 179,  "emisfero": "nord",   "mese": "giugno",    "stelle_principali": ["Alphecca", "Nusakan"]},
    {"nome_en": "Corvus",        "nome_it": "Corvo",          "abbr": "Crv", "area": 184,  "emisfero": "sud",    "mese": "maggio",    "stelle_principali": ["Gienah", "Kraz", "Algorab", "Minkar"]},
    {"nome_en": "Crater",        "nome_it": "Coppa",          "abbr": "Crt", "area": 282,  "emisfero": "sud",    "mese": "aprile",    "stelle_principali": []},
    {"nome_en": "Crux",          "nome_it": "Croce del Sud",  "abbr": "Cru", "area": 68,   "emisfero": "sud",    "mese": "maggio",    "stelle_principali": ["Acrux", "Mimosa", "Gacrux", "Imai"]},
    {"nome_en": "Cygnus",        "nome_it": "Cigno",          "abbr": "Cyg", "area": 804,  "emisfero": "nord",   "mese": "settembre", "stelle_principali": ["Deneb", "Sadr", "Aljanah", "Albireo"]},
    {"nome_en": "Delphinus",     "nome_it": "Delfino",        "abbr": "Del", "area": 189,  "emisfero": "nord",   "mese": "settembre", "stelle_principali": ["Rotanev", "Sualocin"]},
    {"nome_en": "Dorado",        "nome_it": "Dorado",         "abbr": "Dor", "area": 179,  "emisfero": "sud",    "mese": "gennaio",   "stelle_principali": []},
    {"nome_en": "Draco",         "nome_it": "Dragone",        "abbr": "Dra", "area": 1083, "emisfero": "nord",   "mese": "luglio",    "stelle_principali": ["Eltanin", "Rastaban", "Thuban"]},
    {"nome_en": "Equuleus",      "nome_it": "Cavallino",      "abbr": "Equ", "area": 72,   "emisfero": "nord",   "mese": "settembre", "stelle_principali": []},
    {"nome_en": "Eridanus",      "nome_it": "Eridano",        "abbr": "Eri", "area": 1138, "emisfero": "entrambi","mese": "dicembre", "stelle_principali": ["Achernar", "Cursa", "Zaurak"]},
    {"nome_en": "Fornax",        "nome_it": "Fornace",        "abbr": "For", "area": 398,  "emisfero": "sud",    "mese": "dicembre",  "stelle_principali": []},
    {"nome_en": "Gemini",        "nome_it": "Gemelli",        "abbr": "Gem", "area": 514,  "emisfero": "nord",   "mese": "febbraio",  "stelle_principali": ["Pollux", "Castor", "Alhena", "Wasat"]},
    {"nome_en": "Grus",          "nome_it": "Gru",            "abbr": "Gru", "area": 366,  "emisfero": "sud",    "mese": "ottobre",   "stelle_principali": ["Alnair", "Tiaki"]},
    {"nome_en": "Hercules",      "nome_it": "Ercole",         "abbr": "Her", "area": 1225, "emisfero": "nord",   "mese": "luglio",    "stelle_principali": ["Kornephoros", "Zeta Herculis", "Sarin"]},
    {"nome_en": "Horologium",    "nome_it": "Orologio",       "abbr": "Hor", "area": 249,  "emisfero": "sud",    "mese": "dicembre",  "stelle_principali": []},
    {"nome_en": "Hydra",         "nome_it": "Idra",           "abbr": "Hya", "area": 1303, "emisfero": "entrambi","mese": "aprile",   "stelle_principali": ["Alphard"]},
    {"nome_en": "Hydrus",        "nome_it": "Idra Maschio",   "abbr": "Hyi", "area": 243,  "emisfero": "sud",    "mese": "dicembre",  "stelle_principali": []},
    {"nome_en": "Indus",         "nome_it": "Indiano",        "abbr": "Ind", "area": 294,  "emisfero": "sud",    "mese": "settembre", "stelle_principali": []},
    {"nome_en": "Lacerta",       "nome_it": "Lucertola",      "abbr": "Lac", "area": 201,  "emisfero": "nord",   "mese": "ottobre",   "stelle_principali": []},
    {"nome_en": "Leo",           "nome_it": "Leone",          "abbr": "Leo", "area": 947,  "emisfero": "nord",   "mese": "aprile",    "stelle_principali": ["Regulus", "Denebola", "Algieba", "Zosma"]},
    {"nome_en": "Leo Minor",     "nome_it": "Leone Minore",   "abbr": "LMi", "area": 232,  "emisfero": "nord",   "mese": "aprile",    "stelle_principali": []},
    {"nome_en": "Lepus",         "nome_it": "Lepre",          "abbr": "Lep", "area": 290,  "emisfero": "sud",    "mese": "gennaio",   "stelle_principali": ["Arneb", "Nihal"]},
    {"nome_en": "Libra",         "nome_it": "Bilancia",       "abbr": "Lib", "area": 538,  "emisfero": "entrambi","mese": "giugno",   "stelle_principali": ["Zubeneschamali", "Zubenelgenubi"]},
    {"nome_en": "Lupus",         "nome_it": "Lupo",           "abbr": "Lup", "area": 334,  "emisfero": "sud",    "mese": "giugno",    "stelle_principali": []},
    {"nome_en": "Lynx",          "nome_it": "Lince",          "abbr": "Lyn", "area": 545,  "emisfero": "nord",   "mese": "marzo",     "stelle_principali": []},
    {"nome_en": "Lyra",          "nome_it": "Lira",           "abbr": "Lyr", "area": 286,  "emisfero": "nord",   "mese": "agosto",    "stelle_principali": ["Vega", "Sheliak", "Sulafat"]},
    {"nome_en": "Mensa",         "nome_it": "Mensa",          "abbr": "Men", "area": 153,  "emisfero": "sud",    "mese": "gennaio",   "stelle_principali": []},
    {"nome_en": "Microscopium",  "nome_it": "Microscopio",    "abbr": "Mic", "area": 210,  "emisfero": "sud",    "mese": "settembre", "stelle_principali": []},
    {"nome_en": "Monoceros",     "nome_it": "Unicorno",       "abbr": "Mon", "area": 482,  "emisfero": "entrambi","mese": "febbraio", "stelle_principali": []},
    {"nome_en": "Musca",         "nome_it": "Mosca",          "abbr": "Mus", "area": 138,  "emisfero": "sud",    "mese": "maggio",    "stelle_principali": []},
    {"nome_en": "Norma",         "nome_it": "Squadra",        "abbr": "Nor", "area": 165,  "emisfero": "sud",    "mese": "luglio",    "stelle_principali": []},
    {"nome_en": "Octans",        "nome_it": "Ottante",        "abbr": "Oct", "area": 291,  "emisfero": "sud",    "mese": "ottobre",   "stelle_principali": []},
    {"nome_en": "Ophiuchus",     "nome_it": "Ofiuco",         "abbr": "Oph", "area": 948,  "emisfero": "entrambi","mese": "luglio",   "stelle_principali": ["Rasalhague", "Sabik", "Han"]},
    {"nome_en": "Orion",         "nome_it": "Orione",         "abbr": "Ori", "area": 594,  "emisfero": "entrambi","mese": "gennaio",  "stelle_principali": ["Rigel", "Betelgeuse", "Bellatrix", "Mintaka", "Alnilam", "Alnitak", "Saiph"]},
    {"nome_en": "Pavo",          "nome_it": "Pavone",         "abbr": "Pav", "area": 378,  "emisfero": "sud",    "mese": "agosto",    "stelle_principali": ["Peacock"]},
    {"nome_en": "Pegasus",       "nome_it": "Pegaso",         "abbr": "Peg", "area": 1121, "emisfero": "nord",   "mese": "ottobre",   "stelle_principali": ["Enif", "Scheat", "Markab", "Algenib"]},
    {"nome_en": "Perseus",       "nome_it": "Perseo",         "abbr": "Per", "area": 615,  "emisfero": "nord",   "mese": "dicembre",  "stelle_principali": ["Mirfak", "Algol", "Atik"]},
    {"nome_en": "Phoenix",       "nome_it": "Fenice",         "abbr": "Phe", "area": 469,  "emisfero": "sud",    "mese": "novembre",  "stelle_principali": ["Ankaa"]},
    {"nome_en": "Pictor",        "nome_it": "Pittore",        "abbr": "Pic", "area": 247,  "emisfero": "sud",    "mese": "febbraio",  "stelle_principali": []},
    {"nome_en": "Pisces",        "nome_it": "Pesci",          "abbr": "Psc", "area": 889,  "emisfero": "nord",   "mese": "novembre",  "stelle_principali": ["Eta Piscium"]},
    {"nome_en": "Piscis Austrinus","nome_it":"Pesce Australe","abbr": "PsA", "area": 245,  "emisfero": "sud",    "mese": "ottobre",   "stelle_principali": ["Fomalhaut"]},
    {"nome_en": "Puppis",        "nome_it": "Poppa",          "abbr": "Pup", "area": 673,  "emisfero": "sud",    "mese": "febbraio",  "stelle_principali": ["Naos", "Azmidi"]},
    {"nome_en": "Pyxis",         "nome_it": "Bussola",        "abbr": "Pyx", "area": 221,  "emisfero": "sud",    "mese": "marzo",     "stelle_principali": []},
    {"nome_en": "Reticulum",     "nome_it": "Reticolo",       "abbr": "Ret", "area": 114,  "emisfero": "sud",    "mese": "gennaio",   "stelle_principali": []},
    {"nome_en": "Sagitta",       "nome_it": "Freccia",        "abbr": "Sge", "area": 80,   "emisfero": "nord",   "mese": "agosto",    "stelle_principali": []},
    {"nome_en": "Sagittarius",   "nome_it": "Sagittario",     "abbr": "Sgr", "area": 867,  "emisfero": "entrambi","mese": "agosto",   "stelle_principali": ["Kaus Australis", "Nunki", "Ascella"]},
    {"nome_en": "Scorpius",      "nome_it": "Scorpione",      "abbr": "Sco", "area": 497,  "emisfero": "entrambi","mese": "luglio",   "stelle_principali": ["Antares", "Shaula", "Sargas", "Dschubba"]},
    {"nome_en": "Sculptor",      "nome_it": "Scultore",       "abbr": "Scl", "area": 475,  "emisfero": "sud",    "mese": "novembre",  "stelle_principali": []},
    {"nome_en": "Scutum",        "nome_it": "Scudo",          "abbr": "Sct", "area": 109,  "emisfero": "entrambi","mese": "agosto",   "stelle_principali": []},
    {"nome_en": "Serpens",       "nome_it": "Serpente",       "abbr": "Ser", "area": 637,  "emisfero": "entrambi","mese": "luglio",   "stelle_principali": ["Unukalhai"]},
    {"nome_en": "Sextans",       "nome_it": "Sestante",       "abbr": "Sex", "area": 314,  "emisfero": "entrambi","mese": "aprile",   "stelle_principali": []},
    {"nome_en": "Taurus",        "nome_it": "Toro",           "abbr": "Tau", "area": 797,  "emisfero": "nord",   "mese": "gennaio",   "stelle_principali": ["Aldebaran", "Elnath", "Alcyone"]},
    {"nome_en": "Telescopium",   "nome_it": "Telescopio",     "abbr": "Tel", "area": 252,  "emisfero": "sud",    "mese": "agosto",    "stelle_principali": []},
    {"nome_en": "Triangulum",    "nome_it": "Triangolo",      "abbr": "Tri", "area": 132,  "emisfero": "nord",   "mese": "dicembre",  "stelle_principali": ["Mothallah"]},
    {"nome_en": "Triangulum Australe","nome_it":"Triangolo Australe","abbr":"TrA","area":110,"emisfero":"sud",    "mese": "luglio",    "stelle_principali": ["Atria"]},
    {"nome_en": "Tucana",        "nome_it": "Tucano",         "abbr": "Tuc", "area": 295,  "emisfero": "sud",    "mese": "novembre",  "stelle_principali": []},
    {"nome_en": "Ursa Major",    "nome_it": "Orsa Maggiore",  "abbr": "UMa", "area": 1280, "emisfero": "nord",   "mese": "aprile",    "stelle_principali": ["Alioth", "Dubhe", "Merak", "Mizar", "Alcor"]},
    {"nome_en": "Ursa Minor",    "nome_it": "Orsa Minore",    "abbr": "UMi", "area": 256,  "emisfero": "nord",   "mese": "giugno",    "stelle_principali": ["Polaris", "Kochab", "Pherkad"]},
    {"nome_en": "Vela",          "nome_it": "Vela",           "abbr": "Vel", "area": 500,  "emisfero": "sud",    "mese": "marzo",     "stelle_principali": ["Gamma Velorum", "Alsephina"]},
    {"nome_en": "Virgo",         "nome_it": "Vergine",        "abbr": "Vir", "area": 1294, "emisfero": "entrambi","mese": "maggio",   "stelle_principali": ["Spica", "Porrima", "Vindemiatrix"]},
    {"nome_en": "Volans",        "nome_it": "Pesce Volante",  "abbr": "Vol", "area": 141,  "emisfero": "sud",    "mese": "febbraio",  "stelle_principali": []},
    {"nome_en": "Vulpecula",     "nome_it": "Volpetta",       "abbr": "Vul", "area": 268,  "emisfero": "nord",   "mese": "settembre", "stelle_principali": []},
]


def build_constellation_text(c: dict, wiki_summary: str | None) -> str:
    parts = []

    if wiki_summary:
        parts.append(wiki_summary[:1200])
    else:
        parts.append(
            f"{c['nome_it']} ({c['nome_en']}, abbreviazione {c['abbr']}) "
            f"è una costellazione dell'emisfero {c['emisfero']}."
        )

    if c["stelle_principali"]:
        parti_stelle = ", ".join(c["stelle_principali"])
        parts.append(f"Le sue stelle principali sono: {parti_stelle}.")

    parts.append(
        f"Occupa un'area di circa {c['area']} gradi quadrati. "
        f"Il periodo migliore per l'osservazione è {c['mese']}."
    )

    return " ".join(parts)


async def ingest_constellations(
    store: PlanetariumStore,
    wiki_cache_dir: Path,
) -> dict:
    logger.info(f"[CONST] Ingestion {len(CONSTELLATIONS)} costellazioni…")

    stats = {"totale": len(CONSTELLATIONS), "con_wikipedia": 0, "inserite": 0}

    async with WikipediaClient(wiki_cache_dir) as wiki_client:
        requests = [
            (c["nome_en"], [f"{c['nome_en']} constellation"])
            for c in CONSTELLATIONS
        ]
        wiki_results = await wiki_client.get_batch(requests)

    documents = []
    for c, wiki in zip(CONSTELLATIONS, wiki_results):
        if wiki:
            stats["con_wikipedia"] += 1

        text = build_constellation_text(c, wiki["summary"] if wiki else None)

        documents.append({
            "tipo":                 "CONSTELLATIONS",
            "nome":                 c["nome_en"],
            "text":                 text,
            "nome_italiano":        c["nome_it"],
            "abbreviazione":        c["abbr"],
            "area_gradi_quadri":    c["area"],
            "emisfero":             c["emisfero"],
            "mese_osservazione":    c["mese"],
            "stelle_principali":    c["stelle_principali"],
            "has_wikipedia":        wiki is not None,
            "wikipedia_summary":    wiki["summary"] if wiki else None,
            "wikipedia_url":        wiki["url"] if wiki else None,
            "wikipedia_thumbnail":  wiki["thumbnail"] if wiki else None,
        })

    stats["inserite"] = store.upsert(documents)
    logger.info(f"[CONST] ✅ {stats['inserite']} costellazioni inserite")
    return stats
