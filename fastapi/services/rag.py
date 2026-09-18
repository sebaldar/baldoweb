"""
RAGExtractor
============
Analizza il prompt dell'utente ed estrae entità narrative e fisiche (Meteo/Astro).
"""
import json
import logging
from datetime import datetime, timedelta
from zoneinfo import ZoneInfo
from services.llm import LLMRouter

# Le espressioni relative ("stasera", "notte"...) nel prompt sotto sono
# convenzioni orarie italiane (22:00 per "stasera" ecc.) — il riferimento
# "adesso" va quindi calcolato nel fuso dell'utenza (Europe/Rome), non nel
# fuso del container (UTC): altrimenti "stasera" viene risolto rispetto
# all'ora UTC corrente, che di sera differisce di 1-2 ore da quella reale
# in Italia (CET/CEST).
FUSO_UTENZA = ZoneInfo("Europe/Rome")

logger = logging.getLogger(__name__)

SYSTEM_ANALISI = """
Sei un esperto estrattore di entità per storie per bambini.
Analizza il prompt ed estrae i dati richiesti.

La data e ora ATTUALI sono: {data_oggi} alle {ora_adesso}.
Usale come riferimento per risolvere espressioni relative come:
  - "ieri" → {data_ieri}
  - "ieri notte" → {data_ieri} con ora 22:00:00
  - "stanotte" / "questa notte" → {data_oggi} con ora 22:00:00
  - "stamattina" → {data_oggi} con ora 08:00:00
  - "stasera" → {data_oggi} con ora 20:00:00
  - "l'altro ieri" → {data_altroieri}
  - "notte" senza data esplicita → usa la data contestuale con ora 22:00:00
  - "alba" → ora 06:00:00
  - "tramonto" → ora 19:00:00
  - "mezzogiorno" → ora 12:00:00
  - "mezzanotte" → ora 00:00:00

IMPORTANTE:
  - Se l'utente menziona città o luoghi (es. "a Parigi", "sotto il Colosseo"), estraili nel campo 'luogo'.
  - Risolvi SEMPRE le date relative in date assolute DD-MM-YYYY.
  - Risolvi SEMPRE i momenti della giornata in orari HH:MM:SS.
  - Se data e ora sono davvero assenti, lascia null.

Estrai meteo_prompt solo quando il prompt impone condizioni atmosferiche reali nella
scena (anche future nella trama o negative, come senza pioggia). Non dedurre dalla
sola stagione. Non estrarre ipotesi come se piove, domande sul tempo, metafore o
richieste di usare il meteo reale. Conserva una citazione esatta con la condizione.

Formato risposta JSON ESCLUSIVO:
{{
  "personaggi": ["lista"],
  "emozioni": ["lista"],
  "ambientazione": "stringa",
  "luogo": "nome città o null",
  "data_storia": "DD-MM-YYYY o null",
  "ora_storia": "HH:MM:SS o null",
  "meteo_prompt": "citazione esatta delle condizioni meteo imposte o null"
}}
Rispondi solo con il JSON.
"""


class RAGExtractor:
    def __init__(self, llm: LLMRouter):
        self.llm = llm

    async def analizza(self, prompt: str) -> dict:
        # Calcola le date di riferimento al momento della chiamata
        oggi      = datetime.now(FUSO_UTENZA)
        ieri      = oggi - timedelta(days=1)
        altroieri = oggi - timedelta(days=2)

        system = SYSTEM_ANALISI.format(
            data_oggi     = oggi.strftime("%d-%m-%Y"),
            ora_adesso    = oggi.strftime("%H:%M:%S"),
            data_ieri     = ieri.strftime("%d-%m-%Y"),
            data_altroieri= altroieri.strftime("%d-%m-%Y"),
        )

        risultato = await self.llm.chiedi(
            system=system,
            user=f"Analizza questo prompt: {prompt}",
            fase="analizza_prompt",
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
            "durata_secondi": round(risultato.durata_secondi, 2),
        }

        try:
            testo   = risultato.testo.strip().strip("```json").strip("```").strip()
            analisi = json.loads(testo)

            # Backstop deterministico: nonostante l'istruzione esplicita di
            # lasciare null quando il prompt non menziona un luogo, il modello
            # a volte inferisce/inventa comunque una città plausibile (osservato:
            # "Roma" restituito per prompt privi di qualunque riferimento
            # geografico) — un vincolo solo-prompt non basta da solo. Accettiamo
            # quindi il luogo solo se compare davvero, testualmente, nel prompt
            # originale dell'utente.
            luogo_estratto = analisi.get("luogo")
            if luogo_estratto and luogo_estratto.lower() not in prompt.lower():
                logger.warning(
                    f"⚠️ Luogo estratto '{luogo_estratto}' non compare nel prompt "
                    "originale: scartato (probabile inferenza non richiesta)."
                )
                luogo_estratto = None

            return {
                "personaggi":   analisi.get("personaggi")   or ["un bambino curioso"],
                "emozioni":     analisi.get("emozioni")      or ["meraviglia"],
                "ambientazione":analisi.get("ambientazione") or "un posto magico",
                "luogo":        luogo_estratto,
                "meteo_prompt": analisi.get("meteo_prompt"),
                "data_storia":  analisi.get("data_storia"),
                "ora_storia":   analisi.get("ora_storia"),
                "_uso_llm":     uso_llm,
            }

        except Exception as e:
            logger.error(f"Errore parsing RAG: {e}")
            return {
                "personaggi":    ["un bambino curioso"],
                "emozioni":      ["meraviglia"],
                "ambientazione": "un posto magico",
                "luogo":         None,
                "data_storia":   None,
                "ora_storia":    None,
                "_uso_llm":      uso_llm,
            }
