import json
import asyncio
import uuid
from services.neo4j_client import Neo4jClient

async def ingest():
    # Inizializza il client (leggerà le variabili d'ambiente dal tuo .env)
    client = Neo4jClient()
    
    try:
        with open("fragments.json", "r", encoding="utf-8") as f:
            fragments = json.load(f)
    except FileNotFoundError:
        print("Errore: fragments.json non trovato!")
        return

    print(f"🚀 Inizio caricamento di {len(fragments)} frammenti...")

    for data in fragments:
        # Generiamo un ID univoco se non presente
        fragment_id = str(uuid.uuid4())[:8]
        
        # Usiamo una query Cypher diretta per mappare tutto correttamente
        # Questo assicura che i nodi Character/Emotion vengano creati se mancano
        query = """
        MERGE (f:PlotFragment {id: $id})
        SET f.text = $text,
            f.setting = $setting
        
        WITH f
        UNWIND $characters AS char_name
        MERGE (c:Character {name: char_name})
        MERGE (f)-[:CONTAINS]->(c)
        
        WITH f
        UNWIND $emotions AS emo_name
        MERGE (e:Emotion {name: emo_name})
        MERGE (f)-[:EVOKES]->(e)
        """
        
        try:
            await client.execute_query(
                query,
                {
                    "id": fragment_id,
                    "text": data["text"],
                    "setting": data.get("setting", "generico"),
                    "characters": data.get("characters", []),
                    "emotions": data.get("emotions", [])
                }
            )
            print(f"✅ Caricato frammento: {fragment_id}")
        except Exception as e:
            print(f"❌ Errore sul frammento {fragment_id}: {e}")

    await client.close()
    print("\n✨ Ingestion completata!")

if __name__ == "__main__":
    asyncio.run(ingest())
