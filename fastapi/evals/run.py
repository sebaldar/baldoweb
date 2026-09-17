"""Run with `python -m evals.run --help`. Live API use is explicit."""
import argparse
import asyncio
from collections import Counter, defaultdict
from datetime import datetime, timezone
import difflib
import hashlib
import json
from pathlib import Path
import statistics
import time

from services.narrative_review import verifica_testo_finale

ROOT = Path(__file__).resolve().parents[1]
CORPUS = Path(__file__).with_name('cases.json')


def load_cases(path):
    data = json.loads(path.read_text())
    cases = data['cases']
    seen = set()
    for case in cases:
        if case['id'] in seen or not isinstance(case['difettoso'], bool):
            raise ValueError('Identificatore duplicato o etichetta non valida')
        if not case['testo'].strip() or not case['prompt'].strip():
            raise ValueError('Testo e prompt devono essere presenti')
        seen.add(case['id'])
    if not cases:
        raise ValueError('Corpus vuoto')
    return data


def make_router(provider):
    # Import only during opt-in live runs; offline validation needs no credentials.
    from services.llm import LLMRouter
    class FixedRouter(LLMRouter):
        _CATENA_RIPIEGO = ()  # Never silently compare a fallback model.
        def __init__(self):
            super().__init__()
            self.routing = type('FixedRouting', (), {'get_provider': lambda _, phase: provider})()
            self.traces = []
        async def _cloud_chat(self, system, user, fase='generico'):
            response = await super()._cloud_chat(system=system, user=user, fase=fase)
            self.traces.append({'fase': fase, 'system': system, 'user': user,
                                'risposta': response.testo, 'modello': response.modello,
                                'token_input': response.token_input, 'token_output': response.token_output,
                                'durata_secondi': response.durata_secondi})
            return response
    return FixedRouter()


async def close_router(router):
    clients = [getattr(router, name, None) for name in
               ('openai_client', 'anthropic_client', 'deepseek_client')]
    await asyncio.gather(*(client.close() for client in clients if client), return_exceptions=True)


def flagged(audit):
    if not audit or not audit.get('azione_decisiva_iniziale') or not audit.get('continuita_narrativa_iniziale'):
        return None
    return bool(audit.get('modifiche_proposte') or
                audit['azione_decisiva_iniziale']['esito'] == 'incompleta' or
                audit['continuita_narrativa_iniziale']['esito'] == 'incoerente')


def classify(expected, observed):
    if observed is None:
        return 'inconcludente'
    return ('vero_positivo' if observed else 'falso_negativo') if expected else (
        'falso_positivo' if observed else 'vero_negativo')


def summary(rows):
    groups = defaultdict(list)
    for row in rows:
        groups[row['provider']].append(row)
    result = {}
    for provider, group in groups.items():
        counts = Counter(row['classificazione'] for row in group)
        tp, fp, fn = (counts[key] for key in ('vero_positivo', 'falso_positivo', 'falso_negativo'))
        result[provider] = {
            'casi': len(group), **{key: counts[key] for key in
              ('vero_positivo', 'falso_positivo', 'vero_negativo', 'falso_negativo', 'inconcludente')},
            'precisione_segnalazione': tp / (tp + fp) if tp + fp else None,
            'richiamo_su_esiti_validi': tp / (tp + fn) if tp + fn else None,
            'copertura': (len(group) - counts['inconcludente']) / len(group),
            'secondi_mediani': statistics.median(row['secondi'] for row in group),
            'token_input': sum(row['token_input'] for row in group),
            'token_output': sum(row['token_output'] for row in group),
            'esiti_revisione': dict(Counter(row.get('revisione', {}).get('esito', 'errore') for row in group)),
        }
    return result


def state_for(case, text=None):
    return {'prompt_originale': case['prompt'], 'eta_bambino': case['eta'],
            'racconto_finale': case['testo'] if text is None else text}


async def review_one(case, provider, repeat, semaphore):
    async with semaphore:
        start = time.monotonic()
        router = None
        row = {'caso': case['id'], 'provider': provider, 'ripetizione': repeat,
               'difettoso_atteso': case['difettoso'], 'motivo_atteso': case['motivo']}
        try:
            router = make_router(provider)
            result = await verifica_testo_finale(state_for(case), router)
            row.update(revisione=result['revisione_finale'], testo_finale=result['racconto_finale'])
        except Exception as exc:
            row['errore'] = str(exc)
        finally:
            row['tracce'] = router.traces if router else []
            if router:
                await close_router(router)
        row.update(secondi=round(time.monotonic() - start, 3),
                   token_input=sum(t['token_input'] for t in row['tracce']),
                   token_output=sum(t['token_output'] for t in row['tracce']))
        row['classificazione'] = classify(case['difettoso'], flagged(row.get('revisione')))
        print(f"{provider} {case['id']} #{repeat}: {row['classificazione']}", flush=True)
        return row


