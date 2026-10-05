"""
ingest_solarsystem.py
Ingestion corpi del sistema solare (pianeti, lune, pianeti nani, asteroidi)
e fenomeni astronomici ricorrenti.
"""

import asyncio
import logging
from pathlib import Path

from .wikipedia import WikipediaClient
from .qdrant_store import PlanetariumStore

logger = logging.getLogger(__name__)


# ── Catalogo sistema solare ───────────────────────────────────────────────────
# Dati orbitali/fisici statici — la posizione in tempo reale è calcolata
# dall'engine C++ solar, non dalla KB.

SOLAR_SYSTEM: list[dict] = [

    # ── Pianeti ───────────────────────────────────────────────────────────────
    {
        "sottotipo": "PLANET", "nome": "Mercury", "nome_it": "Mercurio",
        "corpo_padre": None, "comando_planetario": "mercury",
        "visibilita_occhio_nudo": True, "magnitudine_media": -0.5,
        "dati_orbitali": {"semiasse_au": 0.387, "periodo_giorni": 87.97, "eccentricita": 0.206, "inclinazione": 7.0},
        "dati_fisici": {"massa_kg": 3.285e23, "raggio_km": 2439.7, "densita": 5.43, "gravita": 3.7, "temperatura_k": 440, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Mercury (planet)", "Mercury planet"],
    },
    {
        "sottotipo": "PLANET", "nome": "Venus", "nome_it": "Venere",
        "corpo_padre": None, "comando_planetario": "venus",
        "visibilita_occhio_nudo": True, "magnitudine_media": -4.4,
        "dati_orbitali": {"semiasse_au": 0.723, "periodo_giorni": 224.7, "eccentricita": 0.007, "inclinazione": 3.4},
        "dati_fisici": {"massa_kg": 4.867e24, "raggio_km": 6051.8, "densita": 5.24, "gravita": 8.87, "temperatura_k": 737, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Venus", "Venus (planet)"],
    },
    {
        "sottotipo": "PLANET", "nome": "Earth", "nome_it": "Terra",
        "corpo_padre": None, "comando_planetario": "earth",
        "visibilita_occhio_nudo": True, "magnitudine_media": None,
        "dati_orbitali": {"semiasse_au": 1.0, "periodo_giorni": 365.25, "eccentricita": 0.017, "inclinazione": 0.0},
        "dati_fisici": {"massa_kg": 5.972e24, "raggio_km": 6371.0, "densita": 5.51, "gravita": 9.81, "temperatura_k": 288, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Earth"],
    },
    {
        "sottotipo": "PLANET", "nome": "Mars", "nome_it": "Marte",
        "corpo_padre": None, "comando_planetario": "mars",
        "visibilita_occhio_nudo": True, "magnitudine_media": 0.71,
        "dati_orbitali": {"semiasse_au": 1.524, "periodo_giorni": 686.97, "eccentricita": 0.093, "inclinazione": 1.85},
        "dati_fisici": {"massa_kg": 6.39e23, "raggio_km": 3389.5, "densita": 3.93, "gravita": 3.72, "temperatura_k": 210, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Mars", "Mars (planet)"],
    },
    {
        "sottotipo": "PLANET", "nome": "Jupiter", "nome_it": "Giove",
        "corpo_padre": None, "comando_planetario": "jupiter",
        "visibilita_occhio_nudo": True, "magnitudine_media": -2.5,
        "dati_orbitali": {"semiasse_au": 5.203, "periodo_giorni": 4332.59, "eccentricita": 0.049, "inclinazione": 1.30},
        "dati_fisici": {"massa_kg": 1.898e27, "raggio_km": 71492.0, "densita": 1.33, "gravita": 24.79, "temperatura_k": 165, "ha_anelli": True, "ha_atmosfera": True},
        "lune_note": ["Io", "Europa", "Ganymede", "Callisto"],
        "wiki_titles": ["Jupiter", "Jupiter (planet)"],
    },
    {
        "sottotipo": "PLANET", "nome": "Saturn", "nome_it": "Saturno",
        "corpo_padre": None, "comando_planetario": "saturn",
        "visibilita_occhio_nudo": True, "magnitudine_media": 0.46,
        "dati_orbitali": {"semiasse_au": 9.537, "periodo_giorni": 10759.0, "eccentricita": 0.057, "inclinazione": 2.49},
        "dati_fisici": {"massa_kg": 5.683e26, "raggio_km": 58232.0, "densita": 0.69, "gravita": 10.44, "temperatura_k": 134, "ha_anelli": True, "ha_atmosfera": True},
        "lune_note": ["Titan", "Enceladus", "Mimas", "Rhea", "Dione", "Tethys"],
        "wiki_titles": ["Saturn", "Saturn (planet)"],
    },
    {
        "sottotipo": "PLANET", "nome": "Uranus", "nome_it": "Urano",
        "corpo_padre": None, "comando_planetario": "uranus",
        "visibilita_occhio_nudo": True, "magnitudine_media": 5.68,
        "dati_orbitali": {"semiasse_au": 19.19, "periodo_giorni": 30688.5, "eccentricita": 0.046, "inclinazione": 0.77},
        "dati_fisici": {"massa_kg": 8.681e25, "raggio_km": 25362.0, "densita": 1.27, "gravita": 8.87, "temperatura_k": 76, "ha_anelli": True, "ha_atmosfera": True},
        "lune_note": ["Titania", "Oberon", "Miranda", "Ariel", "Umbriel"],
        "wiki_titles": ["Uranus", "Uranus (planet)"],
    },
    {
        "sottotipo": "PLANET", "nome": "Neptune", "nome_it": "Nettuno",
        "corpo_padre": None, "comando_planetario": "neptune",
        "visibilita_occhio_nudo": False, "magnitudine_media": 7.83,
        "dati_orbitali": {"semiasse_au": 30.07, "periodo_giorni": 60182.0, "eccentricita": 0.010, "inclinazione": 1.77},
        "dati_fisici": {"massa_kg": 1.024e26, "raggio_km": 24622.0, "densita": 1.64, "gravita": 11.15, "temperatura_k": 72, "ha_anelli": True, "ha_atmosfera": True},
        "lune_note": ["Triton", "Nereid", "Proteus"],
        "wiki_titles": ["Neptune", "Neptune (planet)"],
    },

    # ── Pianeti nani ──────────────────────────────────────────────────────────
    {
        "sottotipo": "DWARF", "nome": "Pluto", "nome_it": "Plutone",
        "corpo_padre": None, "comando_planetario": "pluto",
        "visibilita_occhio_nudo": False, "magnitudine_media": 14.0,
        "dati_orbitali": {"semiasse_au": 39.48, "periodo_giorni": 90560.0, "eccentricita": 0.249, "inclinazione": 17.14},
        "dati_fisici": {"massa_kg": 1.303e22, "raggio_km": 1188.3, "densita": 1.85, "gravita": 0.62, "temperatura_k": 44, "ha_anelli": False, "ha_atmosfera": True},
        "lune_note": ["Charon", "Nix", "Hydra"],
        "wiki_titles": ["Pluto", "Pluto (dwarf planet)"],
    },
    {
        "sottotipo": "DWARF", "nome": "Eris", "nome_it": "Eris",
        "corpo_padre": None, "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 18.7,
        "dati_orbitali": {"semiasse_au": 67.78, "periodo_giorni": 203830.0, "eccentricita": 0.434, "inclinazione": 44.04},
        "dati_fisici": {"massa_kg": 1.66e22, "raggio_km": 1163.0, "densita": 2.43, "gravita": 0.82, "temperatura_k": 30, "ha_anelli": False, "ha_atmosfera": False},
        "wiki_titles": ["Eris (dwarf planet)"],
    },
    {
        "sottotipo": "DWARF", "nome": "Ceres", "nome_it": "Cerere",
        "corpo_padre": None, "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 6.6,
        "dati_orbitali": {"semiasse_au": 2.77, "periodo_giorni": 1681.0, "eccentricita": 0.076, "inclinazione": 10.59},
        "dati_fisici": {"massa_kg": 9.384e20, "raggio_km": 476.2, "densita": 2.16, "gravita": 0.27, "temperatura_k": 168, "ha_anelli": False, "ha_atmosfera": False},
        "wiki_titles": ["Ceres (dwarf planet)", "Ceres asteroid"],
    },
    {
        "sottotipo": "DWARF", "nome": "Makemake", "nome_it": "Makemake",
        "corpo_padre": None, "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 16.7,
        "dati_orbitali": {"semiasse_au": 45.43, "periodo_giorni": 111845.0, "eccentricita": 0.162, "inclinazione": 28.96},
        "dati_fisici": {"massa_kg": 3.1e21, "raggio_km": 715.0, "densita": 2.0, "gravita": 0.4, "temperatura_k": 30, "ha_anelli": False, "ha_atmosfera": False},
        "wiki_titles": ["Makemake", "Makemake (dwarf planet)"],
    },
    {
        "sottotipo": "DWARF", "nome": "Haumea", "nome_it": "Haumea",
        "corpo_padre": None, "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 17.3,
        "dati_orbitali": {"semiasse_au": 43.13, "periodo_giorni": 103774.0, "eccentricita": 0.195, "inclinazione": 28.21},
        "dati_fisici": {"massa_kg": 4.006e21, "raggio_km": 816.0, "densita": 2.0, "gravita": 0.44, "temperatura_k": 32, "ha_anelli": True, "ha_atmosfera": False},
        "wiki_titles": ["Haumea", "Haumea (dwarf planet)"],
    },

    # ── Lune principali ───────────────────────────────────────────────────────
    {
        "sottotipo": "MOON", "nome": "Moon", "nome_it": "Luna",
        "corpo_padre": "Earth", "comando_planetario": "moon",
        "visibilita_occhio_nudo": True, "magnitudine_media": -12.6,
        "dati_orbitali": {"semiasse_au": 0.00257, "periodo_giorni": 27.32, "eccentricita": 0.055, "inclinazione": 5.14},
        "dati_fisici": {"massa_kg": 7.342e22, "raggio_km": 1737.4, "densita": 3.34, "gravita": 1.62, "temperatura_k": 250, "ha_anelli": False, "ha_atmosfera": False},
        "wiki_titles": ["Moon", "Luna (moon)"],
    },
    {
        "sottotipo": "MOON", "nome": "Titan", "nome_it": "Titano",
        "corpo_padre": "Saturn", "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 8.4,
        "dati_orbitali": {"semiasse_au": 0.00817, "periodo_giorni": 15.95, "eccentricita": 0.029, "inclinazione": 0.33},
        "dati_fisici": {"massa_kg": 1.345e23, "raggio_km": 2574.7, "densita": 1.88, "gravita": 1.35, "temperatura_k": 94, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Titan (moon)", "Titan Saturn moon"],
    },
    {
        "sottotipo": "MOON", "nome": "Europa", "nome_it": "Europa",
        "corpo_padre": "Jupiter", "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 5.3,
        "dati_orbitali": {"semiasse_au": 0.00448, "periodo_giorni": 3.55, "eccentricita": 0.009, "inclinazione": 0.47},
        "dati_fisici": {"massa_kg": 4.8e22, "raggio_km": 1560.8, "densita": 3.01, "gravita": 1.31, "temperatura_k": 102, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Europa (moon)", "Europa Jupiter moon"],
    },
    {
        "sottotipo": "MOON", "nome": "Ganymede", "nome_it": "Ganimede",
        "corpo_padre": "Jupiter", "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 4.6,
        "dati_orbitali": {"semiasse_au": 0.00715, "periodo_giorni": 7.15, "eccentricita": 0.001, "inclinazione": 0.18},
        "dati_fisici": {"massa_kg": 1.482e23, "raggio_km": 2634.1, "densita": 1.94, "gravita": 1.43, "temperatura_k": 110, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Ganymede (moon)"],
    },
    {
        "sottotipo": "MOON", "nome": "Io", "nome_it": "Io",
        "corpo_padre": "Jupiter", "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 5.0,
        "dati_orbitali": {"semiasse_au": 0.00282, "periodo_giorni": 1.77, "eccentricita": 0.004, "inclinazione": 0.04},
        "dati_fisici": {"massa_kg": 8.932e22, "raggio_km": 1821.6, "densita": 3.53, "gravita": 1.80, "temperatura_k": 130, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Io (moon)", "Io Jupiter moon"],
    },
    {
        "sottotipo": "MOON", "nome": "Enceladus", "nome_it": "Encelado",
        "corpo_padre": "Saturn", "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 11.7,
        "dati_orbitali": {"semiasse_au": 0.00159, "periodo_giorni": 1.37, "eccentricita": 0.005, "inclinazione": 0.02},
        "dati_fisici": {"massa_kg": 1.08e20, "raggio_km": 252.1, "densita": 1.61, "gravita": 0.11, "temperatura_k": 75, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Enceladus (moon)"],
    },
    {
        "sottotipo": "MOON", "nome": "Triton", "nome_it": "Tritone",
        "corpo_padre": "Neptune", "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 13.5,
        "dati_orbitali": {"semiasse_au": 0.00237, "periodo_giorni": 5.88, "eccentricita": 0.000016, "inclinazione": 156.9},
        "dati_fisici": {"massa_kg": 2.139e22, "raggio_km": 1353.4, "densita": 2.06, "gravita": 0.78, "temperatura_k": 38, "ha_anelli": False, "ha_atmosfera": True},
        "wiki_titles": ["Triton (moon)", "Triton Neptune moon"],
    },

    # ── Asteroidi notevoli ────────────────────────────────────────────────────
    {
        "sottotipo": "ASTEROID", "nome": "Vesta", "nome_it": "Vesta",
        "corpo_padre": None, "comando_planetario": None,
        "visibilita_occhio_nudo": True, "magnitudine_media": 5.4,
        "dati_orbitali": {"semiasse_au": 2.362, "periodo_giorni": 1325.75, "eccentricita": 0.089, "inclinazione": 7.14},
        "dati_fisici": {"massa_kg": 2.59e20, "raggio_km": 262.7, "densita": 3.46, "gravita": 0.25, "temperatura_k": 188, "ha_anelli": False, "ha_atmosfera": False},
        "wiki_titles": ["4 Vesta", "Vesta asteroid"],
    },
    {
        "sottotipo": "ASTEROID", "nome": "Pallas", "nome_it": "Pallade",
        "corpo_padre": None, "comando_planetario": None,
        "visibilita_occhio_nudo": False, "magnitudine_media": 6.5,
        "dati_orbitali": {"semiasse_au": 2.772, "periodo_giorni": 1686.0, "eccentricita": 0.231, "inclinazione": 34.84},
        "dati_fisici": {"massa_kg": 2.04e20, "raggio_km": 256.0, "densita": 3.0, "gravita": 0.21, "temperatura_k": 166, "ha_anelli": False, "ha_atmosfera": False},
        "wiki_titles": ["2 Pallas", "Pallas asteroid"],
    },
]


# ── Fenomeni astronomici ──────────────────────────────────────────────────────

PHENOMENA: list[dict] = [
    {
        "nome": "Perseidi", "nome_it": "Perseidi",
        "sottotipo": "METEOR_SHOWER",
        "ricorrenza": "annuale", "mese_tipico": "agosto",
        "corpo_coinvolto": ["Perseus", "Swift-Tuttle"],
        "wiki_titles": ["Perseids", "Perseid meteor shower"],
        "note": "Picco intorno al 12 agosto. Fino a 100 meteore/ora in condizioni ideali.",
    },
    {
        "nome": "Leonidi", "nome_it": "Leonidi",
        "sottotipo": "METEOR_SHOWER",
        "ricorrenza": "annuale", "mese_tipico": "novembre",
        "corpo_coinvolto": ["Leo", "Tempel-Tuttle"],
        "wiki_titles": ["Leonids", "Leonid meteor shower"],
        "note": "Picco intorno al 17 novembre. Ogni 33 anni produce tempeste di meteore.",
    },
    {
        "nome": "Geminidi", "nome_it": "Geminidi",
        "sottotipo": "METEOR_SHOWER",
        "ricorrenza": "annuale", "mese_tipico": "dicembre",
        "corpo_coinvolto": ["Gemini", "3200 Phaethon"],
        "wiki_titles": ["Geminids", "Geminid meteor shower"],
        "note": "Una delle piogge più intense. Picco 13-14 dicembre, fino a 120 meteore/ora.",
    },
    {
        "nome": "Quadrantidi", "nome_it": "Quadrantidi",
        "sottotipo": "METEOR_SHOWER",
        "ricorrenza": "annuale", "mese_tipico": "gennaio",
        "corpo_coinvolto": ["Boötes", "2003 EH1"],
        "wiki_titles": ["Quadrantids", "Quadrantid meteor shower"],
        "note": "Picco brevissimo (6 ore) intorno al 3-4 gennaio.",
    },
    {
        "nome": "Eclisse solare totale", "nome_it": "Eclisse solare totale",
        "sottotipo": "ECLIPSE",
        "ricorrenza": "variabile", "mese_tipico": None,
        "corpo_coinvolto": ["Sun", "Moon"],
        "wiki_titles": ["Solar eclipse", "Total solar eclipse"],
        "note": "Si verifica quando la Luna copre completamente il disco solare.",
    },
    {
        "nome": "Eclisse lunare totale", "nome_it": "Eclisse lunare totale",
        "sottotipo": "ECLIPSE",
        "ricorrenza": "variabile", "mese_tipico": None,
        "corpo_coinvolto": ["Moon", "Earth"],
        "wiki_titles": ["Lunar eclipse", "Total lunar eclipse"],
        "note": "La Luna entra nell'ombra della Terra, assumendo una colorazione rossastra.",
    },
    {
        "nome": "Opposizione di Marte", "nome_it": "Opposizione di Marte",
        "sottotipo": "OPPOSITION",
        "ricorrenza": "ogni ~26 mesi", "mese_tipico": None,
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Mars opposition", "Opposition of Mars"],
        "note": "Marte è opposto al Sole: massima visibilità e dimensione apparente.",
    },
    {
        "nome": "Opposizione di Giove", "nome_it": "Opposizione di Giove",
        "sottotipo": "OPPOSITION",
        "ricorrenza": "annuale", "mese_tipico": None,
        "corpo_coinvolto": ["Jupiter"],
        "wiki_titles": ["Jupiter opposition"],
        "note": "Giove è alla minima distanza dalla Terra: ideale per l'osservazione.",
    },
    {
        "nome": "Transito di Venere", "nome_it": "Transito di Venere",
        "sottotipo": "TRANSIT",
        "ricorrenza": "raro (cicli 8+121.5 anni)", "mese_tipico": None,
        "corpo_coinvolto": ["Venus", "Sun"],
        "wiki_titles": ["Transit of Venus"],
        "note": "Venere transita di fronte al disco solare. Prossimo nel 2117.",
    },
    {
        "nome": "Aurora Boreale", "nome_it": "Aurora Boreale",
        "sottotipo": "AURORA",
        "ricorrenza": "variabile", "mese_tipico": None,
        "corpo_coinvolto": ["Sun", "Earth"],
        "wiki_titles": ["Aurora borealis", "Northern lights"],
        "note": "Causata da particelle solari che interagono con il campo magnetico terrestre.",
    },
]


# ── Builder documenti ─────────────────────────────────────────────────────────

def build_solarsystem_text(body: dict, wiki_summary: str | None) -> str:
    parts = []
    nome = body["nome_it"] or body["nome"]

    if wiki_summary:
        parts.append(wiki_summary[:1200])
    else:
        tipo_desc = {
            "PLANET":   "un pianeta del Sistema Solare",
            "DWARF":    "un pianeta nano del Sistema Solare",
            "MOON":     f"una luna di {body.get('corpo_padre', 'un pianeta')}",
            "ASTEROID": "un asteroide della Fascia Principale",
        }.get(body["sottotipo"], "un corpo celeste del Sistema Solare")
        parts.append(f"{nome} è {tipo_desc}.")

    f = body.get("dati_fisici", {})
    o = body.get("dati_orbitali", {})

    if f.get("raggio_km"):
        parts.append(f"Ha un raggio di circa {f['raggio_km']:,.0f} km.")
    if o.get("periodo_giorni"):
        parts.append(f"Completa un'orbita in {o['periodo_giorni']:.1f} giorni.")
    if f.get("temperatura_k"):
        parts.append(f"La temperatura media è circa {f['temperatura_k']} K.")
    if body.get("visibilita_occhio_nudo"):
        parts.append("È visibile a occhio nudo.")
    if f.get("ha_anelli"):
        parts.append("Possiede un sistema di anelli.")
    lune = body.get("lune_note", [])
    if lune:
        parts.append(f"Tra le sue lune principali: {', '.join(lune)}.")

    return " ".join(parts)


def build_phenomenon_text(ph: dict, wiki_summary: str | None) -> str:
    parts = []
    if wiki_summary:
        parts.append(wiki_summary[:1200])
    else:
        parts.append(f"{ph['nome_it']} è un fenomeno astronomico.")
    if ph.get("note"):
        parts.append(ph["note"])
    if ph.get("ricorrenza"):
        parts.append(f"Ricorrenza: {ph['ricorrenza']}.")
    if ph.get("mese_tipico"):
        parts.append(f"Si verifica tipicamente a {ph['mese_tipico']}.")
    return " ".join(parts)


# ── Missioni spaziali ─────────────────────────────────────────────────────────

SPACE_MISSIONS: list[dict] = [

    # ── Sbarchi sulla Luna ────────────────────────────────────────────────────
    {
        "nome": "Apollo 11", "nome_it": "Apollo 11",
        "agenzia": "NASA", "programma": "Apollo",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "1969-07-16", "data_arrivo": "1969-07-20",
        "data_fine": "1969-07-24", "attiva": False,
        "equipaggio": ["Neil Armstrong", "Buzz Aldrin", "Michael Collins"],
        "obiettivo": "Primo sbarco umano sulla Luna",
        "scoperte_chiave": "Prima camminata umana sulla Luna nel Mare della Tranquillità",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Apollo 11"],
    },
    {
        "nome": "Apollo 12", "nome_it": "Apollo 12",
        "agenzia": "NASA", "programma": "Apollo",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "1969-11-14", "data_arrivo": "1969-11-19",
        "data_fine": "1969-11-24", "attiva": False,
        "equipaggio": ["Charles Conrad", "Alan Bean", "Richard Gordon"],
        "obiettivo": "Secondo sbarco lunare, atterraggio di precisione vicino a Surveyor 3",
        "scoperte_chiave": "Recupero di componenti di Surveyor 3, campioni dall'Oceano delle Tempeste",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Apollo 12"],
    },
    {
        "nome": "Apollo 13", "nome_it": "Apollo 13",
        "agenzia": "NASA", "programma": "Apollo",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "1970-04-11", "data_arrivo": None,
        "data_fine": "1970-04-17", "attiva": False,
        "equipaggio": ["James Lovell", "Jack Swigert", "Fred Haise"],
        "obiettivo": "Terzo sbarco lunare — abortito per esplosione serbatoio ossigeno",
        "scoperte_chiave": "Rientro di emergenza di successo, dimostrazione della resilienza NASA",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Apollo 13"],
    },
    {
        "nome": "Apollo 17", "nome_it": "Apollo 17",
        "agenzia": "NASA", "programma": "Apollo",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "1972-12-07", "data_arrivo": "1972-12-11",
        "data_fine": "1972-12-19", "attiva": False,
        "equipaggio": ["Eugene Cernan", "Harrison Schmitt", "Ronald Evans"],
        "obiettivo": "Ultima missione Apollo con equipaggio sulla Luna",
        "scoperte_chiave": "Rocce arancioni nel cratere Shorty, campioni più antichi recuperati",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Apollo 17"],
    },

    # ── Rover marziani ────────────────────────────────────────────────────────
    {
        "nome": "Sojourner", "nome_it": "Sojourner",
        "agenzia": "NASA", "programma": "Mars Pathfinder",
        "destinazione": "Mars", "tipo_missione": "rover",
        "data_lancio": "1996-12-04", "data_arrivo": "1997-07-04",
        "data_fine": "1997-09-27", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Primo rover su Marte, dimostrazione tecnologica",
        "scoperte_chiave": "Prove di antico ambiente umido, analisi composizione rocce",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Sojourner (rover)", "Mars Pathfinder"],
    },
    {
        "nome": "Spirit", "nome_it": "Spirit",
        "agenzia": "NASA", "programma": "Mars Exploration Rover",
        "destinazione": "Mars", "tipo_missione": "rover",
        "data_lancio": "2003-06-10", "data_arrivo": "2004-01-04",
        "data_fine": "2010-05-25", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Studio della geologia e del passato acquoso di Marte",
        "scoperte_chiave": "Prove di attività idrotermale nel cratere Gusev, suolo ricco di silice",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Spirit (rover)", "Mars Exploration Rover"],
    },
    {
        "nome": "Opportunity", "nome_it": "Opportunity",
        "agenzia": "NASA", "programma": "Mars Exploration Rover",
        "destinazione": "Mars", "tipo_missione": "rover",
        "data_lancio": "2003-07-07", "data_arrivo": "2004-01-25",
        "data_fine": "2018-06-10", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Studio di Meridiani Planum, ricerca prove di acqua passata",
        "scoperte_chiave": "Ematite sferica (mirtilli) prova di acqua liquida, percorso record 45 km",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Opportunity (rover)"],
    },
    {
        "nome": "Curiosity", "nome_it": "Curiosity",
        "agenzia": "NASA", "programma": "Mars Science Laboratory",
        "destinazione": "Mars", "tipo_missione": "rover",
        "data_lancio": "2011-11-26", "data_arrivo": "2012-08-06",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Valutare abitabilità passata nel cratere Gale, studio clima e geologia",
        "scoperte_chiave": "Composti organici rilevati, metano stagionale, conferma acqua liquida passata",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Curiosity (rover)", "Mars Science Laboratory"],
    },
    {
        "nome": "Perseverance", "nome_it": "Perseverance",
        "agenzia": "NASA", "programma": "Mars 2020",
        "destinazione": "Mars", "tipo_missione": "rover",
        "data_lancio": "2020-07-30", "data_arrivo": "2021-02-18",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Ricerca di biosegnature nel cratere Jezero, raccolta campioni per ritorno",
        "scoperte_chiave": "Produzione ossigeno con MOXIE, volo dell'elicottero Ingenuity, delta fluviale antico",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Perseverance (rover)", "Mars 2020"],
    },

    # ── Sonde planetarie ──────────────────────────────────────────────────────
    {
        "nome": "Voyager 1", "nome_it": "Voyager 1",
        "agenzia": "NASA", "programma": "Voyager",
        "destinazione": "Interstellar", "tipo_missione": "flyby",
        "data_lancio": "1977-09-05", "data_arrivo": "1979-03-05",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Studio di Giove, Saturno e spazio interstellare",
        "scoperte_chiave": "Vulcani su Io, anelli di Giove, prima sonda a lasciare l'eliosfera (2012)",
        "corpo_coinvolto": ["Jupiter", "Saturn"],
        "wiki_titles": ["Voyager 1"],
    },
    {
        "nome": "Voyager 2", "nome_it": "Voyager 2",
        "agenzia": "NASA", "programma": "Voyager",
        "destinazione": "Interstellar", "tipo_missione": "flyby",
        "data_lancio": "1977-08-20", "data_arrivo": "1979-07-09",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Unica sonda a visitare tutti e quattro i giganti gassosi",
        "scoperte_chiave": "Grande macchia scura su Nettuno, geyser su Tritone, anelli di Urano",
        "corpo_coinvolto": ["Jupiter", "Saturn", "Uranus", "Neptune"],
        "wiki_titles": ["Voyager 2"],
    },
    {
        "nome": "Cassini-Huygens", "nome_it": "Cassini-Huygens",
        "agenzia": "NASA/ESA/ASI", "programma": "Cassini-Huygens",
        "destinazione": "Saturn", "tipo_missione": "orbiter",
        "data_lancio": "1997-10-15", "data_arrivo": "2004-07-01",
        "data_fine": "2017-09-15", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Studio approfondito di Saturno, anelli e lune; sonda Huygens su Titano",
        "scoperte_chiave": "Oceano di idrocarburi su Titano, pennacchi d'acqua su Encelado, struttura anelli",
        "corpo_coinvolto": ["Saturn"],
        "wiki_titles": ["Cassini–Huygens", "Cassini-Huygens"],
    },
    {
        "nome": "New Horizons", "nome_it": "New Horizons",
        "agenzia": "NASA", "programma": "New Horizons",
        "destinazione": "Pluto", "tipo_missione": "flyby",
        "data_lancio": "2006-01-19", "data_arrivo": "2015-07-14",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Prima esplorazione ravvicinata di Plutone e Cintura di Kuiper",
        "scoperte_chiave": "Pianure di azoto ghiacciato (Tombaugh Regio), montagne di ghiaccio, atmosfera",
        "corpo_coinvolto": ["Pluto"],
        "wiki_titles": ["New Horizons"],
    },
    {
        "nome": "Juno", "nome_it": "Giunone",
        "agenzia": "NASA", "programma": "New Frontiers",
        "destinazione": "Jupiter", "tipo_missione": "orbiter",
        "data_lancio": "2011-08-05", "data_arrivo": "2016-07-04",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Studio dell'interno, atmosfera e magnetosfera di Giove",
        "scoperte_chiave": "Cicloni polari giganti, interno non uniforme, campo magnetico irregolare",
        "corpo_coinvolto": ["Jupiter"],
        "wiki_titles": ["Juno (spacecraft)"],
    },
    {
        "nome": "Mars Express", "nome_it": "Mars Express",
        "agenzia": "ESA", "programma": "Mars Express",
        "destinazione": "Mars", "tipo_missione": "orbiter",
        "data_lancio": "2003-06-02", "data_arrivo": "2003-12-25",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Mappatura superficie e studio atmosfera marziana",
        "scoperte_chiave": "Ghiaccio d'acqua al polo sud, lago sotterraneo sotto il polo sud",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Mars Express"],
    },
    {
        "nome": "Rosetta", "nome_it": "Rosetta",
        "agenzia": "ESA", "programma": "Rosetta",
        "destinazione": "67P/Churyumov-Gerasimenko", "tipo_missione": "orbiter",
        "data_lancio": "2004-03-02", "data_arrivo": "2014-08-06",
        "data_fine": "2016-09-30", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Prima missione in orbita attorno a una cometa, lander Philae sulla superficie",
        "scoperte_chiave": "Molecole organiche complesse sulla cometa, acqua diversa da quella terrestre",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Rosetta (spacecraft)"],
    },
    {
        "nome": "OSIRIS-REx", "nome_it": "OSIRIS-REx",
        "agenzia": "NASA", "programma": "New Frontiers",
        "destinazione": "Bennu", "tipo_missione": "sample_return",
        "data_lancio": "2016-09-08", "data_arrivo": "2018-12-03",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Raccolta campioni dall'asteroide Bennu e ritorno sulla Terra",
        "scoperte_chiave": "Superficie di Bennu più soffice del previsto, campioni ricchi di carbonio consegnati 2023",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["OSIRIS-REx"],
    },

    # ── Telescopi spaziali ────────────────────────────────────────────────────
    {
        "nome": "Hubble Space Telescope", "nome_it": "Telescopio Spaziale Hubble",
        "agenzia": "NASA/ESA", "programma": "Great Observatories",
        "destinazione": "Earth orbit", "tipo_missione": "space_telescope",
        "data_lancio": "1990-04-24", "data_arrivo": "1990-04-24",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Osservazione astronomica in ottico, UV e infrarosso vicino",
        "scoperte_chiave": "Età dell'universo 13.8 miliardi anni, energia oscura, galassie primordiali",
        "corpo_coinvolto": [],
        "wiki_titles": ["Hubble Space Telescope"],
    },
    {
        "nome": "James Webb Space Telescope", "nome_it": "Telescopio Spaziale James Webb",
        "agenzia": "NASA/ESA/CSA", "programma": "James Webb",
        "destinazione": "L2", "tipo_missione": "space_telescope",
        "data_lancio": "2021-12-25", "data_arrivo": "2022-01-24",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Osservazione infrarosso dell'universo primordiale, atmosfere esopianeti",
        "scoperte_chiave": "Galassie massive nell'universo primordiale, atmosfere esopianeti, prime stelle",
        "corpo_coinvolto": [],
        "wiki_titles": ["James Webb Space Telescope"],
    },
    {
        "nome": "Kepler Space Telescope", "nome_it": "Telescopio Spaziale Kepler",
        "agenzia": "NASA", "programma": "Discovery",
        "destinazione": "Earth orbit", "tipo_missione": "space_telescope",
        "data_lancio": "2009-03-07", "data_arrivo": "2009-03-07",
        "data_fine": "2018-10-30", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Ricerca di esopianeti tramite transiti stellari",
        "scoperte_chiave": "Oltre 2600 esopianeti confermati, molti in zona abitabile",
        "corpo_coinvolto": [],
        "wiki_titles": ["Kepler space telescope"],
    },

    # ── Missioni lunari recenti ───────────────────────────────────────────────
    {
        "nome": "Chang'e 4", "nome_it": "Chang'e 4",
        "agenzia": "CNSA", "programma": "Chang'e",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "2018-12-07", "data_arrivo": "2019-01-03",
        "data_fine": None, "attiva": True,
        "equipaggio": [],
        "obiettivo": "Primo atterraggio sul lato nascosto della Luna",
        "scoperte_chiave": "Esplorazione del bacino Von Kármán, manto lunare esposto in superficie",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Chang'e 4"],
    },
    {
        "nome": "Artemis I", "nome_it": "Artemis I",
        "agenzia": "NASA", "programma": "Artemis",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "2022-11-16", "data_arrivo": "2022-11-21",
        "data_fine": "2022-12-11", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Primo volo di prova non equipaggiato di SLS e Orion attorno alla Luna",
        "scoperte_chiave": "Validazione sistema SLS/Orion per futuri voli con equipaggio",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Artemis I"],
    },

    # ── Missioni iconiche storiche ─────────────────────────────────────────────
    {
        "nome": "Sputnik 1", "nome_it": "Sputnik 1",
        "agenzia": "URSS", "programma": "Sputnik",
        "destinazione": "Earth orbit", "tipo_missione": "orbiter",
        "data_lancio": "1957-10-04", "data_arrivo": "1957-10-04",
        "data_fine": "1958-01-04", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Primo satellite artificiale della Terra",
        "scoperte_chiave": "Inizio dell'era spaziale, prima trasmissione radio dallo spazio",
        "corpo_coinvolto": [],
        "wiki_titles": ["Sputnik 1"],
    },
    {
        "nome": "Luna 9", "nome_it": "Luna 9",
        "agenzia": "URSS", "programma": "Luna",
        "destinazione": "Moon", "tipo_missione": "lander",
        "data_lancio": "1966-01-31", "data_arrivo": "1966-02-03",
        "data_fine": "1966-02-06", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Primo atterraggio morbido sulla Luna",
        "scoperte_chiave": "Prime fotografie dalla superficie lunare, suolo non polveroso come temuto",
        "corpo_coinvolto": ["Moon"],
        "wiki_titles": ["Luna 9"],
    },
    {
        "nome": "Pioneer 10", "nome_it": "Pioneer 10",
        "agenzia": "NASA", "programma": "Pioneer",
        "destinazione": "Jupiter", "tipo_missione": "flyby",
        "data_lancio": "1972-03-02", "data_arrivo": "1973-12-03",
        "data_fine": "2003-01-23", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Prima sonda ad attraversare la fascia degli asteroidi e raggiungere Giove",
        "scoperte_chiave": "Prima misurazione ravvicinata di Giove, porta con messaggio per extraterrestri",
        "corpo_coinvolto": ["Jupiter"],
        "wiki_titles": ["Pioneer 10"],
    },
    {
        "nome": "Mariner 4", "nome_it": "Mariner 4",
        "agenzia": "NASA", "programma": "Mariner",
        "destinazione": "Mars", "tipo_missione": "flyby",
        "data_lancio": "1964-11-28", "data_arrivo": "1965-07-14",
        "data_fine": "1967-12-21", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Primo flyby ravvicinato di Marte, prime fotografie",
        "scoperte_chiave": "Superficie craterizzata, atmosfera molto rarefatta, niente campo magnetico forte",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Mariner 4"],
    },
    {
        "nome": "Viking 1", "nome_it": "Viking 1",
        "agenzia": "NASA", "programma": "Viking",
        "destinazione": "Mars", "tipo_missione": "lander",
        "data_lancio": "1975-08-20", "data_arrivo": "1976-07-20",
        "data_fine": "1982-11-13", "attiva": False,
        "equipaggio": [],
        "obiettivo": "Primo atterraggio di successo su Marte, ricerca di vita",
        "scoperte_chiave": "Nessuna prova conclusiva di vita, composizione atmosfera, foto superficie",
        "corpo_coinvolto": ["Mars"],
        "wiki_titles": ["Viking 1"],
    },
]


# ── Builder testo missioni ────────────────────────────────────────────────────

def build_mission_text(m: dict, wiki_summary: str | None) -> str:
    parts = []
    nome = m.get("nome_it") or m["nome"]

    if wiki_summary:
        parts.append(wiki_summary[:1200])
    else:
        tipo_desc = {
            "rover":          "un rover",
            "lander":         "una sonda di atterraggio",
            "orbiter":        "una sonda in orbita",
            "flyby":          "una sonda di sorvolo",
            "sample_return":  "una missione di ritorno campioni",
            "space_telescope": "un telescopio spaziale",
        }.get(m["tipo_missione"], "una missione spaziale")
        dest = m.get("destinazione", "")
        agenzia = m.get("agenzia", "")
        parts.append(f"{nome} è {tipo_desc} {agenzia} diretta verso {dest}.")

    if m.get("data_lancio"):
        parts.append(f"È stata lanciata il {m['data_lancio']}.")
    if m.get("data_arrivo"):
        parts.append(f"Ha raggiunto la destinazione il {m['data_arrivo']}.")
    if m.get("data_fine") and not m.get("attiva"):
        parts.append(f"La missione si è conclusa il {m['data_fine']}.")
    elif m.get("attiva"):
        parts.append("La missione è ancora attiva.")
    if m.get("obiettivo"):
        parts.append(f"Obiettivo: {m['obiettivo']}.")
    if m.get("scoperte_chiave"):
        parts.append(f"Scoperte principali: {m['scoperte_chiave']}.")
    if m.get("equipaggio"):
        parts.append(f"Equipaggio: {', '.join(m['equipaggio'])}.")

    return " ".join(parts)


# ── Entry points ──────────────────────────────────────────────────────────────

async def ingest_solarsystem(
    store: PlanetariumStore,
    wiki_cache_dir: Path,
) -> dict:
    logger.info(f"[SS] Ingestion {len(SOLAR_SYSTEM)} corpi del sistema solare…")
    stats = {"totale": len(SOLAR_SYSTEM), "con_wikipedia": 0, "inserite": 0}

    async with WikipediaClient(wiki_cache_dir) as wiki_client:
        requests = [(b["wiki_titles"][0], b["wiki_titles"][1:]) for b in SOLAR_SYSTEM]
        wiki_results = await wiki_client.get_batch(requests)

    documents = []
    for body, wiki in zip(SOLAR_SYSTEM, wiki_results):
        if wiki:
            stats["con_wikipedia"] += 1
        text = build_solarsystem_text(body, wiki["summary"] if wiki else None)

        doc = {
            "tipo":                 "SOLARSYSTEM",
            "nome":                 body["nome"],
            "text":                 text,
            "sottotipo":            body["sottotipo"],
            "nome_italiano":        body.get("nome_it"),
            "corpo_padre":          body.get("corpo_padre"),
            "comando_planetario":   body.get("comando_planetario"),
            "visibilita_occhio_nudo": body.get("visibilita_occhio_nudo", False),
            "magnitudine_media":    body.get("magnitudine_media"),
            "lune_note":            body.get("lune_note", []),
            "has_wikipedia":        wiki is not None,
            "wikipedia_summary":    wiki["summary"] if wiki else None,
            "wikipedia_url":        wiki["url"] if wiki else None,
            "wikipedia_thumbnail":  wiki["thumbnail"] if wiki else None,
        }
        # Appiattisci dati orbitali e fisici come campi di primo livello
        for k, v in body.get("dati_orbitali", {}).items():
            doc[f"orb_{k}"] = v
        for k, v in body.get("dati_fisici", {}).items():
            doc[f"fis_{k}"] = v

        documents.append(doc)

    stats["inserite"] = store.upsert(documents)
    logger.info(f"[SS] ✅ {stats['inserite']} corpi inseriti")
    return stats


async def ingest_phenomena(
    store: PlanetariumStore,
    wiki_cache_dir: Path,
) -> dict:
    logger.info(f"[PHE] Ingestion {len(PHENOMENA)} fenomeni…")
    stats = {"totale": len(PHENOMENA), "con_wikipedia": 0, "inserite": 0}

    async with WikipediaClient(wiki_cache_dir) as wiki_client:
        requests = [(p["wiki_titles"][0], p["wiki_titles"][1:]) for p in PHENOMENA]
        wiki_results = await wiki_client.get_batch(requests)

    documents = []
    for ph, wiki in zip(PHENOMENA, wiki_results):
        if wiki:
            stats["con_wikipedia"] += 1
        text = build_phenomenon_text(ph, wiki["summary"] if wiki else None)
        documents.append({
            "tipo":             "PHENOMENA",
            "nome":             ph["nome"],
            "text":             text,
            "sottotipo":        ph["sottotipo"],
            "nome_italiano":    ph.get("nome_it"),
            "ricorrenza":       ph.get("ricorrenza"),
            "mese_tipico":      ph.get("mese_tipico"),
            "corpo_coinvolto":  ph.get("corpo_coinvolto", []),
            "note":             ph.get("note"),
            "has_wikipedia":    wiki is not None,
            "wikipedia_summary": wiki["summary"] if wiki else None,
            "wikipedia_url":    wiki["url"] if wiki else None,
        })

    stats["inserite"] = store.upsert(documents)
    logger.info(f"[PHE] ✅ {stats['inserite']} fenomeni inseriti")
    return stats


async def ingest_missions(
    store: PlanetariumStore,
    wiki_cache_dir: Path,
) -> dict:
    logger.info(f"[MSN] Ingestion {len(SPACE_MISSIONS)} missioni spaziali…")
    stats = {"totale": len(SPACE_MISSIONS), "con_wikipedia": 0, "inserite": 0}

    async with WikipediaClient(wiki_cache_dir) as wiki_client:
        requests = [(m["wiki_titles"][0], m["wiki_titles"][1:]) for m in SPACE_MISSIONS]
        wiki_results = await wiki_client.get_batch(requests)

    documents = []
    for m, wiki in zip(SPACE_MISSIONS, wiki_results):
        if wiki:
            stats["con_wikipedia"] += 1
        text = build_mission_text(m, wiki["summary"] if wiki else None)
        documents.append({
            "tipo":             "PHENOMENA",
            "nome":             m["nome"],
            "text":             text,
            "sottotipo":        "SPACE_MISSION",
            "nome_italiano":    m.get("nome_it"),
            "agenzia":          m.get("agenzia"),
            "programma":        m.get("programma"),
            "destinazione":     m.get("destinazione"),
            "tipo_missione":    m.get("tipo_missione"),
            "data_lancio":      m.get("data_lancio"),
            "data_arrivo":      m.get("data_arrivo"),
            "data_fine":        m.get("data_fine"),
            "attiva":           m.get("attiva", False),
            "equipaggio":       m.get("equipaggio", []),
            "obiettivo":        m.get("obiettivo"),
            "scoperte_chiave":  m.get("scoperte_chiave"),
            "corpo_coinvolto":  m.get("corpo_coinvolto", []),
            "ricorrenza":       None,
            "mese_tipico":      None,
            "has_wikipedia":    wiki is not None,
            "wikipedia_summary": wiki["summary"] if wiki else None,
            "wikipedia_url":    wiki["url"] if wiki else None,
            "wikipedia_thumbnail": wiki["thumbnail"] if wiki else None,
        })

    stats["inserite"] = store.upsert(documents)
    logger.info(f"[MSN] ✅ {stats['inserite']} missioni inserite "
                f"({stats['con_wikipedia']} con Wikipedia)")
    return stats
