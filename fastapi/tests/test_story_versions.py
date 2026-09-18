import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import copy
import unittest
from services.story_versions import snapshot, version_report


class StoryVersionTests(unittest.TestCase):
    def test_history_preserves_all_texts_and_diffs_without_mutating_state(self):
        versions = [snapshot('genera_draft', 'Prima frase.'),
                    snapshot('rifinisci', 'Prima frase.', bypass=True),
                    snapshot('verifica_testo_finale', 'Frase corretta.')]
        before = copy.deepcopy(versions)
        report = version_report(versions)
        self.assertEqual(versions, before)
        self.assertFalse(report[1]['modificata'])
        self.assertEqual(report[1]['diff_precedente'], '')
        self.assertTrue(report[2]['modificata'])
        self.assertIn('-Prima frase.', report[2]['diff_precedente'])
        self.assertIn('+Frase corretta.', report[2]['diff_precedente'])
        self.assertNotEqual(report[0]['sha256'], report[2]['sha256'])

    def test_legacy_report_has_no_invented_draft(self):
        self.assertEqual(version_report([]), [])
