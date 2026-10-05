import unittest

from services.story_choices import AMBIENTAZIONI_AMMESSE, GENERI_AMMESSI, normalizza_scelte


class StoryChoicesTests(unittest.TestCase):
    def test_existing_single_choices_keep_their_format(self):
        self.assertEqual(normalizza_scelte("amicizia", GENERI_AMMESSI), "amicizia")
        self.assertEqual(normalizza_scelte("bosco", AMBIENTAZIONI_AMMESSE), "bosco")

    def test_multiple_choices_are_preserved(self):
        self.assertEqual(normalizza_scelte("amicizia, avventura, nanna", GENERI_AMMESSI), "amicizia, avventura, nanna")
        self.assertEqual(normalizza_scelte("bosco, castello, cielo", AMBIENTAZIONI_AMMESSE), "bosco, castello, cielo")

    def test_unknown_and_duplicate_choices_are_removed(self):
        self.assertEqual(normalizza_scelte("amicizia, sconosciuto, amicizia, nanna", GENERI_AMMESSI), "amicizia, nanna")
        self.assertEqual(normalizza_scelte(" bosco, castello, bosco ", AMBIENTAZIONI_AMMESSE), "bosco, castello")

    def test_no_allowed_choices_becomes_none(self):
        for value in [None, "", "sconosciuto", "sconosciuto, altro", ["bosco"]]:
            with self.subTest(value=value):
                self.assertIsNone(normalizza_scelte(value, AMBIENTAZIONI_AMMESSE))