async def refine_one(case, provider, judge, repeat, semaphore):
    async with semaphore:
        editor = reviewer = None
        row = {'caso': case['id'], 'provider': provider, 'revisore': judge,
               'ripetizione': repeat, 'bozza': case['testo']}
        try:
            editor, reviewer = make_router(provider), make_router(judge)
            # Same input; only the refinement call differs between the two arms.
            refined = await editor.rifinisci(case['testo'], case['eta'], ritornello=None)
            row['rifinita'] = refined.testo
            row['diff'] = '\n'.join(difflib.unified_diff(case['testo'].splitlines(),
                refined.testo.splitlines(), fromfile='senza_rifinitura', tofile='con_rifinitura', lineterm=''))
            row['quota_testo_modificata'] = 1 - difflib.SequenceMatcher(
                None, case['testo'], refined.testo, autojunk=False).ratio()
            for label, text in [('senza_rifinitura', case['testo']), ('con_rifinitura', refined.testo)]:
                # The judge sees neither label nor expected answer.
                result = await verifica_testo_finale(state_for(case, text), reviewer)
                row[label] = result['revisione_finale']
            row['esito_editoriale'] = 'da_valutare_da_un_lettore'
        except Exception as exc:
            row['errore'] = str(exc)
        finally:
            row['tracce_rifinitura'] = editor.traces if editor else []
            row['tracce_revisore'] = reviewer.traces if reviewer else []
            for router in (editor, reviewer):
                if router:
                    await close_router(router)
        print(f"rifinitura {provider} {case['id']} #{repeat}: completato", flush=True)
        return row


def write_report(path, data):
    # Refuse to overwrite a previous experiment.
    with path.open('x', encoding='utf-8') as handle:
        json.dump(data, handle, ensure_ascii=False, indent=2)


async def run(args, data, cases):
    semaphore = asyncio.Semaphore(args.concurrency)
    rows = await asyncio.gather(*(review_one(case, provider, repeat, semaphore)
        for repeat in range(1, args.repeats + 1) for case in cases for provider in args.providers))
    refinements = []
    if args.refine_cases:
        selected = [case for case in cases if case['id'] in args.refine_cases]
        refinements = await asyncio.gather(*(refine_one(case, args.refiner, args.judge, repeat, semaphore)
            for repeat in range(1, args.repeats + 1) for case in selected))
    hashes = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
              for path in [Path(__file__), ROOT / 'services/narrative_review.py', ROOT / 'services/llm.py']}
    output = {
        'timestamp': datetime.now(timezone.utc).isoformat(),
        'corpus_version': data['version'], 'corpus_sha256': hashlib.sha256(args.corpus.read_bytes()).hexdigest(),
        'codice_sha256': hashes, 'ripetizioni': args.repeats, 'casi': cases,
        'nota': 'Le metriche misurano segnalazioni, non qualità letteraria né correttezza delle correzioni. '
                'Controllare motivazioni e falsi positivi. Le risposte restano stocastiche; '
                'gli esiti inconcludenti sono separati. Il confronto di rifinitura richiede lettura umana.',
        'riepilogo': summary(rows), 'risultati': rows, 'confronto_rifinitura': refinements,
    }
    write_report(args.output, output)
    print(json.dumps(output['riepilogo'], ensure_ascii=False, indent=2))
    print(f'Report: {args.output}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus', type=Path, default=CORPUS)
    parser.add_argument('--live', action='store_true', help='Autorizza le chiamate API del confronto')
    parser.add_argument('--providers', nargs='+', choices=['anthropic', 'deepseek', 'openai'], default=['anthropic', 'deepseek'])
    parser.add_argument('--cases', nargs='+')
    parser.add_argument('--repeats', type=int, default=1)
    parser.add_argument('--concurrency', type=int, choices=[1, 2, 3], default=2)
    parser.add_argument('--refine-cases', nargs='+', default=[])
    parser.add_argument('--refiner', choices=['anthropic', 'deepseek', 'openai'], default='deepseek')
    parser.add_argument('--judge', choices=['anthropic', 'deepseek', 'openai'], default='anthropic')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    data = load_cases(args.corpus)
    all_ids = {case['id'] for case in data['cases']}
    if args.repeats < 1 or set(args.cases or []) - all_ids or set(args.refine_cases) - all_ids:
        parser.error('Ripetizioni o identificatori dei casi non validi')
    cases = [case for case in data['cases'] if not args.cases or case['id'] in args.cases]
    if set(args.refine_cases) - {case['id'] for case in cases}:
        parser.error('I casi di rifinitura devono essere inclusi nei casi selezionati')
    if not args.live:
        print(f'Corpus valido: {len(cases)} casi. Nessuna chiamata API. Usa --live per eseguire il confronto.')
        return
    if not args.output or args.output.exists() or not args.output.parent.is_dir():
        parser.error('--output deve indicare un nuovo file in una directory esistente')
    asyncio.run(run(args, data, cases))


if __name__ == '__main__':
    main()
