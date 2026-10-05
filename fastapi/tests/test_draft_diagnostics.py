"""Integration of diagnostic accumulation with LangGraph and YAML reports."""
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import yaml
from langgraph.graph import StateGraph, START, END
from agent.state import BaldoState
from agent.nodes import valuta_draft
from services import report


class DraftDiagnosticsTests(unittest.IsolatedAsyncioTestCase):
    async def test_all_three_diagnoses_survive_in_report(self):
        responses = iter(['Manca una presa.', 'Manca un percorso di uscita.', 'OK'])
        class LLM:
            async def chiedi(self, **kwargs):
                return SimpleNamespace(testo=next(responses), modello='fake',
                    token_input=1, token_output=1, durata_secondi=.1)
        llm = LLM()
        graph = StateGraph(BaldoState)
        async def evaluate(state):
            update = await valuta_draft(state, llm)
            update['tentativi_correzione'] = state.get('tentativi_correzione', 0) + 1
            return update
        for name in ('first', 'second', 'third'):
            graph.add_node(name, evaluate)
        for source, target in [(START, 'first'), ('first', 'second'), ('second', 'third'), ('third', END)]:
            graph.add_edge(source, target)
        result = await graph.compile().ainvoke({'draft': 'Una storia.', 'storia_id': 'test'})
        with tempfile.TemporaryDirectory() as temp:
            with patch.object(report, 'DIRECTORY_STORIE', Path(temp)):
                report.salva_report_storia(result, .3)
            saved = yaml.safe_load((Path(temp) / 'test.yaml').read_text())
        self.assertEqual([d['diagnosi'] for d in saved['diagnosi_draft']],
                         ['Manca una presa.', 'Manca un percorso di uscita.', 'OK'])
        self.assertEqual([d['valutazione'] for d in saved['diagnosi_draft']], [1, 2, 3])
        self.assertEqual(saved['valutazione_draft_finale'], 'ok')
