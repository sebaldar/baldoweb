import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import yaml
from langgraph.graph import StateGraph, START, END
from agent.state import BaldoState
from agent.nodes import genera_draft, correggi_draft, rifinisci
from services.narrative_review import verifica_testo_finale
from services import report


def response(text):
    return SimpleNamespace(testo=text, modello='fake', token_input=1, token_output=1, durata_secondi=.1)


class VersionIntegrationTests(unittest.IsolatedAsyncioTestCase):
    async def test_generation_corrections_refinement_and_final_survive_yaml(self):
        final = 'La volpina aprì la porta. La nonna la salutò.'
        class LLM:
            routing = SimpleNamespace(get_provider=lambda phase: 'test')
            corrections = 0
            async def genera_racconto(self, prompt): return response('Bozza iniziale.')
            async def rifinisci(self, **kwargs): return response(final)
            async def chiedi(self, **kwargs):
                if kwargs['fase'] == 'correggi_draft':
                    self.corrections += 1
                    return response(f'Bozza corretta {self.corrections}.')
                return response(json.dumps({'modifiche': [],
                    'azione_decisiva': {'esito': 'coerente', 'passaggio': 'La volpina aprì la porta.', 'collegamento_mancante': ''},
                    'continuita_narrativa': {'esito': 'coerente', 'problemi': []}}))
        llm = LLM()
        graph = StateGraph(BaldoState)
        from functools import partial
        phases = [('genera', genera_draft), ('correggi1', correggi_draft),
                  ('correggi2', correggi_draft), ('rifinisci', rifinisci), ('finale', verifica_testo_finale)]
        previous = START
        for name, function in phases:
            graph.add_node(name, partial(function, llm=llm))
            graph.add_edge(previous, name)
            previous = name
        graph.add_edge(previous, END)
        state = await graph.compile().ainvoke({'prompt_originale': 'La volpina visita la nonna.',
            'eta_bambino': 6, 'valutazione_draft': 'Correggi il testo.', 'storia_id': 'versions'})
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(report, 'DIRECTORY_STORIE', Path(directory)):
                report.salva_report_storia(state, 1)
            saved = yaml.safe_load((Path(directory) / 'versions.yaml').read_text())
        versions = saved['versioni_racconto']
        self.assertEqual([v['testo'] for v in versions],
                         ['Bozza iniziale.', 'Bozza corretta 1.', 'Bozza corretta 2.', final, final])
        self.assertEqual([v['tentativo'] for v in versions if 'tentativo' in v], [1, 2])
        self.assertEqual(versions[-1]['testo'], saved['storia_generata'])

    async def test_bypass_records_text_before_deterministic_cleanup(self):
        llm = SimpleNamespace(routing=SimpleNamespace(get_provider=lambda phase: 'nessuno'))
        draft = 'Camminò piano piano e parlò forte forte.'
        result = await rifinisci({'eta_bambino': 6, 'draft': draft}, llm)
        self.assertEqual(result['versioni_racconto'][0]['testo'], draft)
        self.assertTrue(result['versioni_racconto'][0]['bypass'])
        self.assertEqual(result['versioni_racconto'][-1]['testo'], result['racconto_finale'])
        self.assertEqual(result['llm_usage'], [])
