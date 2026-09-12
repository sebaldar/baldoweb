"""
agent/state.py
==============
Definisce lo stato condiviso. Aggiornato per includere il contesto 
geografico, temporale e meteorologico (Realismo Totale).
"""

from typing import Optional, Annotated, List, Dict
from typing_extensions import TypedDict
import operator


class BaldoState(TypedDict):
    # --- INPUT ---
    prompt_originale: str
    client_ip: str
    eta_bambino: int
    lunghezza: str
    lingua: str

    # --- ANALISI PROMPT (Nodo 1) ---
    personaggi: List[str]
    emozioni: List[str]
    ambientazione: str
    
    # Nuovi campi per il realismo
    luogo: str                            # Es: "Roma", "Milano", "Parigi"
    data_storia: str                      # Es: "02-03-2024"
    ora_storia: str                       # Es: "21:30"
    
    prompt_chiaro: bool                   
    motivo_rifiuto: Optional[str]          

    # --- DECISIONI AGENTICHE ---
    usa_astronomia: bool                   
    termini_ricerca_neo4j: List[str]       
    tentativi_neo4j: int                   

    # --- DATI RACCOLTI (Nodo 4a e 4b) ---
    frammenti: List[Dict]
    qualita_frammenti: str                 # "buona" | "scarsa" | "assente"
    personaggi_bio: Dict[str, Dict]        # nome -> {description, traits}, solo se presenti
    
    # Nuovi campi per coordinate e meteo
    lat: Optional[float]                   # Latitudine reale da GeoService
    lon: Optional[float]                   # Longitudine reale da GeoService
    dati_meteo: Optional[str]              # Es: "pioggia leggera, 12°C"
    dati_astronomici: Optional[Dict]       # Dati dal server Node/C++

    # --- GENERAZIONE ---
    prompt_arricchito: str
    draft: str
    valutazione_draft: str                 # "ok" | "spaventoso" | "inadeguato" | "troppo_lungo"
    tentativi_correzione: int

    # --- OUTPUT FINALE ---
    racconto_finale: str
    frammenti_usati: List[str]

    # --- MEMORIA ---
    storia_id: Optional[str]              
    storie_precedenti: List[Dict]          

    # --- CONTROLLO FLUSSO ---
    errore: Optional[str]
    iterazioni_totali: Annotated[int, operator.add]
