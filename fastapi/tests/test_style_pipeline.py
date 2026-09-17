import json
from types import SimpleNamespace
import unittest
import httpx
from agent.nodes import verifica_coerenza_domanda, decide_tools, fetch_contesto_fisico
from agent.graph import build_graph
from services.astronomy import AstronomyClient
from services.llm import RisultatoLLM


class StylePipelineTests(unittest.IsolatedAsyncioTestCase):
    def test_entire_graph_imports_and_compiles(self):
        dependency = object()
        graph = build_graph(dependency, dependency, dependency, dependency, dependency)
        nodes = graph.get_graph().nodes
        for required in ('componi_prompt', 'genera_draft', 'correggi_draft', 'rifinisci', 'verifica_testo_finale'):
            self.assertIn(required, nodes)

    async def test_blizzard_does_not_use_astronomy(self):
        class NoLLM:
            async def chiedi(self, **kwargs):
                raise AssertionError('No sky classification needed')
        result = await decide_tools({'prompt_originale': 'In un inverno rigido i lupi hanno scarsità di cibo. Una tormenta separa Lupetto dal branco. Una renna lo aiuta.', 'ora_storia': '23:00:00'}, NoLLM())
        self.assertFalse(result['usa_astronomia'])
        self.assertEqual(result['llm_usage'], [])
        result = await decide_tools({'prompt_originale': 'Osserviamo Saturno da Roma alle 21'}, NoLLM())
        self.assertTrue(result['usa_astronomia'])

    async def test_prompt_weather_skips_service_and_preserves_astronomy(self):
        class Geo:
            async def get_coordinates(self, *args, **kwargs): return (41.9, 12.5)
        class Weather:
            calls = 0
            async def get_weather(self, *args):
                self.calls += 1
                return 'pioggia attuale'
        class Astronomy:
            calls = 0
            async def get_sky_data(self, **kwargs):
                self.calls += 1
                return {'status': 'reale', 'corpi': []}
        weather, astronomy = Weather(), Astronomy()
        for prompt in ['In un inverno rigido una tormenta sorprende Lupetto.', 'Il cielo sereno permette di osservare Saturno.']:
            result = await fetch_contesto_fisico({'prompt_originale': prompt, 'usa_astronomia': True}, astronomy, weather, Geo())
            self.assertEqual(result['fonte_meteo'], 'prompt')
            self.assertIn(prompt.rstrip('.'), result['dati_meteo'])
        self.assertEqual(weather.calls, 0)
        self.assertEqual(astronomy.calls, 2)
        result = await fetch_contesto_fisico({'prompt_originale': 'Osserva il cielo: se piove cambia programma.', 'usa_astronomia': False}, astronomy, weather, Geo())
        self.assertEqual(result['fonte_meteo'], 'tool')
        self.assertEqual(weather.calls, 1)
        self.assertEqual(astronomy.calls, 2)

    async def test_many_comparisons_do_not_trigger_a_full_rewrite(self):
        calls = []
        class LLM:
            async def chiedi(self, **kwargs):
                calls.append(kwargs['fase'])
                return RisultatoLLM(testo='SI', modello='fake')
        story = 'Il cielo era come un mare. La luna come una barca. Le stelle come luci. Il vento come un soffio.'
        result = await verifica_coerenza_domanda({'racconto_finale': story}, LLM())
        self.assertEqual(result['racconto_finale'], story)
        self.assertEqual(result['similitudini_stimate'], 4)
        self.assertNotIn('verifica_coerenza_domanda.similitudini', calls)

    async def test_astronomy_preserves_bearings_and_converts_local_time(self):
        requests = []
        def handler(request):
            requests.append(request)
            return httpx.Response(200, json={'status': 'success', 'info_cielo': {
                'oggetti_visibili': [{'nome': 'Saturno', 'altezza_deg': 9, 'azimut_deg': 90, 'direzione_cardinale': 'est'}],
                'fase_giorno': 'notte', 'data_osservazione': '17-9-2026 19:0:0'}})
        async with httpx.AsyncClient(transport=httpx.MockTransport(handler)) as http:
            client = AstronomyClient(http)
            client.base_url = 'http://test/sky'
            result = await client.get_sky_data('127.0.0.1', lat=41.9, lon=12.5,
                data_storia='17-09-2026', ora_storia='21:00:00')
        self.assertEqual(requests[0].url.params['ora'], '19:00:00')
        self.assertEqual(result['corpi'][0]['direzione_cardinale'], 'est')

    async def test_unavailable_engine_does_not_invent_a_moon(self):
        async with httpx.AsyncClient(transport=httpx.MockTransport(lambda request: httpx.Response(503))) as http:
            client = AstronomyClient(http)
            client.base_url = 'http://test/sky'
            result = await client.get_sky_data('127.0.0.1')
        self.assertEqual(result['status'], 'fallback')
        self.assertEqual(result['corpi'], [])
