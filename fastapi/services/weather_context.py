"""Prompt weather overrides current weather; conditional weather is not an override."""
import re

CONDITIONS = re.compile(r'\b(torment\w*|bufer\w*|nevica\w*|piov\w*|pioggia|temporale|tempesta|grandine|nebbia|nevicat\w*|inverno\s+(?:rigido|gelido)|freddo\s+(?:intenso|gelido|pungente)|caldo\s+(?:intenso|torrido)|cielo\s+(?:sereno|coperto|nuvoloso)|vento\s+(?:forte|gelido)|giornata\s+(?:soleggiata|piovosa))\b', re.I)
CONDITIONAL = re.compile(r'\b(se|qualora|eventualmente|potrebbe|prevision\w*|che tempo|meteo reale)\b', re.I)


def prompt_weather(prompt, extracted=None):
    if isinstance(extracted, str) and extracted.strip() and extracted in prompt and not CONDITIONAL.search(extracted):
        return extracted.strip()
    parts = [m.group(1) for m in re.finditer(r'([^.!?;\n]+)([.!?;\n]|$)', prompt or '') if m.group(2) != '?']
    conditions = [part.strip() for part in parts if CONDITIONS.search(part) and not CONDITIONAL.search(part)]
    return ' '.join(conditions) or None
