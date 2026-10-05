import json
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

import httpx

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import main
from services.report import aggrega_per_nodo, stima_costo_usd
from tools.stories_stats import summarize

USAGE = [
    {"nodo": "genera_draft", "modello": "m-grande", "token_input": 2000, "token_output": 1000, "durata_secondi": 8.0},
    {"nodo": "genera_draft", "modello": "m-grande", "token_input": 1000, "token_output": 500, "durata_secondi": 4.0},
    {"nodo": "valuta_prompt", "modello": "m-piccolo", "token_input": 100, "token_output": 20, "durata_secondi": 0.5},
]
PRICES = {"m-grande": {"input": 3.0, "output": 15.0}, "m-piccolo": {"input": 1.0, "output": 5.0}}


class FakeGraph:
    async def astream_events(self, *args, **kwargs):
        yield {"event": "on_chat_model_stream", "name": "llm", "data": {"chunk": SimpleNamespace(content="bozza grezza")}}

    def get_state(self, config):
        return SimpleNamespace(values={"racconto_finale": "Storia finale.", "personaggi": []})


class StreamingTests(unittest.IsolatedAsyncioTestCase):
    async def post(self):
        async with httpx.AsyncClient(transport=httpx.ASGITransport(app=main.app), base_url="http://test") as client:
            with patch.object(main, "build_graph", return_value=FakeGraph()), patch.object(main, "salva_report_storia"):
                return (await client.post("/racconto/stream", json={"prompt": "una storia"})).text

    async def test_raw_tokens_are_held_back_by_default(self):
        with patch.dict("os.environ", {"STREAM_DRAFT_TOKENS": "0"}):
            text = await self.post()
        self.assertNotIn("bozza grezza", text)
        self.assertIn('"racconto": "Storia finale."', text)

    async def test_raw_tokens_can_be_enabled(self):
        with patch.dict("os.environ", {"STREAM_DRAFT_TOKENS": "1"}):
            self.assertIn("bozza grezza", await self.post())


class CostTests(unittest.TestCase):
    def test_cost_is_estimated_from_configured_prices(self):
        self.assertAlmostEqual(stima_costo_usd(USAGE, PRICES), (3000 * 3 + 1500 * 15 + 100 + 100) / 1e6, places=6)

    def test_cost_is_none_when_a_price_is_missing(self):
        self.assertIsNone(stima_costo_usd(USAGE, {"m-grande": PRICES["m-grande"]}))
        self.assertIsNone(stima_costo_usd(USAGE, {}))

    def test_usage_is_grouped_by_node(self):
        nodes = aggrega_per_nodo(USAGE)
        self.assertEqual(nodes["genera_draft"], {"chiamate": 2, "token_input": 3000, "token_output": 1500, "durata_secondi": 12.0})

    def test_stats_summarise_reports(self):
        import yaml
        with tempfile.TemporaryDirectory() as directory:
            for i, (seconds, cost) in enumerate([(30, 0.02), (50, None)]):
                report = {"tempo_elaborazione_secondi": seconds, "token_input": 100, "token_output": 50,
                          "uso_per_nodo": aggrega_per_nodo(USAGE)}
                if cost is not None:
                    report["costo_stimato_usd"] = cost
                (Path(directory) / f"{i}.yaml").write_text(yaml.safe_dump(report), encoding="utf-8")
            summary = summarize(directory)
        self.assertEqual(summary["storie"], 2)
        self.assertEqual(summary["tempo_medio_s"], 40)
        self.assertEqual(summary["costo_totale_usd"], 0.02)
        self.assertEqual(next(iter(summary["per_nodo"])), "genera_draft")


if __name__ == "__main__":
    unittest.main()
