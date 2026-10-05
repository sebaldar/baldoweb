"""Riepilogo di tempi, token e costi dai report YAML in stories/.

Uso: python tools/stories_stats.py [cartella] [--json]
"""
import json
import statistics
import sys
from pathlib import Path

import yaml

DEFAULT_DIR = Path(__file__).resolve().parent.parent / "stories"


def percentile(values, pct):
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int(round(pct / 100 * (len(ordered) - 1))))] if ordered else None


def summarize(directory):
    reports = []
    for path in sorted(Path(directory).glob("*.yaml")):
        try:
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
        except (OSError, yaml.YAMLError):
            continue
        if isinstance(data, dict):
            reports.append(data)

    times = [r["tempo_elaborazione_secondi"] for r in reports if isinstance(r.get("tempo_elaborazione_secondi"), (int, float))]
    costs = [r["costo_stimato_usd"] for r in reports if isinstance(r.get("costo_stimato_usd"), (int, float))]
    nodes = {}
    for report in reports:
        for name, row in (report.get("uso_per_nodo") or {}).items():
            total = nodes.setdefault(name, {"chiamate": 0, "token_output": 0, "durata_secondi": 0.0})
            total["chiamate"] += row.get("chiamate", 0)
            total["token_output"] += row.get("token_output", 0)
            total["durata_secondi"] = round(total["durata_secondi"] + row.get("durata_secondi", 0), 2)
    return {
        "storie": len(reports),
        "tempo_medio_s": round(statistics.mean(times), 1) if times else None,
        "tempo_p95_s": percentile(times, 95),
        "token_input_totali": sum(r.get("token_input") or 0 for r in reports),
        "token_output_totali": sum(r.get("token_output") or 0 for r in reports),
        "storie_con_costo": len(costs),
        "costo_totale_usd": round(sum(costs), 4) if costs else None,
        "costo_medio_usd": round(statistics.mean(costs), 4) if costs else None,
        "per_nodo": dict(sorted(nodes.items(), key=lambda item: -item[1]["durata_secondi"])),
    }


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    summary = summarize(args[0] if args else DEFAULT_DIR)
    if "--json" in argv:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
        return
    for key, value in summary.items():
        if key != "per_nodo":
            print(f"{key:22} {value}")
    print("\nnodi (per secondi totali):")
    for name, row in summary["per_nodo"].items():
        print(f"  {name:36} {row['durata_secondi']:8.1f}s  {row['chiamate']:3} chiamate  {row['token_output']:7} token out")


if __name__ == "__main__":
    main(sys.argv[1:])
