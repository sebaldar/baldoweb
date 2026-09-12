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

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/admin", tags=["admin"])


# ---------------------------------------------------------------------------
# Modelli Pydantic
# ---------------------------------------------------------------------------

class PlotFragmentIn(BaseModel):
    id:          str              = Field(..., description="ID univoco, es. 'frag_001'")
    text:        str              = Field(..., description="Testo narrativo del frammento")
    setting:     str              = Field(..., description="Ambientazione, es. 'bosco magico'")
    characters:  list[str]       = Field(default_factory=list, description="Personaggi coinvolti")
    emotions:    list[str]       = Field(default_factory=list, description="Emozioni evocate")
    tema:        Optional[str]   = Field(None, description="Tema narrativo, es. 'amicizia'")
    archetipo:   Optional[str]   = Field(None, description="Archetipo, es. 'viaggio_eroe'")

class BulkLoadRequest(BaseModel):
    frammenti:   list[PlotFragmentIn]
    sovrascrivi: bool = Field(False, description="Se true, sovrascrive frammenti esistenti con stesso id")

class BulkLoadResponse(BaseModel):
    caricati:   int
    saltati:    int
    errori:     list[str]

class FragmentDeleteRequest(BaseModel):
    ids: list[str]


# ---------------------------------------------------------------------------
# Dipendenza Neo4j (iniettata da main.py via app.state)
# ---------------------------------------------------------------------------

def get_neo4j() -> Neo4jClient:
    from main import neo4j_client
    if not neo4j_client:
        raise HTTPException(status_code=503, detail="Neo4j non disponibile")
    return neo4j_client


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
                    SET f.text     = $text,
                        f.setting  = $setting,
                        f.tema     = $tema,
                        f.archetipo = $archetipo
                    """,
                    id        = frag.id,
                    text      = frag.text,
                    setting   = frag.setting,
                    tema      = frag.tema,
                    archetipo = frag.archetipo,
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
            RETURN f.id        AS id,
                   f.text      AS text,
                   f.setting   AS setting,
                   f.tema      AS tema,
                   f.archetipo AS archetipo,
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
