"""
main.py
Orchestratore del processo di ingestion completo.
Eseguire con:  python main.py [--recreate] [--only TIPO]

Flags:
  --recreate   Elimina e ricrea la collezione Qdrant (sviluppo)
  --only       Esegue solo un tipo: stars | constellations | solarsystem | phenomena
  --mag-limit  Magnitudine limite per le stelle (default 6.5)
  --bs5        Percorso file JSON BS5 (default data/bs5.json)
"""

import asyncio
import argparse
import logging
import sys
from pathlib import Path

# Configura logging prima di tutto
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler("logs/ingestion.log", encoding="utf-8"),
    ],
)
logger = logging.getLogger(__name__)

from src.qdrant_store import PlanetariumStore
from src.ingest_stars import ingest_stars
from src.ingest_constellations import ingest_constellations
from src.ingest_solarsystem import ingest_solarsystem, ingest_phenomena, ingest_missions


WIKI_CACHE_DIR = Path("data/wiki_cache")
BS5_DEFAULT    = Path("data/bsc5.json")
QDRANT_HOST    = "localhost"   # in Docker: nome servizio es. "qdrant"
QDRANT_PORT    = 6333


async def run(args: argparse.Namespace) -> None:

    WIKI_CACHE_DIR.mkdir(parents=True, exist_ok=True)
    Path("logs").mkdir(exist_ok=True)

    store = PlanetariumStore(host=QDRANT_HOST, port=QDRANT_PORT)
    store.ensure_collection(recreate=args.recreate)

    only = args.only.lower() if args.only else None
    all_stats = {}

    # ── STARS ─────────────────────────────────────────────────────────────────
    if only in (None, "stars"):
        bs5_path = Path(args.bs5)
        if not bs5_path.exists():
            logger.error(f"File BS5 non trovato: {bs5_path}")
            if only == "stars":
                sys.exit(1)
        else:
            stats = await ingest_stars(
                bs5_path=bs5_path,
                store=store,
                wiki_cache_dir=WIKI_CACHE_DIR,
                mag_limit=args.mag_limit,
            )
            all_stats["stars"] = stats

    # ── CONSTELLATIONS ────────────────────────────────────────────────────────
    if only in (None, "constellations"):
        stats = await ingest_constellations(
            store=store,
            wiki_cache_dir=WIKI_CACHE_DIR,
        )
        all_stats["constellations"] = stats

    # ── SOLARSYSTEM ───────────────────────────────────────────────────────────
    if only in (None, "solarsystem"):
        stats = await ingest_solarsystem(
            store=store,
            wiki_cache_dir=WIKI_CACHE_DIR,
        )
        all_stats["solarsystem"] = stats

    # ── PHENOMENA ─────────────────────────────────────────────────────────────
    if only in (None, "phenomena"):
        stats = await ingest_phenomena(
            store=store,
            wiki_cache_dir=WIKI_CACHE_DIR,
        )
        all_stats["phenomena"] = stats

        stats = await ingest_missions(
            store=store,
            wiki_cache_dir=WIKI_CACHE_DIR,
        )
        all_stats["missions"] = stats

    # ── Report finale ─────────────────────────────────────────────────────────
    logger.info("=" * 60)
    logger.info("INGESTION COMPLETATA")
    logger.info("=" * 60)
    for tipo, s in all_stats.items():
        logger.info(f"  {tipo.upper():20s} → {s.get('inserite', s.get('totale', '?'))} documenti")

    qdrant_stats = store.stats()
    logger.info("─" * 60)
    logger.info("Documenti in Qdrant per tipo:")
    for tipo, count in qdrant_stats.items():
        logger.info(f"  {tipo:20s} → {count}")
    logger.info("=" * 60)


def main() -> None:
    parser = argparse.ArgumentParser(description="Planetarium KB Ingestion")
    parser.add_argument("--recreate",  action="store_true", help="Ricrea la collezione Qdrant")
    parser.add_argument("--only",      type=str, default=None,
                        choices=["stars", "constellations", "solarsystem", "phenomena"],
                        help="Esegui solo un tipo di ingestion")
    parser.add_argument("--mag-limit", type=float, default=6.5,
                        help="Magnitudine limite stelle (default 6.5)")
    parser.add_argument("--bs5",       type=str, default="data/bsc5.json",
                        help="Percorso file JSON BS5")
    args = parser.parse_args()

    asyncio.run(run(args))


if __name__ == "__main__":
    main()
