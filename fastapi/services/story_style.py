"""Conservative refrain detection and descriptive style metrics."""
import re
from collections import Counter


def detect_refrain(text):
    # A repeated noun phrase or action embedded in prose is not a refrain.
    # Protect repeated whole spoken lines, or an isolated line repeated 3+ times.
    candidates = []
    # Count a spoken phrase at most once per paragraph/dialogue exchange.
    quoted = []
    for paragraph in re.split(r'\n\s*\n', text or ''):
        quoted.extend(set(re.sub(r"\s+", " ", match.group(1)).strip('«»“”" .!?;:') for match in re.finditer(r'[«“"]([^«»“”"\n]+)[»”"]', paragraph)))
    paragraphs = [part.strip() for part in re.split(r'\n\s*\n', text or '')]
    for units, minimum in [(quoted, 2), (paragraphs, 3)]:
        counts = Counter(re.sub(r'\s+', ' ', unit).strip('«»“”" .!?;:') for unit in units)
        for unit, count in counts.items():
            if count >= minimum and 4 <= len(unit.split()) <= 12:
                candidates.append((count, len(unit), unit))
    return max(candidates)[2] if candidates else None


def count_similitudes(text, refrain=None):
    paragraphs = (text or '').split('\n\n')
    body = '\n\n'.join(paragraphs[:-1]) if len(paragraphs) > 1 and paragraphs[-1].rstrip().endswith('?') else text or ''
    if refrain:
        body = body.replace(refrain, '')
    normalized = body.lower()
    total = len(re.findall(r'\bcome\b', normalized))
    for idiom in ('come mai', 'come stai', 'come sta', 'come va', 'come si chiama',
                  'come ti chiami', 'come faccio', 'come fai', 'come si fa'):
        total -= normalized.count(idiom)
    return max(0, total)
