"""Version history for narrative diagnostics; no extra model calls."""
import difflib
import hashlib


def snapshot(fase, testo, uso_llm=(), **metadata):
    return {'fase': fase, 'testo': testo,
            'sha256': hashlib.sha256(testo.encode('utf-8')).hexdigest(),
            'modelli': sorted({entry['modello'] for entry in uso_llm if entry.get('modello')}),
            **metadata}


def version_report(versions):
    result = []
    previous = None
    for version in versions:
        entry = dict(version)
        entry['parole'] = len(entry['testo'].split())
        entry['modificata'] = previous is not None and entry['testo'] != previous['testo']
        entry['diff_precedente'] = '' if previous is None else '\n'.join(difflib.unified_diff(
            previous['testo'].splitlines(), entry['testo'].splitlines(),
            fromfile=previous['fase'], tofile=entry['fase'], lineterm=''))
        result.append(entry)
        previous = entry
    return result
