"""
qdrant_store.py
Wrapper Qdrant per la knowledge base del planetario.
Gestisce creazione collezione, upsert con embedding e ricerca.
"""

import logging
import uuid
import hashlib
from typing import Any, Optional

from qdrant_client import QdrantClient
from qdrant_client.models import (
    Distance, VectorParams,
    PointStruct, Filter, FieldCondition,
    MatchValue, MatchAny, Range, OrderBy,
    PayloadSchemaType,
)
from sentence_transformers import SentenceTransformer

logger = logging.getLogger(__name__)

COLLECTION_NAME = "planetarium_kb"
VECTOR_DIM      = 384        # all-MiniLM-L6-v2
EMBEDDING_MODEL = "all-MiniLM-L6-v2"

# Campi indicizzati per filtri veloci
INDEXED_FIELDS = [
    ("tipo",                 PayloadSchemaType.KEYWORD),
    ("sottotipo",            PayloadSchemaType.KEYWORD),
    ("classe_spettrale",     PayloadSchemaType.KEYWORD),
    ("classe_luminosita",    PayloadSchemaType.KEYWORD),
    ("colori",               PayloadSchemaType.KEYWORD),
    ("costellazione",        PayloadSchemaType.KEYWORD),
    ("emisfero",             PayloadSchemaType.KEYWORD),
    ("mese_osservazione",    PayloadSchemaType.KEYWORD),
    ("has_wikipedia",        PayloadSchemaType.BOOL),
    ("visibilita_occhio_nudo", PayloadSchemaType.BOOL),
    ("magnitudine_v",        PayloadSchemaType.FLOAT),
    ("distanza_ly",          PayloadSchemaType.FLOAT),
    ("temperatura_stimata_k", PayloadSchemaType.INTEGER),
]


# src/qdrant_store.py

class PlanetariumStore:
    def __init__(self, host: str = "localhost", port: int = 6333):
        # Aumenta il timeout a 60 o 120 secondi
        self.client = QdrantClient(host=host, port=port, timeout=120)
        self.model  = SentenceTransformer(EMBEDDING_MODEL)
        logger.info(f"[QDRANT] Connesso a {host}:{port} con timeout 120s")
    # ── Setup collezione ─────────────────────────────────────────────────────

    def ensure_collection(self, recreate: bool = False) -> None:
        """
        Crea la collezione se non esiste.
        Con recreate=True la elimina e ricrea (usare solo in sviluppo).
        """
        exists = any(
            c.name == COLLECTION_NAME
            for c in self.client.get_collections().collections
        )

        if exists and recreate:
            self.client.delete_collection(COLLECTION_NAME)
            logger.warning(f"[QDRANT] Collezione '{COLLECTION_NAME}' eliminata.")
            exists = False

        if not exists:
            self.client.create_collection(
                collection_name=COLLECTION_NAME,
                vectors_config=VectorParams(
                    size=VECTOR_DIM,
                    distance=Distance.COSINE,
                ),
            )
            # Crea indici sui campi usati nei filtri
            for field_name, field_type in INDEXED_FIELDS:
                self.client.create_payload_index(
                    collection_name=COLLECTION_NAME,
                    field_name=field_name,
                    field_schema=field_type,
                )
            logger.info(f"[QDRANT] Collezione '{COLLECTION_NAME}' creata con {len(INDEXED_FIELDS)} indici.")
        else:
            logger.info(f"[QDRANT] Collezione '{COLLECTION_NAME}' già esistente.")

    # ── Embedding ────────────────────────────────────────────────────────────

    def embed(self, texts: list[str]) -> list[list[float]]:
        return self.model.encode(texts, show_progress_bar=False).tolist()

    def embed_one(self, text: str) -> list[float]:
        return self.model.encode([text], show_progress_bar=False)[0].tolist()

    # ── ID deterministico ────────────────────────────────────────────────────

    @staticmethod
    def make_id(tipo: str, nome: str) -> str:
        """
        ID deterministico basato su tipo+nome: garantisce idempotenza
        negli upsert (reingestione non crea duplicati).
        """
        raw = f"{tipo}:{nome}".lower().encode()
        h   = hashlib.md5(raw).hexdigest()
        return str(uuid.UUID(h))

    # ── Upsert ───────────────────────────────────────────────────────────────

    def upsert(self, documents: list[dict]) -> int:
        """
        Upsert di una lista di documenti.
        Ogni documento deve avere almeno: tipo, nome, text.
        Ritorna il numero di documenti inseriti/aggiornati.
        """
        if not documents:
            return 0

        texts  = [d["text"] for d in documents]
        vectors = self.embed(texts)

        points = []
        for doc, vector in zip(documents, vectors):
            doc_id  = self.make_id(doc["tipo"], doc["nome"])
            payload = {k: v for k, v in doc.items() if k != "text"}
            points.append(PointStruct(id=doc_id, vector=vector, payload=payload))

        self.client.upsert(
            collection_name=COLLECTION_NAME,
            points=points,
            wait=False  # Cambia wait da True (default) a False
        )

        logger.info(f"[QDRANT] Upsert {len(points)} documenti (tipo={documents[0]['tipo']})")
        return len(points)

    def upsert_one(self, document: dict) -> None:
        self.upsert([document])

    # ── Search ───────────────────────────────────────────────────────────────

    def search(
        self,
        query: str,
        tipo: Optional[str] = None,
        extra_filters: Optional[list] = None,
        limit: int = 5,
    ) -> list[dict]:
        """
        Similarity search con filtro opzionale per tipo e campi aggiuntivi.
        """
        conditions = []
        if tipo:
            conditions.append(FieldCondition(key="tipo", match=MatchValue(value=tipo)))
        if extra_filters:
            conditions.extend(extra_filters)

        query_filter = Filter(must=conditions) if conditions else None
        vector       = self.embed_one(query)

        hits = self.client.search(
            collection_name=COLLECTION_NAME,
            query_vector=vector,
            query_filter=query_filter,
            limit=limit,
            with_payload=True,
        )
        return [h.payload for h in hits]

    def scroll_ordered(
        self,
        tipo: str,
        order_field: str,
        direction: str = "asc",
        extra_filters: Optional[list] = None,
        limit: int = 1,
    ) -> list[dict]:
        """
        Query strutturata ordinata per campo numerico.
        Usata per superlative queries (più vicina, più brillante, ecc.).
        """
        conditions = [
            FieldCondition(key="tipo", match=MatchValue(value=tipo)),
            FieldCondition(key=order_field, range=Range(gt=0)),
        ]
        if extra_filters:
            conditions.extend(extra_filters)

        results, _ = self.client.scroll(
            collection_name=COLLECTION_NAME,
            scroll_filter=Filter(must=conditions),
            order_by=OrderBy(key=order_field, direction=direction),
            limit=limit,
            with_payload=True,
        )
        return [r.payload for r in results]

    # ── Stats ────────────────────────────────────────────────────────────────

    def count(self, tipo: Optional[str] = None) -> int:
        f = None
        if tipo:
            f = Filter(must=[FieldCondition(key="tipo", match=MatchValue(value=tipo))])
        return self.client.count(collection_name=COLLECTION_NAME, count_filter=f).count

    def stats(self) -> dict:
        tipi = ["STARS", "CONSTELLATIONS", "SOLARSYSTEM", "PHENOMENA"]
        return {t: self.count(t) for t in tipi}
