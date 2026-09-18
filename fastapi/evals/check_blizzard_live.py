"""Opt-in regression: python -m evals.check_blizzard_live (uses provider credentials)."""
import asyncio
import json
from evals.run import make_router, close_router
from services.narrative_review import verifica_testo_finale

REQUEST = 'In un inverno rigido i lupi hanno scarsità di cibo. Lupetto segue il branco di nascosto, si perde nella tormenta e una giovane renna lo aiuta a ritrovare il branco. I lupi rinunciano alla caccia e tornano alla tana.'
CASES = {
    'omissioni': 'Faceva molto freddo. Lupetto seguì il branco di nascosto e si perse nella tormenta. Non vedeva nemmeno le sue zampe. Una giovane renna gli disse: «Conosco il sentiero della mandria». Camminò sicura e lo portò dai lupi. Il capo guardò la mandria lontana nella tormenta fitta. «La caccia è finita». Tornarono alla tana.',
    'coerente': 'Era un inverno rigido e il branco non trovava cibo da giorni. Lupetto seguì i lupi di nascosto e si perse nella tormenta. Una giovane renna si fermò vicino a lui. «Ho visto il tuo branco ripararsi dietro quelle rocce, poco fa». Lo accompagnò al riparo. La mamma gli scaldò il muso. «Con questa tormenta basta caccia», disse il capo. Tornarono alla tana.'
}

async def main():
    router = make_router('anthropic')
    try:
        for name, story in CASES.items():
            result = await verifica_testo_finale({'racconto_finale': story, 'prompt_originale': REQUEST, 'eta_bambino': 6}, router)
            print(json.dumps({'caso': name, 'testo': result['racconto_finale'], 'revisione': result['revisione_finale']}, ensure_ascii=False))
            assert result['revisione_finale']['esito'] == ('corretto' if name == 'omissioni' else 'ok')
    finally:
        await close_router(router)

if __name__ == '__main__':
    asyncio.run(main())
