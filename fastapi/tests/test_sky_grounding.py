import json
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from services.sky_context import sky_context, sky_prompt
from services.composer import StoryComposer


class SkyGroundingTests(unittest.TestCase):
    def test_full_coordinates_are_in_generation_prompt(self):
        data = {'status': 'reale', 'fase_giorno': 'notte', 'corpi': [
            {'nome': 'Saturno', 'altezza_deg': 9, 'azimut_deg': 90, 'direzione_cardinale': 'est'}]}
        prompt, _ = StoryComposer().componi('Guarda Saturno da Roma alle 21.',
            {'personaggi': ['Lupetto'], 'dati_astronomici': data, 'ora_storia': '21:00:00'}, [], eta_bambino=6)
        self.assertIn('"direzione_cardinale": "est"', prompt)
        self.assertIn('"altezza_deg": 9', prompt)
        self.assertIn('"azimut_deg": 90', prompt)

    def test_missing_direction_or_fallback_does_not_create_coordinates(self):
        data = {'status': 'reale', 'corpi': [{'nome': 'Saturno', 'altezza_deg': 9}]}
        self.assertNotIn('direzione_cardinale', sky_context(data)['dati_motore']['corpi'][0])
        self.assertEqual(sky_context(None)['dati_motore']['corpi'], [])
        self.assertIn('non inventare una direzione', sky_prompt(data))
