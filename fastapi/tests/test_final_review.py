import asyncio
import json
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from services.narrative_review import apply_edits, verifica_testo_finale, response_format, continuity_quote

STORY = 'Lupetto arrivò al ruscello. Il tasso rompeva il ghiaccio con lo zoccolo. Poi lo accompagnò alla quercia, dove abitava sua madre.'
EDIT = dict(originale='con lo zoccolo', sostituzione='con la zampa', motivo='Anatomia del tasso')

class FakeLLM:
    def __init__(self, responses):
        self.responses = iter(responses)
        self.calls = []
    async def chiedi(self, **kwargs):
        self.calls.append(kwargs)
        value = next(self.responses)
        if isinstance(value, Exception):
            raise value
        return SimpleNamespace(testo=value, modello='fake', token_input=10, token_output=5, durata_secondi=.1)

def payload(edits):
    return json.dumps({'modifiche': edits, 'continuita_narrativa': {'esito': 'coerente', 'problemi': []}, 'azione_decisiva': {
        'passaggio': 'Poi lo accompagnò alla quercia, dove abitava sua madre.',
        'esito': 'coerente', 'collegamento_mancante': ''}})

class FinalReviewTests(unittest.TestCase):
    def review(self, llm):
        return asyncio.run(verifica_testo_finale(dict(racconto_finale=STORY, personaggi=['Lupetto'], eta_bambino=6), llm))

    def test_explicit_request_cannot_pass_without_audit(self):
        result = asyncio.run(verifica_testo_finale(
            dict(racconto_finale=STORY, prompt_originale='Scegli un pianeta.'),
            FakeLLM([payload([])])))
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_missing_planet_is_corrected_and_confirmed(self):
        story = 'La mamma apre la mappa. «Cerchiamo il pianeta verso est». Lupetto spinge il foglio con il righello e lo recupera. Le nuvole coprono tutto. Tornano a guardare la mappa.'
        request = 'Scegli un pianeta reale. Se ci sono nuvole, non fingere di vederlo.'
        def answer(text, missing=False, edits=None):
            return json.dumps({'modifiche': edits or [],
                'azione_decisiva': {'passaggio': 'Lupetto spinge il foglio con il righello e lo recupera.', 'esito': 'coerente', 'collegamento_mancante': ''},
                'continuita_narrativa': {'esito': 'coerente', 'problemi': []},
                'verifica_richiesta': [{'requisito': 'Scegli un pianeta reale.', 'esito': 'mancante' if missing else 'soddisfatto', 'evidenza': '' if missing else 'Cerchiamo Saturno verso est'}]})
        state = dict(racconto_finale=story, prompt_originale=request)
        unresolved = asyncio.run(verifica_testo_finale(state, FakeLLM([answer(story, True), '{"modifiche": []}'])))
        self.assertEqual(unresolved['revisione_finale']['esito'], 'problemi_non_risolti')
        edit = dict(originale='Cerchiamo il pianeta verso est', sostituzione='Cerchiamo Saturno verso est', motivo='Il pianeta richiesto va nominato')
        result = asyncio.run(verifica_testo_finale(state, FakeLLM([answer(story, True, [edit]), answer(story)])))
        self.assertEqual(result['revisione_finale']['esito'], 'corretto')
        self.assertIn('Saturno', result['racconto_finale'])
        recovered = asyncio.run(verifica_testo_finale(state, FakeLLM([
            answer(story, True), json.dumps({'modifiche': [edit]}), answer(story)])))
        self.assertEqual(recovered['revisione_finale']['esito'], 'corretto')
        self.assertEqual(recovered['racconto_finale'], result['racconto_finale'])
        self.assertEqual(len(recovered['llm_usage']), 3)
        rejected = asyncio.run(verifica_testo_finale(state, FakeLLM([
            answer(story, True), json.dumps({'modifiche': [edit]}), answer(story, True)])))
        self.assertEqual(rejected['racconto_finale'], story)
        self.assertEqual(rejected['revisione_finale']['esito'], 'problemi_non_risolti')

        self.assertEqual(result['revisione_finale']['richiesta_finale'][0]['esito'], 'soddisfatto')
        # An invented evidence quote must not validate an unchanged candidate.
        invalid = asyncio.run(verifica_testo_finale(state, FakeLLM([answer(story)])))
        self.assertEqual(invalid['revisione_finale']['esito'], 'non_verificato')

    def test_misquoted_requirement_correction_is_repaired_and_confirmed(self):
        # Osservato in produzione: la correzione automatica dei requisiti
        # mancanti citava il verbo sbagliato ("la porta" invece di "la
        # portò"), apply_edits falliva, e l'intera revisione veniva scartata
        # come non_verificato pur non avendo nulla di sbagliato nel racconto.
        story = ('Il sole del mattino scaldava appena la neve intorno alla tana. Il tasso raccolse '
                  'una nocciola e la portò a Lupetto, vicino al ruscello, mentre gli uccelli cantavano '
                  'tra i rami spogli.')
        request = 'Il tasso deve fare un regalo a Lupetto.'
        audit = json.dumps({
            'modifiche': [],
            'azione_decisiva': {'passaggio': story, 'esito': 'coerente', 'collegamento_mancante': ''},
            'continuita_narrativa': {'esito': 'coerente', 'problemi': []},
            'verifica_richiesta': [{'requisito': request, 'esito': 'mancante', 'evidenza': ''}],
        })
        bad_correction = json.dumps({'modifiche': [
            {'originale': 'Il tasso raccolse una noce e la porta a Lupetto',
             'sostituzione': 'Il tasso raccolse una nocciola e la regalò a Lupetto',
             'motivo': 'Rende esplicito il regalo'}]})
        good_correction = json.dumps({'modifiche': [
            {'originale': 'Il tasso raccolse una nocciola e la portò a Lupetto',
             'sostituzione': 'Il tasso raccolse una nocciola e gliela regalò',
             'motivo': 'Rende esplicito il regalo'}]})
        edited_story = ('Il sole del mattino scaldava appena la neve intorno alla tana. Il tasso raccolse '
                         'una nocciola e gliela regalò, vicino al ruscello, mentre gli uccelli cantavano '
                         'tra i rami spogli.')
        confirmation = json.dumps({
            'modifiche': [],
            'azione_decisiva': {'passaggio': edited_story, 'esito': 'coerente', 'collegamento_mancante': ''},
            'continuita_narrativa': {'esito': 'coerente', 'problemi': []},
            'verifica_richiesta': [{'requisito': request, 'esito': 'soddisfatto', 'evidenza': 'gliela regalò'}],
        })
        llm = FakeLLM([audit, bad_correction, good_correction, confirmation])
        result = asyncio.run(verifica_testo_finale(dict(racconto_finale=story, prompt_originale=request), llm))

        self.assertEqual(len(llm.calls), 4)
        self.assertEqual(llm.calls[2]['fase'], 'verifica_testo_finale.ripara_modifiche')
        self.assertEqual(result['revisione_finale']['esito'], 'corretto')
        self.assertIn('gliela regalò', result['racconto_finale'])

    def test_edit_repair_gives_up_after_one_retry(self):
        story = 'Il tasso raccolse una nocciola e la portò a Lupetto, vicino al ruscello.'
        request = 'Il tasso deve fare un regalo a Lupetto.'
        audit = json.dumps({
            'modifiche': [],
            'azione_decisiva': {'passaggio': story, 'esito': 'coerente', 'collegamento_mancante': ''},
            'continuita_narrativa': {'esito': 'coerente', 'problemi': []},
            'verifica_richiesta': [{'requisito': request, 'esito': 'mancante', 'evidenza': ''}],
        })
        bad_correction = json.dumps({'modifiche': [
            {'originale': 'frase mai comparsa nel racconto', 'sostituzione': 'altra frase', 'motivo': 'm'}]})
        llm = FakeLLM([audit, bad_correction, bad_correction])
        result = asyncio.run(verifica_testo_finale(dict(racconto_finale=story, prompt_originale=request), llm))

        self.assertEqual(len(llm.calls), 3)
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')
        self.assertEqual(result['racconto_finale'], story)

    def test_evidence_repair_keeps_story_and_valid_audits(self):
        data = json.loads(payload([]))
        data['verifica_richiesta'] = [{'requisito': 'Lupetto arriva al ruscello', 'esito': 'soddisfatto', 'evidenza': 'Lupetto raggiunge il ruscello'}]
        repaired = {'verifica_richiesta': [dict(data['verifica_richiesta'][0], evidenza='Lupetto arrivò al ruscello.')]}
        llm = FakeLLM([json.dumps(data), json.dumps(repaired)])
        result = asyncio.run(verifica_testo_finale(dict(racconto_finale=STORY, prompt_originale='Lupetto arriva al ruscello'), llm))
        self.assertEqual(result['revisione_finale']['esito'], 'ok')
        self.assertEqual(result['racconto_finale'], STORY)
        self.assertEqual(len(result['revisione_finale']['errori_evidenze']), 1)
        self.assertTrue(result['llm_usage'][-1]['nodo'].endswith('.ripara_evidenze'))

    def test_failed_repair_preserves_action_and_continuity(self):
        data = json.loads(payload([]))
        data['verifica_richiesta'] = [{'requisito': 'Lupetto arriva al ruscello', 'esito': 'soddisfatto', 'evidenza': 'Citazione inesistente'}]
        llm = FakeLLM([json.dumps(data), json.dumps(data)])
        result = asyncio.run(verifica_testo_finale(dict(racconto_finale=STORY, prompt_originale='Lupetto arriva al ruscello'), llm))
        audit = result['revisione_finale']
        self.assertEqual(audit['esito'], 'non_verificato')
        self.assertIsNotNone(audit['azione_decisiva_iniziale'])
        self.assertIsNotNone(audit['continuita_narrativa_iniziale'])
        self.assertIn('Citazione inesistente', audit['dettaglio'])
        self.assertEqual(len(llm.calls), 2)
        self.assertEqual(result['racconto_finale'], STORY)

    def test_whitespace_evidence_uses_original_span_without_llm_repair(self):
        from services.narrative_review import validate_request
        text = 'Un occhio si aprì.\n\n«Chi osa svegliarmi?»'
        audit = {'verifica_richiesta': [{'requisito': 'orso svegliato', 'esito': 'soddisfatto', 'evidenza': 'Un occhio si aprì. «Chi osa svegliarmi?»'}]}
        checks = validate_request(audit, text, 'orso svegliato')
        self.assertEqual(checks[0]['evidenza'], text)
        with self.assertRaises(ValueError):
            apply_edits(text, [dict(originale='Un occhio si aprì. «Chi osa svegliarmi?»', sostituzione='Nuovo testo', motivo='test')])

    def test_action_whitespace_and_independent_repair(self):
        story = STORY.replace(' Poi lo', '\n\nPoi lo')
        data = json.loads(payload([]))
        data['azione_decisiva']['passaggio'] = story.replace('\n\n', ' ')
        result = asyncio.run(verifica_testo_finale({'racconto_finale': story}, FakeLLM([json.dumps(data)])))
        self.assertEqual(result['revisione_finale']['esito'], 'ok')
        data['azione_decisiva']['passaggio'] = 'Passaggio inventato'
        llm = FakeLLM([json.dumps(data), json.dumps({'azione_decisiva': json.loads(payload([]))['azione_decisiva']})])
        result = self.review(llm)
        self.assertEqual(result['revisione_finale']['esito'], 'ok')
        self.assertEqual(len(llm.calls), 2)
        self.assertEqual(result['revisione_finale']['errori_evidenze'][0]['controllo'], 'azione_decisiva')
        failed = self.review(FakeLLM([json.dumps(data), json.dumps(data)]))
        self.assertEqual(failed['revisione_finale']['esito'], 'non_verificato')
        self.assertIsNotNone(failed['revisione_finale']['continuita_narrativa_iniziale'])
        self.assertEqual(failed['revisione_finale']['richiesta_iniziale'], [])

    def test_clean_text_unchanged_one_call(self):
        llm = FakeLLM([payload([])])
        result = self.review(llm)
        self.assertEqual(result['racconto_finale'], STORY)
        self.assertEqual(result['revisione_finale']['esito'], 'ok')
        self.assertEqual(len(llm.calls), 1)

    def test_local_edit_confirmed_and_tracked(self):
        llm = FakeLLM([payload([EDIT]), payload([])])
        result = self.review(llm)
        self.assertEqual(result['racconto_finale'], STORY.replace('con lo zoccolo', 'con la zampa'))
        self.assertEqual(result['revisione_finale']['esito'], 'corretto')
        self.assertEqual(len(result['llm_usage']), 2)
        self.assertIn('con la zampa', llm.calls[1]['user'])

    def test_unconfirmed_edit_keeps_original(self):
        result = self.review(FakeLLM([payload([EDIT]), payload([EDIT])]))
        self.assertEqual(result['racconto_finale'], STORY)
        self.assertEqual(result['revisione_finale']['esito'], 'correzione_non_confermata')

    def test_invalid_json_or_provider_failure_not_reported_as_ok(self):
        for response in ['not json', '{}', RuntimeError('unavailable')]:
            with self.subTest(response=response):
                result = self.review(FakeLLM([response]))
                self.assertEqual(result['racconto_finale'], STORY)
                self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_all_format_examples_have_required_action_field(self):
        decoder = json.JSONDecoder()
        for confirm in (False, True):
            prompt = response_format(confirm)
            example, _ = decoder.raw_decode(prompt[prompt.index('{'):])
            self.assertIn('azione_decisiva', example)
            self.assertIn('continuita_narrativa', example)
            self.assertEqual(example['modifiche'], [])

    def test_confirmation_missing_action_keeps_original_and_usage(self):
        result = self.review(FakeLLM([payload([EDIT]), '{"modifiche": []}']))
        self.assertEqual(result['racconto_finale'], STORY)
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')
        self.assertEqual(len(result['llm_usage']), 2)

    def test_missing_action_check_cannot_pass(self):
        result = self.review(FakeLLM([json.dumps({'modifiche': []})]))
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_incomplete_action_without_edit_is_not_ok(self):
        data = json.loads(payload([]))
        data['azione_decisiva'].update(esito='incompleta', collegamento_mancante='Manca un aggancio')
        result = self.review(FakeLLM([json.dumps(data)]))
        self.assertEqual(result['revisione_finale']['esito'], 'problemi_non_risolti')
        self.assertEqual(result['racconto_finale'], STORY)

    def test_fabricated_action_quote_is_rejected(self):
        data = json.loads(payload([]))
        data['azione_decisiva']['passaggio'] = 'Una frase che non compare.'
        result = self.review(FakeLLM([json.dumps(data)]))
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_continuity_ellipsis_expands_only_existing_ordered_evidence(self):
        text = 'Arrivarono davanti alla tana del riccio. Si salutarono. Ognuno andò dalla sua nonna.'
        quote = 'Arrivarono davanti alla tana del riccio... Ognuno andò dalla sua nonna.'
        self.assertEqual(continuity_quote(quote, text), text)
        for invalid in [
            'Ognuno andò dalla sua nonna... Arrivarono davanti alla tana del riccio',
            'Arrivarono davanti alla tana del riccio... La nonna li accolse insieme',
        ]:
            with self.assertRaises(ValueError):
                continuity_quote(invalid, text)
        with self.assertRaises(ValueError):
            apply_edits(text, [dict(originale=quote, sostituzione='Nuovo finale', motivo='Continuità')])

    def test_json_fence_is_accepted_without_accepting_extra_prose(self):
        result = self.review(FakeLLM(['```json\n' + payload([]) + '\n```']))
        self.assertEqual(result['revisione_finale']['esito'], 'ok')
        result = self.review(FakeLLM(['Commento: ' + payload([])]))
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_astronomy_data_reaches_initial_check_and_confirmation(self):
        llm = FakeLLM([payload([EDIT]), payload([])])
        astronomy = {'status': 'reale', 'corpi': [{'nome': 'Saturno', 'azimut_deg': 90, 'direzione_cardinale': 'est'}]}
        asyncio.run(verifica_testo_finale(dict(racconto_finale=STORY, eta_bambino=6,
            usa_astronomia=True, dati_astronomici=astronomy, luogo='Roma', ora_storia='21:00:00'), llm))
        self.assertEqual(len(llm.calls), 2)
        for call in llm.calls:
            context = json.loads(call['user'])['contesto_astronomico']
            self.assertEqual(context['dati_motore'], astronomy)
            self.assertEqual(context['osservatore']['ora_locale'], '21:00:00')

    def test_missing_continuity_cannot_pass(self):
        data = json.loads(payload([]))
        del data['continuita_narrativa']
        result = self.review(FakeLLM([json.dumps(data)]))
        self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_incoherent_destination_without_edits_cannot_pass(self):
        data = json.loads(payload([]))
        data['continuita_narrativa'] = {'esito': 'incoerente', 'problemi': [{
            'passaggio': 'Poi lo accompagnò alla quercia, dove abitava sua madre.',
            'motivo': 'La destinazione richiesta era diversa.'}]}
        result = self.review(FakeLLM([json.dumps(data)]))
        self.assertEqual(result['revisione_finale']['esito'], 'problemi_non_risolti')
        self.assertEqual(result['racconto_finale'], STORY)
        self.assertEqual(result['revisione_finale']['continuita_narrativa_finale'], data['continuita_narrativa'])

    def test_confirmation_with_unresolved_continuity_keeps_original(self):
        data = json.loads(payload([]))
        data['continuita_narrativa'] = {'esito': 'incoerente', 'problemi': [{
            'passaggio': 'Poi lo accompagnò alla quercia, dove abitava sua madre.',
            'motivo': 'La destinazione resta sbagliata.'}]}
        result = self.review(FakeLLM([payload([EDIT]), json.dumps(data)]))
        self.assertEqual(result['racconto_finale'], STORY)
        self.assertEqual(result['revisione_finale']['esito'], 'problemi_non_risolti')
        self.assertEqual(result['revisione_finale']['continuita_narrativa_finale']['esito'], 'coerente')
        self.assertEqual(result['revisione_finale']['dettaglio']['continuita_narrativa']['esito'], 'incoerente')

    def test_continuity_rejects_inconsistent_or_invented_evidence(self):
        for audit in [
            {'esito': 'incoerente', 'problemi': []},
            {'esito': 'coerente', 'problemi': [{'passaggio': 'Lupetto', 'motivo': 'Contraddizione'}]},
            {'esito': 'incoerente', 'problemi': [{'passaggio': 'Un finale inventato', 'motivo': 'Contraddizione'}]},
        ]:
            with self.subTest(audit=audit):
                data = json.loads(payload([]))
                data['continuita_narrativa'] = audit
                result = self.review(FakeLLM([json.dumps(data)]))
                self.assertEqual(result['revisione_finale']['esito'], 'non_verificato')

    def test_reject_missing_ambiguous_overlapping_or_wholesale_edits(self):
        for text, edits in [
            (STORY, [dict(EDIT, originale='inesistente')]),
            (STORY + STORY, [EDIT]),
            (STORY, [EDIT, dict(EDIT, originale='lo zoccolo')]),
            (STORY, [dict(EDIT, originale=STORY)]),
        ]:
            with self.subTest(edits=edits):
                with self.assertRaises(ValueError):
                    apply_edits(text, edits)

    def test_multiple_edits_do_not_cascade(self):
        edits = [dict(EDIT, originale='zoccolo', sostituzione='ghiaccio'),
                 dict(EDIT, originale='ghiaccio', sostituzione='neve')]
        result = apply_edits(STORY, edits)
        self.assertIn('neve con lo ghiaccio', result)

    def test_typographic_apostrophe_in_originale_still_matches_straight_text(self):
        # Il modello a volte "abbellisce" gli apici in JSON (’ invece di ')
        # anche quando il testo originale li usa dritti ovunque: la citazione
        # resta fedele al contenuto, va accettata comunque.
        text = "L'orso dorme nella grotta fredda."
        edit = dict(originale='L’orso dorme', sostituzione='L’orso russa forte',
                     motivo='variare il verbo')
        result = apply_edits(text, [edit])
        self.assertEqual(result, "L'orso russa forte nella grotta fredda.")

    def test_apostrophe_only_typographic_difference_is_not_a_valid_edit(self):
        text = "L'orso dorme nella grotta fredda."
        edit = dict(originale="L'orso dorme", sostituzione='L’orso dorme', motivo='nessun cambiamento reale')
        with self.assertRaises(ValueError):
            apply_edits(text, [edit])

if __name__ == '__main__':
    unittest.main()
