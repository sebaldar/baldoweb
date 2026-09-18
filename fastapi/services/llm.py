"""
LLMRouter
=========
Instrada ogni chiamata verso il provider configurato per la sua fase
(services/model_routing.py — modificabile a runtime dall'admin), con
Claude e OpenAI come catena di ripiego fissa se il provider scelto non è
disponibile o fallisce. Un provider non richiesto esplicitamente per una
fase non viene mai usato come ripiego automatico: DeepSeek entra in gioco
solo dove l'admin lo ha assegnato di persona, non come sostituto silenzioso
di Claude/OpenAI quando quelli falliscono.
"""

import asyncio
import logging
import re
import time
from dataclasses import dataclass
from typing import Optional
import openai
import anthropic

from config import settings
from services.model_routing import ModelRoutingConfig

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

# Le sei categorie sono pattern verificabili nel testo (uno stesso costrutto
# ripeso, la stessa apertura di frase...), non un giudizio estetico generico:
# così il modello individua solo problemi realmente presenti invece di
# inventarne per riempire il conteggio richiesto.
HUMANIZE_CRITERIA = (
    "Individua tra 4 e 6 punti in cui il racconto è troppo regolare, meccanico o "
    "prevedibile — scegli i più significativi se ce ne sono di più, e se non ne trovi "
    "almeno 4 restituisci solo quelli reali, senza inventarne per arrivare al numero. "
    "Cerca solo pattern realmente presenti, in queste categorie: "
    "(1) costrutto-stampella: la stessa struttura sintattica riusata più volte per "
    "introdurre un'azione o una sensazione (es. tre frasi che iniziano tutte con "
    "'Nome sentì...'); "
    "(2) apertura di frase monotona: lo stesso avverbio o connettivo in testa a "
    "frasi consecutive o quasi (es. 'Poi' ripetuto come apertura più volte); "
    "(3) aggettivazione a coppia meccanica: un doppione aggettivale usato come tic "
    "ritmico invece che come scelta espressiva (es. 'grande, vecchio, addormentato'); "
    "(4) simmetria di scena: due momenti paralleli della trama raccontati con lo "
    "stesso ordine di verbi e la stessa cadenza, che li rende intercambiabili invece "
    "che distinti nel tono; "
    "(5) dialogo piatto: sempre lo stesso verbo dichiarativo, nessuna esitazione, "
    "interruzione o gesto che accompagni la battuta; "
    "(6) assenza di dettaglio incidentale: ogni frase è strettamente funzionale alla "
    "trama, senza mai un dettaglio sensoriale non necessario all'azione, di quelli "
    "che un narratore userebbe per dare consistenza al mondo; "
    "(7) progressione senza attrito: tre o più successi identici di fila (scoperte, "
    "tentativi, azioni riuscite) senza un momento di esitazione, fallimento "
    "temporaneo o distrazione che la interrompa, rendendo la sequenza meccanicamente "
    "lineare; "
    "(8) assenza di goffaggine: il personaggio esegue ogni azione fisica in modo "
    "perfetto al primo tentativo, senza mai un piccolo errore innocuo (inciampare, "
    "sbagliare mira, lasciar cadere qualcosa) che lo renda più vero; "
    "(9) compiutezza adulta nel comportamento infantile: un bambino piccolo parla o "
    "agisce con una precisione da adulto — frasi sempre complete e ben formate, "
    "azioni sempre efficienti al primo colpo — invece di mostrare le imperfezioni "
    "tipiche dell'età (una frase lasciata a metà, un'esclamazione ripetuta, "
    "un'azione ripetuta senza motivo pratico, come ricontare qualcosa già contato). "
    "Non segnalare un punto se non rientra chiaramente in una di queste categorie. "
    "Non tagliare o sostituire un'immagine solo perché poetica o letteraria: "
    "conserva le immagini riuscite, anche quelle di chiusura — non è un difetto da "
    "correggere qui."
)


