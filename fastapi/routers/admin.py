"""
routers/admin.py
================
Endpoint di amministrazione per la gestione dei PlotFragment.
Accessibile solo dall'interno della rete Docker.

Aggiungere a main.py:
    from routers.admin import router as admin_router
    app.include_router(admin_router)
"""

import logging
from typing import Optional
from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel, Field

from services.neo4j_client import Neo4jClient
from services.llm import LLMRouter
from services.model_routing import FASI, FASI_CON_BYPASS
from config import settings

logger = logging.getLogger(__name__)

from services.admin_auth import require_admin

router = APIRouter(prefix="/admin", tags=["admin"], dependencies=[Depends(require_admin)])


# ---------------------------------------------------------------------------
# Modelli Pydantic
# ---------------------------------------------------------------------------

class PlotFragmentIn(BaseModel):
    id:             str            = Field(..., description="ID univoco, es. 'frag_001'")
    text:           str            = Field(..., description="Testo narrativo del frammento")
    setting:        str            = Field(..., description="Ambientazione, es. 'bosco magico'")
    characters:     list[str]      = Field(default_factory=list, description="Personaggi coinvolti")
    emotions:       list[str]      = Field(default_factory=list, description="Emozioni evocate")
    tema:           Optional[str]  = Field(None, description="Tema narrativo, es. 'amicizia'")
    archetipo:      Optional[str]  = Field(None, description="Archetipo, es. 'viaggio_eroe'")
    tecnica_narrativa: Optional[str] = Field(None, description="Tecnica narrativa usata, es. 'binomio_fantastico' (Rodari), 'ripetizione_tre_volte' (Grimm)")
    fascia_eta:     Optional[str]  = Field(None, description="Fascia d'età consigliata, es. '3-6'")
    ritornello:     Optional[str]  = Field(None, description="Frase-ritornello ripetibile della storia")
    domanda:        Optional[str]  = Field(None, description="Domanda finale per coinvolgere il bambino")

class BulkLoadRequest(BaseModel):
    frammenti:   list[PlotFragmentIn]
    sovrascrivi: bool = Field(False, description="Se true, sovrascrive frammenti esistenti con stesso id")

class BulkLoadResponse(BaseModel):
    caricati:   int
    saltati:    int
    errori:     list[str]

class FragmentDeleteRequest(BaseModel):
    ids: list[str]

class ProviderInfo(BaseModel):
    disponibile: bool   = Field(..., description="True se la relativa API key è configurata")
    modello:     str    = Field(..., description="Modello attualmente configurato per questo provider")

class ModelRoutingResponse(BaseModel):
    fasi:                 dict[str, str]
    provider_disponibili: dict[str, ProviderInfo]
    fasi_con_bypass:       list[str] = Field(
        default_factory=list,
        description="Fasi dove 'nessuno' è un valore accettato (salta la chiamata LLM)",
    )

class ModelRoutingUpdate(BaseModel):
    provider: str = Field(..., description="'anthropic' | 'openai' | 'deepseek', o 'nessuno' solo per le fasi con bypass")


# ---------------------------------------------------------------------------
# Dipendenza Neo4j (iniettata da main.py via app.state)
# ---------------------------------------------------------------------------

def get_neo4j() -> Neo4jClient:
    from main import neo4j_client
    if not neo4j_client:
        raise HTTPException(status_code=503, detail="Neo4j non disponibile")
    return neo4j_client


def get_llm_router() -> LLMRouter:
    from main import llm_router
    if not llm_router:
        raise HTTPException(status_code=503, detail="LLMRouter non disponibile")
    return llm_router


# ---------------------------------------------------------------------------
# POST /admin/frammenti/bulk — Caricamento massivo
# ---------------------------------------------------------------------------

