"""
Neo4jClient
===========
Schema (English Unified):
  Nodes:
    (:PlotFragment {id, text, setting})
    (:Story {id, text, original_prompt, setting, timestamp})
    (:Character {name})
    (:Emotion {name})

  Relationships:
    (:PlotFragment|Story)-[:CONTAINS]->(:Character)
    (:PlotFragment|Story)-[:EVOKES]->(:Emotion)
    (:Character)-[:RELATIONSHIP {type}]->(:Character)
    (:Story)-[:INSPIRED_BY]->(:PlotFragment)
"""



import logging
from neo4j import AsyncGraphDatabase

logger = logging.getLogger(__name__)

MAX_FRAGMENTS = 5
MAX_PREVIOUS_STORIES = 3

class Neo4jClient:

    def __init__(self, uri: str, user: str, password: str):
        self.driver = AsyncGraphDatabase.driver(uri, auth=(user, password))

    async def close(self):
        await self.driver.close()

    # ------------------------------------------------------------------
    # FRAGMENT SEARCH
    # ------------------------------------------------------------------
    async def get_character_bios(self, names: list[str]) -> dict[str, dict]:
        """
        Recupera descrizione/tratti dei personaggi nominati, per coerenza
        narrativa tra una storia e l'altra. L'identificativo è il nome
        (Character.name), lo stesso usato ovunque per collegare i frammenti
        ai personaggi — non serve un id separato per questa relazione.
        Un personaggio senza descrizione/tratti (il caso comune per quelli
        creati automaticamente dal caricamento dei frammenti) viene escluso.
        """
        if not names:
            return {}
        async with self.driver.session() as session:
            result = await session.run(
                """
                MATCH (c:Character)
                WHERE c.name IN $names
                  AND (c.description IS NOT NULL OR c.traits IS NOT NULL)
                RETURN c.name AS name, c.description AS description, c.traits AS traits
                """,
                names=names,
            )
            return {
                r["name"]: {"description": r["description"], "traits": r["traits"]}
                async for r in result
            }

    async def cerca_frammenti(
        self,
        characters: list[str],
        emotions: list[str],
        setting: str,
        extra_terms: list[str] = None,
        eta_bambino: int = None,
    ) -> list[dict]:
        all_terms = list(set((extra_terms or []) + characters + emotions))

        async with self.driver.session() as session:
            result = await session.run(
                """
                MATCH (f:PlotFragment)
                OPTIONAL MATCH (f)-[:CONTAINS]->(p:Character)
                WITH f, collect(DISTINCT p.name) AS nomi_personaggi
                OPTIONAL MATCH (f)-[:EVOKES]->(e:Emotion)
                WITH f, nomi_personaggi, collect(DISTINCT e.name) AS nomi_emozioni
                WITH f,
                     size([n IN nomi_personaggi WHERE n IN $characters]) AS match_p,
                     size([n IN nomi_emozioni WHERE n IN $emotions]) AS match_e,
                     size([n IN nomi_personaggi WHERE n IN $terms]) +
                     size([n IN nomi_emozioni WHERE n IN $terms]) AS match_t
                WHERE match_p > 0 OR match_e > 0 OR match_t > 0
                WITH f, match_p, match_e, match_t,
                     CASE
                       WHEN $eta_bambino IS NULL OR f.fascia_eta IS NULL THEN 0
                       WHEN $eta_bambino >= toInteger(trim(split(f.fascia_eta, '-')[0]))
                        AND $eta_bambino <= toInteger(trim(split(f.fascia_eta, '-')[1]))
                       THEN 1
                       ELSE 0
                     END AS match_eta
                RETURN f.id AS id, f.text AS testo, f.setting AS ambientazione,
                       f.tecnica_narrativa AS tecnica_narrativa, f.domanda AS domanda,
                       f.ritornello AS ritornello, f.archetipo AS archetipo,
                       (match_p * 2 + match_e + match_t + match_eta) AS score
                ORDER BY score DESC, rand()
                LIMIT $limit
                """,
                characters=characters,
                emotions=emotions,
                terms=all_terms,
                eta_bambino=eta_bambino,
                limit=MAX_FRAGMENTS,
            )
            fragments = [dict(record) async for record in result]

        if not fragments:
            fragments = await self._search_by_setting(setting, eta_bambino)

        logger.info(f"Neo4j: found {len(fragments)} fragments")
        return fragments

    async def _search_by_setting(self, setting: str, eta_bambino: int = None) -> list[dict]:
        keyword = setting.split()[0] if setting else ""
        async with self.driver.session() as session:
            result = await session.run(
                """
                MATCH (f:PlotFragment)
                WHERE toLower(f.setting) CONTAINS toLower($keyword)
                RETURN f.id AS id, f.text AS testo, f.setting AS ambientazione,
                       f.tecnica_narrativa AS tecnica_narrativa, f.domanda AS domanda,
                       f.ritornello AS ritornello, f.archetipo AS archetipo,
                       CASE
                         WHEN $eta_bambino IS NULL OR f.fascia_eta IS NULL THEN 0
                         WHEN $eta_bambino >= toInteger(trim(split(f.fascia_eta, '-')[0]))
                          AND $eta_bambino <= toInteger(trim(split(f.fascia_eta, '-')[1]))
                         THEN 1
                         ELSE 0
                       END AS score
                ORDER BY score DESC, rand()
                LIMIT $limit
                """,
                keyword=keyword,
                eta_bambino=eta_bambino,
                limit=MAX_FRAGMENTS,
            )
            return [dict(record) async for record in result]

    # ------------------------------------------------------------------
    # MEMORY — Previous Stories
    # ------------------------------------------------------------------
    async def cerca_storie_precedenti(self, characters: list[str], emotions: list[str]) -> list[dict]:
        async with self.driver.session() as session:
            result = await session.run(
                """
                MATCH (s:Story)
                OPTIONAL MATCH (s)-[:CONTAINS]->(p:Character)
                WITH s, collect(DISTINCT p.name) AS nomi_personaggi
                OPTIONAL MATCH (s)-[:EVOKES]->(e:Emotion)
                WITH s, nomi_personaggi, collect(DISTINCT e.name) AS nomi_emozioni
                WITH s,
                     size([n IN nomi_personaggi WHERE n IN $characters]) AS match_p,
                     size([n IN nomi_emozioni WHERE n IN $emotions]) AS match_e
                WHERE match_p > 0 OR match_e > 0
                RETURN s.id AS id, s.text AS testo, s.setting AS ambientazione,
                       s.timestamp AS timestamp,
                       (match_p * 2 + match_e) AS score
                ORDER BY score DESC, s.timestamp DESC
                LIMIT $limit
                """,
                characters=characters,
                emotions=emotions,
                limit=MAX_PREVIOUS_STORIES,
            )
            stories = [dict(record) async for record in result]

        logger.info(f"Neo4j: found {len(stories)} relevant previous stories")
        return stories

    # ------------------------------------------------------------------
    # MEMORY — Save New Story
    # ------------------------------------------------------------------
    async def salva_storia(
        self,
        story_id: str,
        text: str,
        characters: list[str],
        emotions: list[str],
        setting: str,
        original_prompt: str,
        used_fragments: list[str],
        timestamp: str,
    ) -> None:
        async with self.driver.session() as session:
            # Create Story node
            await session.run(
                """
                CREATE (s:Story {
                    id: $id,
                    text: $text,
                    setting: $setting,
                    original_prompt: $prompt,
                    timestamp: $timestamp
                })
                """,
                id=story_id, text=text, setting=setting,
                prompt=original_prompt, timestamp=timestamp,
            )

            # Link Characters
            for name in characters:
                await session.run(
                    """
                    MATCH (s:Story {id: $sid})
                    MERGE (p:Character {name: $name})
                    MERGE (s)-[:CONTAINS]->(p)
                    """,
                    sid=story_id, name=name,
                )

            # Link Emotions
            for emotion in emotions:
                await session.run(
                    """
                    MATCH (s:Story {id: $sid})
                    MERGE (e:Emotion {name: $name})
                    MERGE (s)-[:EVOKES]->(e)
                    """,
                    sid=story_id, name=emotion,
                )

            # Link Fragments
            for fid in used_fragments:
                await session.run(
                    """
                    MATCH (s:Story {id: $sid})
                    MATCH (f:PlotFragment {id: $fid})
                    MERGE (s)-[:INSPIRED_BY]->(f)
                    """,
                    sid=story_id, fid=fid,
                )

        logger.info(f"Neo4j: story {story_id} saved successfully.")

    async def get_relazioni_personaggi(self, characters: list[str]) -> list[dict]:
        async with self.driver.session() as session:
            result = await session.run(
                """
                MATCH (a:Character)-[r:RELATIONSHIP]->(b:Character)
                WHERE a.name IN $characters AND b.name IN $characters
                RETURN a.name AS da, r.type AS tipo, b.name AS a
                """,
                characters=characters,
            )
            return [dict(record) async for record in result]
