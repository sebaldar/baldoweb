import asyncio
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from agent.nodes import valuta_prompt, verifica_sicurezza_output
from agent.graph import _route_sicurezza_output
from services.content_safety import find_inappropriate, parse_verdict


class FakeLLM:
    def __init__(self, text):
        self.text, self.calls = text, 0

    async def chiedi(self, **kwargs):
        self.calls += 1
        return SimpleNamespace(testo=self.text, modello="fake", token_input=1, token_output=1, durata_secondi=.1)


def judge(llm, **extra):
    state = dict(prompt_originale="Un orsetto nel bosco", personaggi=["orsetto"], **extra)
    return asyncio.run(valuta_prompt(state, llm))


class FilterTests(unittest.TestCase):
    def test_flags_clear_terms_and_ignores_fairy_tale_peril(self):
        self.assertEqual(find_inappropriate("Una storia sul suicidio"), "suicidio")
        self.assertEqual(find_inappropriate("ok", ["il drago", "Eroina"]), "eroina")
        self.assertIsNone(find_inappropriate("Il lupo cattivo inseguì la strega nel castello buio."))
        self.assertIsNone(find_inappropriate("Il sessantesimo giorno", None, 5))

    def test_parse_verdict_handles_fences_and_noise(self):
        self.assertEqual(parse_verdict('```json\n{"chiaro": false, "motivo": "x"}\n```'), {"chiaro": False, "motivo": "x"})
        self.assertEqual(parse_verdict('Ecco: {"chiaro": true}'), {"chiaro": True})
        for bad in ("", "non json", "[1]", None):
            self.assertIsNone(parse_verdict(bad))


class PromptJudgeTests(unittest.TestCase):
    def test_filter_rejects_without_calling_the_llm(self):
        llm = FakeLLM('{"chiaro": true}')
        result = asyncio.run(valuta_prompt(dict(prompt_originale="Storia di sesso", personaggi=[]), llm))
        self.assertFalse(result["prompt_chiaro"])
        self.assertEqual(llm.calls, 0)

    def test_free_form_fields_are_checked(self):
        result = judge(FakeLLM('{"chiaro": true}'), animale_preferito="cocaina")
        self.assertFalse(result["prompt_chiaro"])

    def test_llm_verdict_is_honoured_even_inside_code_fence(self):
        result = judge(FakeLLM('```json\n{"chiaro": false, "motivo": "troppo cupo"}\n```'))
        self.assertFalse(result["prompt_chiaro"])
        self.assertEqual(result["motivo_rifiuto"], "troppo cupo")

    def test_unreadable_verdict_passes_to_the_deterministic_layer(self):
        self.assertTrue(judge(FakeLLM("boh"))["prompt_chiaro"])


class OutputCheckTests(unittest.TestCase):
    def test_unsafe_story_is_routed_to_error(self):
        state = dict(racconto_finale="Lupetto pensò al suicidio.")
        state.update(asyncio.run(verifica_sicurezza_output(state)))
        self.assertEqual(_route_sicurezza_output(state), "nodo_errore")
        self.assertTrue(state["motivo_rifiuto"])

    def test_clean_story_is_saved(self):
        state = dict(racconto_finale="Lupetto tornò a casa.")
        self.assertEqual(asyncio.run(verifica_sicurezza_output(state)), {})
        self.assertEqual(_route_sicurezza_output(state), "salva_memoria")


if __name__ == "__main__":
    unittest.main()
