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

    # Dati di personalizzazione dal form (opzionali): oggi finiscono già
    # intrecciati nel prompt arricchito lato frontend, ma servono anche qui
    # come campi distinti per il report YAML amministrativo della storia.
    nome: Optional[str]
    colore_preferito: Optional[str]
    animale_preferito: Optional[str]

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
    # Termini di jargon della KB (per il controllo, senza LLM, che non
    # trapelino letteralmente nel racconto finale — un bambino non sa cosa
    # significhi "capovolgimento" o "binomio fantastico").
    tecnica_narrativa_kb: Optional[str]
    archetipo_kb: Optional[str]
    draft: str
    # Ritornello REALMENTE usato nel draft (rilevato per codice, non LLM,
    # cercando una frase che si ripete 2+ volte — non il testo grezzo del
    # frammento: quello può contenere un nome proprio della fiaba d'origine
    # — es. "Mangiafuoco" — che il draft, lasciato libero, ha già sostituito
    # con i personaggi della propria trama). Passato a rifinisci come
    # vincolo da preservare parola per parola. None se non rilevato.
    ritornello_atteso: Optional[str]
    valutazione_draft: str                 # "ok" | "spaventoso" | "inadeguato" | "troppo_lungo"
    tentativi_correzione: int

    # --- OUTPUT FINALE ---
    racconto_finale: str
    frammenti_usati: List[str]
    # None se non c'era un "nome" personalizzato da verificare; altrimenti
    # True/False a seconda che compaia parola-per-parola nel racconto finale.
    nome_presente_in_output: Optional[bool]
    # Valori di tecnica_narrativa_kb/archetipo_kb trapelati alla lettera nel
    # racconto finale (lista vuota se nessuno, o se non c'era nulla da
    # controllare).
    termini_kb_trapelati: List[str]
    # Stima approssimativa (conteggio di "come" comparativi, non una vera
    # analisi semantica) delle similitudini nel racconto finale — solo
    # osservabilità per il report, non usata per correggere il testo: il
    # tetto vero è la Regola 20 nel prompt di generazione.
    similitudini_stimate: Optional[int]

    # --- MEMORIA ---
    storia_id: Optional[str]              
    storie_precedenti: List[Dict]          

    # --- CONTROLLO FLUSSO ---
    errore: Optional[str]
    iterazioni_totali: Annotated[int, operator.add]

    # Traccia di ogni chiamata LLM del grafo (nodo, modello, token in/out) —
    # ogni nodo restituisce la propria voce in una lista di un elemento, il
    # reducer le concatena in ordine di esecuzione. Usata per il report YAML
    # amministrativo della storia (modello utilizzato, token consumati).
    llm_usage: Annotated[List[Dict], operator.add]