@router.post("/frammenti/bulk", response_model=BulkLoadResponse)
async def carica_frammenti_bulk(
    body: BulkLoadRequest,
    neo4j: Neo4jClient = Depends(get_neo4j),
):
    """
    Carica uno o più PlotFragment nel database Neo4j.
    
    Esempio body:
    {
      "sovrascrivi": false,
      "frammenti": [
        {
          "id": "frag_001",
          "text": "Nel bosco incantato viveva un piccolo gufo...",
          "setting": "bosco magico",
          "characters": ["Gufo", "Volpe"],
          "emotions": ["curiosità", "amicizia"],
          "tema": "amicizia",
          "archetipo": "incontro_inaspettato"
        }
      ]
    }
    """
    caricati, saltati, errori = 0, 0, []

    for frag in body.frammenti:
        try:
            async with neo4j.driver.session() as session:

                # Controlla se esiste già
                esistente = await session.run(
                    "MATCH (f:PlotFragment {id: $id}) RETURN f.id",
                    id=frag.id
                )
                record = await esistente.single()

                if record and not body.sovrascrivi:
                    saltati += 1
                    continue

                # Crea o aggiorna il nodo PlotFragment
                await session.run(
                    """
                    MERGE (f:PlotFragment {id: $id})
                    SET f.text              = $text,
                        f.setting           = $setting,
                        f.tema              = $tema,
                        f.archetipo         = $archetipo,
                        f.tecnica_narrativa = $tecnica_narrativa,
                        f.fascia_eta        = $fascia_eta,
                        f.ritornello        = $ritornello,
                        f.domanda           = $domanda
                    """,
                    id                = frag.id,
                    text              = frag.text,
                    setting           = frag.setting,
                    tema              = frag.tema,
                    archetipo         = frag.archetipo,
                    tecnica_narrativa = frag.tecnica_narrativa,
                    fascia_eta        = frag.fascia_eta,
                    ritornello        = frag.ritornello,
                    domanda           = frag.domanda,
                )

                # Collega i personaggi
                for name in frag.characters:
                    await session.run(
                        """
                        MATCH (f:PlotFragment {id: $fid})
                        MERGE (p:Character {name: $name})
                        MERGE (f)-[:CONTAINS]->(p)
                        """,
                        fid=frag.id, name=name,
                    )

                # Collega le emozioni
                for emotion in frag.emotions:
                    await session.run(
                        """
                        MATCH (f:PlotFragment {id: $fid})
                        MERGE (e:Emotion {name: $name})
                        MERGE (f)-[:EVOKES]->(e)
                        """,
                        fid=frag.id, name=emotion,
                    )

                caricati += 1
                logger.info(f"PlotFragment '{frag.id}' caricato.")

        except Exception as e:
            errori.append(f"{frag.id}: {str(e)}")
            logger.error(f"Errore caricamento frammento '{frag.id}': {e}")

    logger.info(f"Bulk load: {caricati} caricati, {saltati} saltati, {len(errori)} errori.")
    return BulkLoadResponse(caricati=caricati, saltati=saltati, errori=errori)


# ---------------------------------------------------------------------------
# GET /admin/frammenti — Lista tutti i frammenti
# ---------------------------------------------------------------------------

@router.get("/frammenti")
async def lista_frammenti(neo4j: Neo4jClient = Depends(get_neo4j)):
    """Restituisce tutti i PlotFragment con personaggi ed emozioni collegate."""
    async with neo4j.driver.session() as session:
        result = await session.run(
            """
            MATCH (f:PlotFragment)
            OPTIONAL MATCH (f)-[:CONTAINS]->(p:Character)
            OPTIONAL MATCH (f)-[:EVOKES]->(e:Emotion)
            RETURN f.id             AS id,
                   f.text           AS text,
                   f.setting        AS setting,
                   f.tema           AS tema,
                   f.archetipo      AS archetipo,
                   f.tecnica_narrativa AS tecnica_narrativa,
                   f.fascia_eta     AS fascia_eta,
                   f.ritornello     AS ritornello,
                   f.domanda        AS domanda,
                   collect(DISTINCT p.name) AS characters,
                   collect(DISTINCT e.name) AS emotions
            ORDER BY f.id
            """
        )
        frammenti = [dict(r) async for r in result]

    return {"totale": len(frammenti), "frammenti": frammenti}


# ---------------------------------------------------------------------------
# DELETE /admin/frammenti — Elimina frammenti per ID
# ---------------------------------------------------------------------------

