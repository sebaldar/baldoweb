"""Offline regressions: load pure helpers/nodes without cloud or DB dependencies."""
import ast
import asyncio
import logging
from pathlib import Path
import re
from types import SimpleNamespace
from typing import Optional
import unittest
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from services.story_versions import snapshot
from services.story_style import detect_refrain, count_similitudes

SOURCE = Path(__file__).resolve().parents[1] / 'agent' / 'nodes.py'
NAMES = {'_RITORNELLO_MIN_PAROLE', '_PUNTEGGIATURA_BORDO_RE', '_TERMINALE_RE',
         '_parola_normalizzata', '_rileva_ritornello', '_COME_NON_COMPARATIVO',
         '_conta_similitudini_approssimate', '_uso_revisione', 'valuta_draft', 'correggi_draft'}
namespace = dict(detect_refrain=detect_refrain, count_similitudes=count_similitudes, snapshot=snapshot, REVIEW_CRITERIA="", re=re, Optional=Optional, BaldoState=dict, LLMRouter=object,
                 logger=logging.getLogger(__name__), _emit=lambda *args: None)
tree = ast.parse(SOURCE.read_text())
selected = [node for node in tree.body if getattr(node, 'name', None) in NAMES or
            isinstance(node, ast.Assign) and any(getattr(t, 'id', None) in NAMES for t in node.targets)]
exec(compile(ast.Module(body=selected, type_ignores=[]), str(SOURCE), 'exec'), namespace)


class NarrativeTests(unittest.TestCase):
    def test_partial_repetition_is_not_a_refrain(self):
        story = ('Un filo di luce, dal sole al bosco.\n\nCamminò ancora.\n\n'
                 'Un filo di luce, dal sole al bosco: lo aveva riportato a casa.')
        self.assertIsNone(namespace['_rileva_ritornello'](story))

    def test_whole_dialogue_and_standalone_refrains(self):
        spoken = '«Anch’io ho un’idea piccola!»'
        self.assertEqual(namespace['_rileva_ritornello'](spoken + ' disse.\n\n' + spoken),
                         'Anch’io ho un’idea piccola')
        phrase = 'Drin drin! Pronto? Una storia piccola piccola.'
        self.assertEqual(namespace['_rileva_ritornello']('\n\n'.join([phrase] * 3)), phrase.rstrip('.'))

    def test_real_false_positives_are_not_protected(self):
        for story in [
            'Fece un buchino tra le dita. Guardò nel buchino tra le dita.',
            'La volpina posò la focaccia sulla pietra. La volpina posò la focaccia sul tavolo.',
            'Era grande, con le squame ben aperte. La trovò grande, con le squame ben aperte.',
        ]:
            self.assertIsNone(namespace['_rileva_ritornello'](story))

    def test_repetition_in_one_exchange_is_not_refrain(self):
        self.assertIsNone(namespace['_rileva_ritornello']('«Non so dove andare» disse piano. «Non so dove andare.»'))

    def test_no_refrain(self):
        self.assertIsNone(namespace['_rileva_ritornello']('Lupetto chiamò. La mamma rispose.'))

    def test_count_includes_ending_without_question(self):
        self.assertEqual(namespace['_conta_similitudini_approssimate'](
            'Lupetto correva.\n\nLa neve brillava come vetro.'), 1)

    def test_review_and_correction_record_usage_and_update_refrain(self):
        class LLM:
            async def chiedi(self, **kwargs):
                return SimpleNamespace(testo='La luce risolve tutto senza un indizio.'
                    if kwargs['fase'] == 'valuta_draft' else 'Lupetto chiamò. La mamma rispose.',
                    modello='fake', token_input=10, token_output=8, durata_secondi=.1)
        state = dict(draft='Lupetto seguì la luce fino a casa.', prompt_originale='Lupetto si perde', eta_bambino=6)
        state.update(asyncio.run(namespace['valuta_draft'](state, LLM())))
        self.assertNotEqual(state['valutazione_draft'], 'ok')
        self.assertEqual(state['diagnosi_draft'][0]['diagnosi'], state['valutazione_draft'])
        self.assertEqual(state['diagnosi_draft'][0]['valutazione'], 1)
        self.assertEqual(state['llm_usage'][0]['nodo'], 'valuta_draft')
        result = asyncio.run(namespace['correggi_draft'](state, LLM()))
        self.assertEqual(result['tentativi_correzione'], 1)
        self.assertIsNone(result['ritornello_atteso'])
        self.assertIn('La mamma rispose', result['draft'])
        self.assertEqual(result['llm_usage'][0]['nodo'], 'correggi_draft')

    def test_diagnoses_include_success_after_previous_corrections(self):
        class LLM:
            async def chiedi(self, **kwargs):
                return SimpleNamespace(testo='OK', modello='fake', token_input=1,
                                       token_output=1, durata_secondi=.1)
        result = asyncio.run(namespace['valuta_draft'](
            dict(draft='Una storia.', tentativi_correzione=2), LLM()))
        self.assertEqual(result['diagnosi_draft'], [{
            'valutazione': 3, 'correzioni_precedenti': 2, 'esito': 'ok', 'diagnosi': 'OK'}])

if __name__ == '__main__':
    unittest.main()
