"""
LLMRouter
=========
Usa Claude o OpenAI (in ordine di preferenza).
Tenta Claude (Sonnet 5) come provider principale, OpenAI come fallback automatico.
"""

import asyncio
import logging
import re
import openai
import anthropic

from config import settings

logger = logging.getLogger(__name__)

# Nessuno dei due client aveva un timeout esplicito: quello di default di
# Anthropic è 10 minuti (troppo lungo per restare "appesi" in un nodo del
# grafo senza che scatti alcun fallback), e OpenAI non ne aveva affatto.
# Osservato in produzione: una richiesta bloccata silenziosamente subito
# dopo una chiamata Claude riuscita, senza errori né timeout per oltre 6
# minuti — il nodo successivo del grafo non partiva mai e l'utente restava
# senza risposta. asyncio.wait_for aggiunge un limite anche per blocchi non
# di rete (es. contesa su un lock interno), che un timeout sul solo client
# HTTP non coprirebbe.
TIMEOUT_LLM_SECONDI = 45.0

# Il testo finisce incollato direttamente in innerHTML dal frontend (nessun
# rendering Markdown): qualunque sintassi Markdown comparirebbe come testo
# letterale con i simboli (es. "*Ops!*" invece di "Ops!" in corsivo, "---"
# invece di una riga vuota). Rimossa qui come rete di sicurezza, anche se il
# prompt istruisce già il modello a non usarla.
_TITOLO_MARKDOWN_RE = re.compile(r"^\s*#{1,6}\s.+\n+")
_ENFASI_MARKDOWN_RE = re.compile(r"\*\*(.+?)\*\*|\*(.+?)\*")
_SEPARATORE_MARKDOWN_RE = re.compile(r"^\s*-{3,}\s*$", re.MULTILINE)


