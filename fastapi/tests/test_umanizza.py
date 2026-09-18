import json
import unittest
from types import SimpleNamespace

from agent.nodes import umanizza


def response(text):
    return SimpleNamespace(testo=text, modello='fake', token_input=10, token_output=5, durata_secondi=.1)


class FakeLLM:
    """Records every call to umanizza() and replays canned responses in order."""

    def __init__(self, responses, provider='anthropic'):
        self.responses = iter(responses)
        self.calls = []
        self.routing = SimpleNamespace(get_provider=lambda fase: provider)

    async def umanizza(self, **kwargs):
        self.calls.append(kwargs)
        value = next(self.responses)
        if isinstance(value, Exception):
            raise value
        return response(value)


def diagnosi_json(edits):
    return json.dumps({'diagnosi': edits})


class UmanizzaTests(unittest.IsolatedAsyncioTestCase):
    async def test_valid_diagnosis_applies_targeted_edit(self):
        draft = 'Lupetto sentì il cuore battere forte. Poi corse verso il branco.'
        edit = dict(categoria='costrutto-stampella', originale='battere forte',
                     sostituzione='martellare piano', motivo='variare il costrutto ripetuto')
        llm = FakeLLM([diagnosi_json([edit])])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertEqual(result['racconto_finale'],
                          'Lupetto sentì il cuore martellare piano. Poi corse verso il branco.')
        self.assertEqual(result['versioni_racconto'][0]['fase'], 'umanizza')
        self.assertEqual(result['versioni_racconto'][0]['testo'], result['racconto_finale'])
        self.assertEqual(result['versioni_racconto'][0]['diagnosi'], [edit])
        self.assertEqual(result['llm_usage'][0]['nodo'], 'umanizza')

    async def test_json_wrapped_in_code_fence_is_accepted(self):
        draft = 'Lupetto sentì il cuore battere forte. Poi corse verso il branco.'
        edit = dict(categoria='costrutto-stampella', originale='battere forte',
                     sostituzione='martellare piano', motivo='variare il costrutto ripetuto')
        llm = FakeLLM(['```json\n' + diagnosi_json([edit]) + '\n```'])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertIn('martellare piano', result['racconto_finale'])

    async def test_bypass_provider_skips_call_entirely(self):
        llm = FakeLLM([diagnosi_json([])], provider='nessuno')

        result = await umanizza({'racconto_finale': 'Un racconto qualsiasi.', 'eta_bambino': 6}, llm)

        self.assertEqual(result, {})
        self.assertEqual(llm.calls, [])

    async def test_missing_racconto_finale_skips_call(self):
        llm = FakeLLM([diagnosi_json([])])

        result = await umanizza({'eta_bambino': 6}, llm)

        self.assertEqual(result, {})
        self.assertEqual(llm.calls, [])

    async def test_invalid_json_falls_back_to_unchanged_text(self):
        draft = 'Lupetto corse veloce nella neve fresca del mattino.'
        llm = FakeLLM(['questo non è JSON valido {'])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertNotIn('racconto_finale', result)
        self.assertEqual(result['llm_usage'][0]['nodo'], 'umanizza')

    async def test_missing_diagnosi_field_falls_back_to_unchanged_text(self):
        draft = 'Lupetto corse veloce nella neve fresca del mattino.'
        llm = FakeLLM([json.dumps({'modifiche': []})])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertNotIn('racconto_finale', result)

    async def test_quote_not_found_in_text_falls_back_to_unchanged_text(self):
        draft = 'Il lupo corse veloce nella neve fresca.'
        edit = dict(categoria='dialogo-piatto', originale='una frase mai scritta nel racconto',
                     sostituzione='altra frase', motivo='inventata')
        llm = FakeLLM([diagnosi_json([edit])])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertNotIn('racconto_finale', result)

    async def test_more_than_six_edits_are_rejected(self):
        draft = 'Testo di prova per il test dei limiti di modifica ammessi in questo racconto.'
        edits = [dict(categoria='c', originale=f'x{i}', sostituzione=f'y{i}', motivo='m') for i in range(7)]
        llm = FakeLLM([diagnosi_json(edits)])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertNotIn('racconto_finale', result)

    async def test_large_length_deviation_is_rejected(self):
        draft = ('Lupetto sentì il cuore battere forte. Poi sentì il vento gelido sul muso. '
                  'Infine sentì la neve sotto le zampe.')
        edit = dict(
            categoria='dettaglio-incidentale',
            originale='sul muso.',
            sostituzione=('sul muso freddo e pungente, che il vento continuava a sferzare '
                           'senza sosta per tutta la notte gelida.'),
            motivo='troppo dettaglio aggiunto',
        )
        llm = FakeLLM([diagnosi_json([edit])])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertNotIn('racconto_finale', result)

    async def test_length_deviation_between_five_and_eight_percent_is_accepted(self):
        # Quasi tutte le categorie spingono ad aggiungere testo: con più di un
        # edit contemporaneo capita di superare il vecchio tetto del 5% per pura
        # somma, pur restando ragionevoli (osservato in produzione: +5,6% su 5
        # edit validi, scartati tutti). Il tetto è stato alzato all'8%.
        draft = ' '.join(f'w{i}' for i in range(1, 51))  # 50 parole
        edit = dict(categoria='dettaglio-incidentale', originale='w25 w26 w27',
                     sostituzione='w25 w26 w27 con più forza', motivo='aggiunge dettaglio')
        llm = FakeLLM([diagnosi_json([edit])])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertIn('w25 w26 w27 con più forza', result['racconto_finale'])

    async def test_edit_introducing_a_reduplication_is_neutralized_by_the_safety_net(self):
        # "forte forte" è già la reduplicazione ammessa nel testo di partenza;
        # se la modifica ne introduce una seconda ("piano piano"), la rete di
        # sicurezza _limita_reduplicazioni la riassorbe e il risultato torna
        # identico all'originale: il nodo non deve segnalarlo come modificato.
        draft = ('Il vento soffiava forte forte contro la grotta buia e fredda, mentre la neve '
                  'cadeva senza sosta sul sentiero ghiacciato. Lupetto camminava piano nella neve '
                  'profonda, cercando con lo sguardo le tracce del branco lontano.')
        edit = dict(categoria='costrutto-stampella', originale='camminava piano',
                     sostituzione='camminava piano piano', motivo='variare il ritmo')
        llm = FakeLLM([diagnosi_json([edit])])

        result = await umanizza({'racconto_finale': draft, 'eta_bambino': 6}, llm)

        self.assertNotIn('racconto_finale', result)

    async def test_eta_and_ritornello_are_forwarded_to_the_llm(self):
        draft = 'Piccolo lupo, piccolo lupo, dove sei? Lupetto corse nella neve.'
        llm = FakeLLM([diagnosi_json([])])

        await umanizza({'racconto_finale': draft, 'eta_bambino': 7,
                         'ritornello_atteso': 'Piccolo lupo, piccolo lupo, dove sei?'}, llm)

        self.assertEqual(llm.calls[0]['draft'], draft)
        self.assertEqual(llm.calls[0]['eta'], 7)
        self.assertEqual(llm.calls[0]['ritornello'], 'Piccolo lupo, piccolo lupo, dove sei?')


if __name__ == '__main__':
    unittest.main()
