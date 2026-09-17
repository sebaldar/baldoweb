import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from evals.run import CORPUS, classify, flagged, load_cases, summary, write_report


class EvalTests(unittest.TestCase):
    def test_balanced_corpus_and_unique_cases(self):
        cases = load_cases(CORPUS)['cases']
        self.assertEqual(len(cases), 8)
        self.assertEqual(sum(case['difettoso'] for case in cases), 4)

    def test_inconclusive_not_counted_as_correct(self):
        self.assertIsNone(flagged({'esito': 'non_verificato'}))
        self.assertEqual(classify(True, None), 'inconcludente')
        self.assertEqual(classify(False, None), 'inconcludente')
        self.assertEqual(classify(False, True), 'falso_positivo')
        self.assertEqual(classify(True, False), 'falso_negativo')

    def test_summary_exposes_coverage_and_false_alarms(self):
        rows = [dict(provider='test', classificazione=classification, secondi=1,
                     token_input=2, token_output=3) for classification in
                ['vero_positivo', 'vero_negativo', 'falso_positivo', 'falso_negativo', 'inconcludente']]
        result = summary(rows)['test']
        self.assertEqual(result['precisione_segnalazione'], .5)
        self.assertEqual(result['richiamo_su_esiti_validi'], .5)
        self.assertEqual(result['copertura'], .8)
        self.assertEqual(result['token_input'], 10)

    def test_results_are_not_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'result.json'
            write_report(target, {'first': True})
            with self.assertRaises(FileExistsError):
                write_report(target, {'second': True})
            self.assertEqual(json.loads(target.read_text()), {'first': True})