class LLMRouter:

    def __init__(self):
        self.openai_client = (
            openai.AsyncOpenAI(api_key=settings.OPENAI_API_KEY, timeout=TIMEOUT_LLM_SECONDI)
            if settings.OPENAI_API_KEY else None
        )
        self.anthropic_client = (
            anthropic.AsyncAnthropic(api_key=settings.ANTHROPIC_API_KEY, timeout=TIMEOUT_LLM_SECONDI)
            if settings.ANTHROPIC_API_KEY else None
        )

        if not self.openai_client and not self.anthropic_client:
            raise RuntimeError("Nessun LLM configurato. Imposta OPENAI_API_KEY o ANTHROPIC_API_KEY nel .env")

        provider = "Claude" if self.anthropic_client else "OpenAI"
        logger.info(f"LLMRouter pronto — provider principale: {provider}")

    async def chiedi(self, system: str, user: str) -> str:
        """Chiamata generica per RAG, analisi e valutazioni."""
        return await self._cloud_chat(system=system, user=user)

    async def genera_racconto(self, prompt: str) -> str:
        """Genera il draft del racconto tramite cloud LLM."""
        system = (
            "Sei Baldo, un anziano astrologo e cantastorie gentile che vive in una "
            "torre antica — questa cornice (chi sei, da dove racconti) va sempre "
            "mantenuta: il messaggio dell'utente contiene le istruzioni esatte su "
            "cosa vedi in cielo in questo momento (giorno o notte, meteo reale) — "
            "seguile alla lettera, non inventare un cielo stellato di default. "
            "Usa un linguaggio semplice, immagini vivide, ritmo narrativo "
            "coinvolgente, adatto a bambini in età prescolare. "
            "Non iniziare mai con un titolo o un'intestazione, e non usare ALCUNA "
            "formattazione Markdown (niente *, **, #, --- o simili): il testo va "
            "incollato così com'è in una pagina web, senza rendering Markdown — "
            "se vuoi enfasi o un'onomatopea, scrivila in testo semplice."
        )
        return await self._cloud_chat(system=system, user=prompt)

    async def rifinisci(self, draft: str, eta: int) -> str:
        """Rifinitura editoriale finale del racconto."""
        system = (
            f"Sei un editor esperto di letteratura per l'infanzia. "
            f"Il racconto è destinato a bambini di circa {eta} anni. "
            f"Raffina il testo mantenendone lo spirito: migliora il ritmo, "
            f"semplifica dove necessario, usa solo parole che un bambino di "
            f"quell'età conosce già (evita aggettivi astratti o letterari come "
            f"\"viscido\", \"ambiguo\"). "
            f"NON toccare la cornice di apertura/chiusura di Baldo (la torre, il "
            f"cielo, il modo in cui si rivolge al bambino) se è già presente nel "
            f"testo — lasciala intatta, non sostituirla con una formula fiabesca "
            f"generica (mai \"vissero felici e contenti\" o simili, tanto più al "
            f"plurale se il protagonista è uno solo). "
            f"NON far dichiarare a un personaggio o al narratore la morale della "
            f"storia (frasi tipo \"capì una cosa importante\"): se c'è una domanda "
            f"finale rivolta al bambino, deve restare lei a fare quel lavoro. "
            f"Non usare ALCUNA formattazione Markdown (niente *, **, #, ---): il "
            f"testo va incollato così com'è in una pagina web. "
            f"NON aggiungere elementi nuovi alla trama, solo rifinisci."
        )
        return await self._cloud_chat(
            system=system,
            user=f"Raffina questo racconto:\n\n{draft}"
        )

    # ------------------------------------------------------------------
    # Metodo interno
    # ------------------------------------------------------------------

    async def _cloud_chat(self, system: str, user: str) -> str:
        """Tenta Claude (Sonnet 5), poi OpenAI come fallback."""
        if self.anthropic_client:
            try:
                resp = await asyncio.wait_for(
                    self.anthropic_client.messages.create(
                        model=settings.ANTHROPIC_MODEL,
                        # Margine ampio: con il thinking adattivo attivo, un tetto
                        # basso rischia di esaurirsi nel ragionamento interno prima
                        # ancora di produrre il testo visibile (successo in test:
                        # nessun blocco "text" nella risposta, scattato il
                        # fallback). Alzare il tetto non costa di più: si paga
                        # solo per i token davvero generati, non per il tetto.
                        max_tokens=8192,
                        system=system,
                        messages=[{"role": "user", "content": user}],
                        thinking={"type": "adaptive"},
                        # "low": scrittura creativa breve per bambini, non
                        # ragionamento complesso — riduce anche il rischio sopra.
                        output_config={"effort": "low"},
                    ),
                    timeout=TIMEOUT_LLM_SECONDI,
                )
                testo = self._estrai_testo(resp.content)
                if testo:
                    return self._pulisci(testo)
                logger.warning("Claude ha risposto senza blocco di testo, provo OpenAI.")
            except Exception as e:
                logger.warning(f"Claude fallito ({e}), provo OpenAI.")

        if self.openai_client:
            try:
                resp = await asyncio.wait_for(
                    self.openai_client.chat.completions.create(
                        model=settings.OPENAI_MODEL,
                        messages=[
                            {"role": "system", "content": system},
                            {"role": "user", "content": user},
                        ],
                        temperature=0.8,
                    ),
                    timeout=TIMEOUT_LLM_SECONDI,
                )
                return self._pulisci(resp.choices[0].message.content)
            except Exception as e:
                logger.error(f"Anche OpenAI fallito ({e}) — nessun LLM disponibile per questa richiesta.")
                raise RuntimeError(f"Entrambi i provider LLM non disponibili: {e}") from e

        raise RuntimeError("Nessun LLM disponibile — Claude e OpenAI entrambi offline.")

    @staticmethod
    def _pulisci(testo: str) -> str:
        testo = _TITOLO_MARKDOWN_RE.sub("", testo, count=1)
        testo = _SEPARATORE_MARKDOWN_RE.sub("", testo)
        testo = _ENFASI_MARKDOWN_RE.sub(lambda m: m.group(1) or m.group(2), testo)
        return testo.strip()

    @staticmethod
    def _estrai_testo(content_blocks) -> str:
        """
        Con il thinking adattivo attivo, il primo blocco della risposta può
        essere un blocco di ragionamento (senza attributo .text) invece del
        testo vero e proprio — va cercato il blocco di tipo "text", non
        preso semplicemente il primo (content_blocks[0]).
        """
        for block in content_blocks:
            if getattr(block, "type", None) == "text":
                return block.text
        return ""