@router.delete("/frammenti")
async def elimina_frammenti(
    body: FragmentDeleteRequest,
    neo4j: Neo4jClient = Depends(get_neo4j),
):
    """Elimina i PlotFragment con gli ID specificati."""
    async with neo4j.driver.session() as session:
        result = await session.run(
            """
            MATCH (f:PlotFragment)
            WHERE f.id IN $ids
            DETACH DELETE f
            RETURN count(f) AS eliminati
            """,
            ids=body.ids,
        )
        record = await result.single()
        eliminati = record["eliminati"] if record else 0

    logger.info(f"Eliminati {eliminati} PlotFragment.")
    return {"eliminati": eliminati}


# ---------------------------------------------------------------------------
# GET /admin/stats — Statistiche DB
# ---------------------------------------------------------------------------

@router.get("/stats")
async def stats_db(neo4j: Neo4jClient = Depends(get_neo4j)):
    """Conteggio nodi per label."""
    try:
        async with neo4j.driver.session() as session:
            result = await session.run(
                """
                CALL apoc.meta.stats()
                YIELD labels
                RETURN labels
                """
            )
            record = await result.single()
            if record:
                return {"labels": record["labels"]}
    except Exception:
        pass  # APOC non installato: passa al fallback manuale sotto

    # Fallback senza APOC
    counts = {}
    for label in ["PlotFragment", "Story", "Character", "Emotion"]:
        async with neo4j.driver.session() as session:
            r = await session.run(f"MATCH (n:{label}) RETURN count(n) AS n")
            rec = await r.single()
            counts[label] = rec["n"] if rec else 0

    return {"labels": counts}


# ---------------------------------------------------------------------------
# GET/PUT /admin/modelli — Routing dei provider LLM per fase della pipeline
# ---------------------------------------------------------------------------

@router.get("/modelli", response_model=ModelRoutingResponse)
async def leggi_routing_modelli(llm: LLMRouter = Depends(get_llm_router)):
    """
    Provider configurato per ciascuna fase della pipeline, più la
    disponibilità/modello di ciascun provider (una fase assegnata a un
    provider senza API key configurata userà comunque la catena di ripiego
    Claude → OpenAI al momento della chiamata, ma qui la segnaliamo per
    evitare configurazioni che sembrano attive e non lo sono).
    """
    return ModelRoutingResponse(
        fasi=llm.routing.get_tutti(),
        provider_disponibili={
            "anthropic": ProviderInfo(
                disponibile=llm.anthropic_client is not None,
                modello=settings.ANTHROPIC_MODEL,
            ),
            "openai": ProviderInfo(
                disponibile=llm.openai_client is not None,
                modello=settings.OPENAI_MODEL,
            ),
            "deepseek": ProviderInfo(
                disponibile=llm.deepseek_client is not None,
                modello=settings.DEEPSEEK_MODEL,
            ),
        },
        fasi_con_bypass=sorted(FASI_CON_BYPASS),
    )


@router.put("/modelli/{fase}")
async def imposta_routing_modello(
    fase: str,
    body: ModelRoutingUpdate,
    llm: LLMRouter = Depends(get_llm_router),
):
    """Assegna un provider a una singola fase della pipeline (o 'nessuno'
    per le fasi in FASI_CON_BYPASS, per saltare del tutto la chiamata)."""
    if fase not in FASI:
        raise HTTPException(
            status_code=404,
            detail=f"Fase sconosciuta: '{fase}'. Fasi valide: {', '.join(FASI)}",
        )
    try:
        llm.routing.set_provider(fase, body.provider)
    except ValueError as e:
        raise HTTPException(status_code=422, detail=str(e))
    # Ritorniamo tutte le fasi (non solo quella appena scritta): con più
    # worker uvicorn una scrittura può ancora, in rari casi, sovrapporsi a
    # un'altra in corso su un processo diverso — restituire lo stato intero
    # rilegge dal file appena salvato e rende visibile subito un eventuale
    # disallineamento, invece di scoprirlo solo nel report di una storia.
    return {"fase": fase, "provider": body.provider, "fasi": llm.routing.get_tutti()}
