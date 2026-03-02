"""
LLMRouter
=========
Usa OpenAI o Claude (in ordine di preferenza).
Tenta OpenAI come provider principale, Claude come fallback automatico.
"""

import logging
import openai
import anthropic

from config import settings

logger = logging.getLogger(__name__)


class LLMRouter:

    def __init__(self):
        self.openai_client = (
            openai.AsyncOpenAI(api_key=settings.OPENAI_API_KEY)
            if settings.OPENAI_API_KEY else None
        )
        self.anthropic_client = (
            anthropic.AsyncAnthropic(api_key=settings.ANTHROPIC_API_KEY)
            if settings.ANTHROPIC_API_KEY else None
        )

        if not self.openai_client and not self.anthropic_client:
            raise RuntimeError("Nessun LLM configurato. Imposta OPENAI_API_KEY o ANTHROPIC_API_KEY nel .env")

        provider = "OpenAI" if self.openai_client else "Claude"
        logger.info(f"LLMRouter pronto — provider principale: {provider}")

    async def chiedi(self, system: str, user: str) -> str:
        """Chiamata generica per RAG, analisi e valutazioni."""
        return await self._cloud_chat(system=system, user=user)

    async def genera_racconto(self, prompt: str) -> str:
        """Genera il draft del racconto tramite cloud LLM."""
        system = (
            "Sei un contastorie poetico e fantasioso per bambini in età prescolare. "
            "Usa un linguaggio semplice, immagini vivide, ritmo narrativo coinvolgente. "
            "Incorpora elementi astronomici in modo magico e meraviglioso."
        )
        return await self._cloud_chat(system=system, user=prompt)

    async def rifinisci(self, draft: str, eta: int) -> str:
        """Rifinitura editoriale finale del racconto."""
        system = (
            f"Sei un editor esperto di letteratura per l'infanzia. "
            f"Il racconto è destinato a bambini di circa {eta} anni. "
            f"Raffina il testo mantenendone lo spirito: migliora il ritmo, "
            f"semplifica dove necessario, rendi la conclusione più soddisfacente. "
            f"NON aggiungere elementi nuovi, solo rifinisci."
        )
        return await self._cloud_chat(
            system=system,
            user=f"Raffina questo racconto:\n\n{draft}"
        )

    # ------------------------------------------------------------------
    # Metodo interno
    # ------------------------------------------------------------------

    async def _cloud_chat(self, system: str, user: str) -> str:
        """Tenta OpenAI, poi Claude come fallback."""
        if self.openai_client:
            try:
                resp = await self.openai_client.chat.completions.create(
                    model=settings.OPENAI_MODEL,
                    messages=[
                        {"role": "system", "content": system},
                        {"role": "user", "content": user},
                    ],
                    temperature=0.8,
                )
                return resp.choices[0].message.content
            except Exception as e:
                logger.warning(f"OpenAI fallito ({e}), provo Claude.")

        if self.anthropic_client:
            resp = await self.anthropic_client.messages.create(
                model=settings.ANTHROPIC_MODEL,
                max_tokens=2048,
                system=system,
                messages=[{"role": "user", "content": user}],
            )
            return resp.content[0].text

        raise RuntimeError("Nessun LLM disponibile — OpenAI e Claude entrambi offline.")
