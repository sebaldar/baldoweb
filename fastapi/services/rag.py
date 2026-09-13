"""
RAGExtractor
============
Analizza il prompt dell'utente ed estrae entità narrative e fisiche (Meteo/Astro).
"""

import json
import logging
from services.llm import LLMRouter

logger = logging.getLogger(__name__)

SYSTEM_ANALISI = """
Sei un esperto estrattore di entità per storie per bambini.
Analizza il prompt ed estrae i dati richiesti.
IMPORTANTE: Se l'utente menziona città o luoghi (es. "a Parigi", "sotto il Colosseo"), estraili nel campo 'luogo'.

Formato risposta JSON ESCLUSIVO:
{
  "personaggi": ["lista"],
  "emozioni": ["lista"],
  "ambientazione": "stringa",
  "luogo": "nome città o null",
  "data_storia": "DD-MM-YYYY o null",
  "ora_storia": "HH:MM:SS o null"
}

Se mancano luogo, data o ora, lascia null. Rispondi solo con il JSON.
"""

class RAGExtractor:
    def __init__(self, llm: LLMRouter):
        self.llm = llm

    async def analizza(self, prompt: str) -> dict:
        risultato = await self.llm.chiedi(
            system=SYSTEM_ANALISI,
            user=f"Analizza questo prompt: {prompt}",
        )
        # Riportato dentro il dict (chiave "_uso_llm") invece di cambiare la
        # firma in una tupla: il chiamante (analizza_prompt) lo estrae per il
        # report YAML della storia, tutti gli altri consumer ignorano una
        # chiave in più senza modifiche.
        uso_llm = {
            "nodo": "analizza_prompt",
            "modello": risultato.modello,
            "token_input": risultato.token_input,
            "token_output": risultato.token_output,
        }

        try:
            # Pulizia markdown e spazi
            testo = risultato.testo.strip().strip("```json").strip("```").strip()
            analisi = json.loads(testo)

            # Normalizzazione con fallback
            return {
                "personaggi": analisi.get("personaggi") or ["un bambino curioso"],
                "emozioni": analisi.get("emozioni") or ["meraviglia"],
                "ambientazione": analisi.get("ambientazione") or "un posto magico",
                "luogo": analisi.get("luogo"),
                "data_storia": analisi.get("data_storia"),
                "ora_storia": analisi.get("ora_storia"),
                "_uso_llm": uso_llm,
            }

        except Exception as e:
            logger.error(f"Errore parsing RAG: {e}")
            return {
                "personaggi": ["un bambino curioso"],
                "emozioni": ["meraviglia"],
                "ambientazione": "un posto magico",
                "luogo": None,
                "data_storia": None,
                "ora_storia": None,
                "_uso_llm": uso_llm,
            }