@dataclass
class RisultatoLLM:
    """
    Testo generato più i metadati della chiamata — usati per il report YAML
    amministrativo di ogni storia (modello effettivamente risposto, non
    necessariamente quello primario, token consumati, e durata: misurato che
    la latenza segue i token di OUTPUT, non quelli di input — una chiamata
    con migliaia di token in ingresso ma pochi in uscita costa un decimo in
    tempo di una con output lungo, indipendentemente dall'input. durata_secondi
    permette di verificarlo run per run invece che stimarlo, e di capire se
    a pesare di più è genera_draft o rifinisci quando i due divergono).
    """
    testo: str
    modello: str
    token_input: int = 0
    token_output: int = 0
    durata_secondi: float = 0.0


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
        # DeepSeek: stesso SDK di OpenAI, solo base_url diverso — la loro API
        # è dichiaratamente compatibile con lo schema chat.completions.
        self.deepseek_client = (
            openai.AsyncOpenAI(
                api_key=settings.DEEPSEEK_API_KEY,
                base_url=settings.DEEPSEEK_BASE_URL,
                timeout=TIMEOUT_LLM_SECONDI,
            )
            if settings.DEEPSEEK_API_KEY else None
        )

        if not self.openai_client and not self.anthropic_client and not self.deepseek_client:
            raise RuntimeError(
                "Nessun LLM configurato. Imposta OPENAI_API_KEY, ANTHROPIC_API_KEY "
                "o DEEPSEEK_API_KEY nel .env"
            )

        self.routing = ModelRoutingConfig()

        provider = "Claude" if self.anthropic_client else ("OpenAI" if self.openai_client else "DeepSeek")
        logger.info(f"LLMRouter pronto — provider principale: {provider}")

    async def chiedi(self, system: str, user: str, fase: str = "generico") -> RisultatoLLM:
        """Chiamata generica per RAG, analisi e valutazioni."""
        return await self._cloud_chat(system=system, user=user, fase=fase)

    async def genera_racconto(self, prompt: str) -> RisultatoLLM:
        """Genera il draft del racconto tramite cloud LLM."""
        system = (
            "Sei Baldo, un cantastorie gentile. La tua voce non richiede una cornice "
            "o un saluto: segui le istruzioni narrative del prompt e l'ambientazione "
            "richiesta. Usa i dati reali sul cielo solo quando pertinenti. "
            "Usa un linguaggio semplice, immagini vivide, ritmo narrativo "
            "coinvolgente, adatto a bambini in età prescolare. "
            "Non iniziare mai con un titolo o un'intestazione, e non usare ALCUNA "
            "formattazione Markdown (niente *, **, #, --- o simili): il testo va "
            "incollato così com'è in una pagina web, senza rendering Markdown — "
            "se vuoi enfasi o un'onomatopea, scrivila in testo semplice."
        )
        return await self._cloud_chat(system=system, user=prompt, fase="genera_draft")

    async def rifinisci(self, draft: str, eta: int, ritornello: str | None = None) -> RisultatoLLM:
        """Rifinitura editoriale finale del racconto."""
        vincolo_ritornello = (
            f"Preserva il ritornello già usato: {ritornello!r}, senza aggiungere ripetizioni. "
            if ritornello else "Non aggiungere un ritornello. "
        )
        system = (
            f"Sei un editor di racconti per bambini di {eta} anni. "
            "Intervieni solo su problemi effettivi di chiarezza, lessico o ridondanza. "
            "Conserva voce, dettagli personali, dialoghi e ritmo del draft. Se una frase "
            "funziona, copiala invariata. Non rendere il testo più poetico o affettuoso "
            "per abitudine. Non trasformare automaticamente emozioni nominate in reazioni "
            "fisiche stereotipate. Elimina spiegazioni emotive che duplicano un gesto "
            "e cornici affettive generiche. Non aggiungere saluti, appellativi, metafore, "
            "morali o domande finali: la loro assenza è valida. "
            f"{vincolo_ritornello}"
            "Correggi gli errori grammaticali effettivi. Preserva battute brevi, silenzi, "
            "parole quotidiane e variazioni di ritmo; non trasformare i dialoghi in massime. "
            "Preserva azioni, indizi e nessi causali; non aggiungere elementi alla trama. "
            "Restituisci solo il racconto completo, senza Markdown o commenti."
        )
        return await self._cloud_chat(
            system=system,
            user=f"Raffina questo racconto:\n\n{draft}",
            fase="rifinisci",
        )

    async def umanizza(self, draft: str, eta: int, ritornello: str | None = None) -> RisultatoLLM:
        """Rompe la regolarità meccanica del racconto con interventi minimi e mirati."""
        vincolo_ritornello = f"Non toccare il ritornello: {ritornello!r}. " if ritornello else ""
        system = (
            f"Sei un narratore che rilegge un racconto già pronto per bambini di {eta} anni, "
            "cercando i segni di una scrittura troppo regolare — quelli che tradiscono un "
            "testo scritto di fretta, non da un autore che si prende cura del ritmo. "
            + HUMANIZE_CRITERIA +
            " Per ogni punto individuato, proponi la modifica minima che lo risolve: una parola "
            "diversa, un dettaglio concreto, una frase spezzata o unita, un verbo dichiarativo "
            "diverso, un piccolo gesto. Non riscrivere l'intera frase se basta cambiare una parte. "
            "Non toccare trama, causalità, personaggi, luoghi, età target: conserva ogni fatto "
            "narrativo. Non aggiungere similitudini, ripetizioni o cornici affettive nuove. "
            f"{vincolo_ritornello}"
            "Restituisci soltanto un oggetto JSON con il campo diagnosi: una lista di oggetti "
            "con categoria (una delle sei sopra), originale (citazione esatta e univoca dal "
            "racconto), sostituzione, motivo (perché il punto era troppo regolare)."
        )
        return await self._cloud_chat(
            system=system,
            user=f"Rileggi questo racconto e individua i punti da variare:\n\n{draft}",
            fase="umanizza",
        )

    # ------------------------------------------------------------------
    # Metodo interno
    # ------------------------------------------------------------------

    # Catena di ripiego "storica", usata solo quando il provider scelto per
    # la fase non è disponibile o fallisce. DeepSeek non ne fa parte
    # deliberatamente: essendo nuovo e non ancora verificato sulla tenuta
    # delle regole di stile italiane fini (apertura di Baldo, ritornello,
    # reduplicazioni...), lo usiamo solo dove l'admin lo ha assegnato di
    # persona a una fase — mai come sostituto silenzioso altrove.
    _CATENA_RIPIEGO = ("anthropic", "openai")

    async def _cloud_chat(self, system: str, user: str, fase: str = "generico") -> RisultatoLLM:
        """Prova il provider configurato per `fase`, poi la catena di
        ripiego Claude → OpenAI se quello scelto manca o fallisce."""
        provider_scelto = self.routing.get_provider(fase)
        catena = [provider_scelto] + [p for p in self._CATENA_RIPIEGO if p != provider_scelto]

        ultimo_errore: Optional[Exception] = None
        for provider in catena:
            client, modello = self._client_e_modello(provider)
            if not client:
                continue
            t0 = time.monotonic()
            try:
                if provider == "anthropic":
                    risultato = await self._chiedi_anthropic(client, modello, system, user, t0)
                else:
                    # DeepSeek ha il "thinking mode" attivo di default, anche
                    # su richieste banali: osservato in test diretto, 107
                    # token di output per rispondere "OK" contro 1 token con
                    # il thinking disattivato esplicitamente. Per le fasi
                    # classificatorie (l'uso previsto qui) quel ragionamento
                    # nascosto non serve e mangerebbe buona parte del
                    # risparmio di prezzo che DeepSeek offrirebbe altrimenti.
                    # OpenAI non ha questo parametro: lo passiamo solo se il
                    # provider è davvero DeepSeek.
                    extra_body = {"thinking": {"type": "disabled"}} if provider == "deepseek" else None
                    risultato = await self._chiedi_openai_compatibile(
                        client, modello, system, user, t0, extra_body=extra_body
                    )
                if risultato:
                    return risultato
                logger.warning(
                    f"[{fase}] {provider} ha risposto senza testo utile, provo il prossimo."
                )
            except Exception as e:
                ultimo_errore = e
                logger.warning(f"[{fase}] {provider} fallito ({e}), provo il prossimo.")

        raise RuntimeError(
            f"Nessun provider LLM disponibile per la fase '{fase}'"
            + (f": {ultimo_errore}" if ultimo_errore else "")
        )

    def _client_e_modello(self, provider: str):
        if provider == "anthropic":
            return self.anthropic_client, settings.ANTHROPIC_MODEL
        if provider == "openai":
            return self.openai_client, settings.OPENAI_MODEL
        if provider == "deepseek":
            return self.deepseek_client, settings.DEEPSEEK_MODEL
        return None, None

    async def _chiedi_anthropic(self, client, modello, system, user, t0) -> Optional["RisultatoLLM"]:
        resp = await asyncio.wait_for(
            client.messages.create(
                model=modello,
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
        if not testo:
            return None
        uso = getattr(resp, "usage", None)
        return RisultatoLLM(
            testo=self._pulisci(testo),
            modello=modello,
            token_input=getattr(uso, "input_tokens", 0) or 0,
            token_output=getattr(uso, "output_tokens", 0) or 0,
            durata_secondi=time.monotonic() - t0,
        )

    async def _chiedi_openai_compatibile(
        self, client, modello, system, user, t0, extra_body: Optional[dict] = None
    ) -> Optional["RisultatoLLM"]:
        """Usato sia per OpenAI sia per DeepSeek: stesso schema di chiamata
        (l'API di DeepSeek dichiara compatibilità con l'SDK OpenAI).
        `extra_body` è un passthrough di campi specifici del provider
        (es. il thinking mode di DeepSeek) — None per OpenAI, che non lo
        conosce e lo ignorerebbe comunque se lo passassimo sempre, ma è più
        chiaro ometterlo del tutto quando non serve."""
        kwargs = dict(
            model=modello,
            messages=[
                {"role": "system", "content": system},
                {"role": "user", "content": user},
            ],
            temperature=0.8,
        )
        if extra_body:
            kwargs["extra_body"] = extra_body
        resp = await asyncio.wait_for(
            client.chat.completions.create(**kwargs),
            timeout=TIMEOUT_LLM_SECONDI,
        )
        testo = resp.choices[0].message.content if resp.choices else ""
        if not testo:
            return None
        uso = getattr(resp, "usage", None)
        return RisultatoLLM(
            testo=self._pulisci(testo),
            modello=modello,
            token_input=getattr(uso, "prompt_tokens", 0) or 0,
            token_output=getattr(uso, "completion_tokens", 0) or 0,
            durata_secondi=time.monotonic() - t0,
        )

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
