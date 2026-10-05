"""
wikipedia.py
Client per Wikipedia REST API con rate limiting, retry e cache su file.
Recupera summary e categorie per stelle, costellazioni, corpi del sistema solare.
"""

import asyncio
import aiohttp
import json
import hashlib
import logging
from pathlib import Path
from typing import Optional

logger = logging.getLogger(__name__)

# ── Configurazione ───────────────────────────────────────────────────────────

WIKI_API    = "https://en.wikipedia.org/api/rest_v1/page/summary/{title}"
WIKI_SEARCH = "https://en.wikipedia.org/w/api.php"
HEADERS     = {"User-Agent": "PlanetariumKB/1.0 (astronomy knowledge base builder)"}

MAX_CONCURRENT = 2      # richieste parallele max
RATE_DELAY     = 0.5   # secondi tra batch
MAX_RETRIES    = 3
RETRY_DELAY    = 3.8    # secondi prima del retry
MIN_SUMMARY_LEN = 100   # caratteri minimi per considerare il summary valido

# ── Cache su file ────────────────────────────────────────────────────────────

class FileCache:
    """Cache JSON su file per evitare di ri-chiamare Wikipedia dopo interruzioni."""

    def __init__(self, cache_dir: Path):
        self.cache_dir = cache_dir
        self.cache_dir.mkdir(parents=True, exist_ok=True)

    def _path(self, key: str) -> Path:
        h = hashlib.md5(key.encode()).hexdigest()
        return self.cache_dir / f"{h}.json"

    def get(self, key: str) -> Optional[dict]:
        p = self._path(key)
        if p.exists():
            try:
                return json.loads(p.read_text(encoding="utf-8"))
            except Exception:
                return None
        return None

    def set(self, key: str, value: dict) -> None:
        p = self._path(key)
        p.write_text(json.dumps(value, ensure_ascii=False), encoding="utf-8")

    def set_miss(self, key: str) -> None:
        """Salva un miss esplicito per non richiamare l'API."""
        self.set(key, {"_miss": True})

    def is_miss(self, cached: dict) -> bool:
        return cached.get("_miss", False)


# ── Client principale ────────────────────────────────────────────────────────

class WikipediaClient:

    def __init__(self, cache_dir: Path):
        self.cache    = FileCache(cache_dir)
        self._sem     = asyncio.Semaphore(MAX_CONCURRENT)
        self._session: Optional[aiohttp.ClientSession] = None

    async def __aenter__(self):
        self._session = aiohttp.ClientSession(headers=HEADERS)
        return self

    async def __aexit__(self, *args):
        if self._session:
            await self._session.close()

    # ── Summary ──────────────────────────────────────────────────────────────

    async def get_summary(self, title: str) -> Optional[dict]:
        """
        Recupera il summary Wikipedia per un titolo.
        Ritorna None se la pagina non esiste o il summary è troppo breve.
        Usa cache su file per evitare ri-chiamate.
        """
        cached = self.cache.get(title)
        if cached is not None:
            if self.cache.is_miss(cached):
                return None
            return cached

        result = await self._fetch_summary(title)

        if result is None:
            self.cache.set_miss(title)
        else:
            self.cache.set(title, result)

        return result

    async def _fetch_summary(self, title: str) -> Optional[dict]:
        url = WIKI_API.format(title=title.replace(" ", "_"))

        for attempt in range(1, MAX_RETRIES + 1):
            async with self._sem:
                try:
                    async with self._session.get(url, timeout=aiohttp.ClientTimeout(total=10)) as resp:
                        if resp.status == 404:
                            logger.debug(f"[WIKI] 404 per '{title}'")
                            return None
                        if resp.status == 429:
                            wait = int(resp.headers.get("Retry-After", RETRY_DELAY * attempt))
                            logger.warning(f"[WIKI] Rate limit, attendo {wait}s…")
                            await asyncio.sleep(wait)
                            continue
                        if resp.status != 200:
                            logger.warning(f"[WIKI] HTTP {resp.status} per '{title}'")
                            return None

                        data = await resp.json()

                        summary = data.get("extract", "").strip()
                        if len(summary) < MIN_SUMMARY_LEN:
                            logger.debug(f"[WIKI] Summary troppo breve per '{title}' ({len(summary)} car.)")
                            return None

                        return {
                            "summary":      summary,
                            "url":          data.get("content_urls", {}).get("desktop", {}).get("page", ""),
                            "thumbnail":    data.get("thumbnail", {}).get("source"),
                            "description":  data.get("description", ""),
                            "pageid":       data.get("pageid"),
                        }

                except asyncio.TimeoutError:
                    logger.warning(f"[WIKI] Timeout per '{title}' (tentativo {attempt})")
                except aiohttp.ClientError as e:
                    logger.warning(f"[WIKI] Errore rete per '{title}': {e}")

                if attempt < MAX_RETRIES:
                    await asyncio.sleep(RETRY_DELAY * attempt)

        return None

    # ── Search (fallback per nomi alternativi) ────────────────────────────────

    async def search_title(self, query: str) -> Optional[str]:
        """
        Cerca il titolo Wikipedia più pertinente per una query.
        Usato come fallback quando il titolo diretto non funziona.
        """
        cached = self.cache.get(f"search:{query}")
        if cached is not None:
            return None if self.cache.is_miss(cached) else cached.get("title")

        params = {
            "action":   "query",
            "list":     "search",
            "srsearch": query,
            "srlimit":  1,
            "format":   "json",
        }

        async with self._sem:
            try:
                async with self._session.get(
                    WIKI_SEARCH, params=params,
                    timeout=aiohttp.ClientTimeout(total=10)
                ) as resp:
                    if resp.status != 200:
                        return None
                    data  = await resp.json()
                    hits  = data.get("query", {}).get("search", [])
                    if not hits:
                        self.cache.set_miss(f"search:{query}")
                        return None
                    title = hits[0]["title"]
                    self.cache.set(f"search:{query}", {"title": title})
                    return title
            except Exception as e:
                logger.warning(f"[WIKI] Search fallita per '{query}': {e}")
                return None

    # ── Fetch con fallback automatico ─────────────────────────────────────────

    async def get_summary_with_fallback(
        self, primary: str, fallbacks: list[str] = []
    ) -> Optional[dict]:
        """
        Prova il titolo primario, poi i fallback, poi una search.
        Ritorna il primo risultato valido.
        """
        for title in [primary] + fallbacks:
            result = await self.get_summary(title)
            if result:
                logger.debug(f"[WIKI] ✓ '{title}'")
                await asyncio.sleep(RATE_DELAY)
                return result

        # Ultimo tentativo: ricerca testuale
        found = await self.search_title(primary)
        if found and found.lower() != primary.lower():
            result = await self.get_summary(found)
            if result:
                logger.debug(f"[WIKI] ✓ via search: '{found}' per '{primary}'")
                return result

        logger.debug(f"[WIKI] ✗ nessuna pagina per '{primary}'")
        return None

    # ── Batch helper ──────────────────────────────────────────────────────────

    async def get_batch(
        self, items: list[tuple[str, list[str]]]
    ) -> list[Optional[dict]]:
        """
        Recupera in parallelo (rispettando il semaforo) una lista di
        (titolo_primario, [fallback1, fallback2, ...]).
        """
        tasks = [
            self.get_summary_with_fallback(primary, fallbacks)
            for primary, fallbacks in items
        ]
        return await asyncio.gather(*tasks)
