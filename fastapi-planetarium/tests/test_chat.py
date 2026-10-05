"""Real FastAPI/LangGraph flow; providers, vector search and weather are mocked."""
import asyncio
import gc
import importlib
import json
import sys
import types
import unittest
import weakref
from unittest.mock import patch

import httpx
from langgraph.checkpoint.memory import MemorySaver

# Import the actual graph without loading the embedding stack or contacting Qdrant.
rag = types.ModuleType('agent.nodes.node_rag')
rag.node_rag = lambda state, emitter: {'rag_docs': []}
previous_rag = sys.modules.get('agent.nodes.node_rag')
sys.modules['agent.nodes.node_rag'] = rag
try:
    chat_router = importlib.import_module('routers.chat')
    from main import app
finally:
    if previous_rag is None:
        sys.modules.pop('agent.nodes.node_rag', None)
    else:
        sys.modules['agent.nodes.node_rag'] = previous_rag

from agent.emitter import SseEmitter
from agent.nodes import node_classify, node_answer
from services.conversations import ConversationStore
from services import config


class FakeLLM:
    def __init__(self):
        self.classification_inputs = []
        self.answer_inputs = []

    def invoke(self, messages):
        self.classification_inputs.append(messages)
        return types.SimpleNamespace(content=json.dumps({'intenti': ['info'], 'corpi_celesti': ['Luna']}))

    def stream(self, messages):
        self.answer_inputs.append(messages)
        yield types.SimpleNamespace(content='La Luna ')
        yield types.SimpleNamespace(content='è visibile.')


class ProviderConfigTests(unittest.TestCase):
    def test_installed_providers_accept_timeout_and_retry_limits_without_api_calls(self):
        settings = config.Settings(_env_file=None, ionos_api_key='test-only',
            anthropic_api_key='test-only', openai_api_key='test-only', google_api_key='test-only')
        with patch.object(config, 'get_settings', return_value=settings):
            for provider in ['ionos', 'anthropic', 'openai', 'gemini']:
                with self.subTest(provider=provider):
                    model = config.get_llm(provider)
                    fields = model.model_dump(by_alias=True)
                    self.assertEqual(fields.get('timeout', fields.get('request_timeout')), 45)
                    self.assertEqual(model.max_retries, 1)


class ChatTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.llm = FakeLLM()
        self.patches = [patch.object(chat_router, 'conversations', ConversationStore()),
            patch.object(node_classify, 'get_llm', return_value=self.llm),
            patch.object(node_answer, 'get_llm', return_value=self.llm)]
        for item in self.patches:
            item.start()
        self.client = httpx.AsyncClient(transport=httpx.ASGITransport(app=app), base_url='http://test')

    async def asyncTearDown(self):
        await self.client.aclose()
        if chat_router.active_tasks:
            await asyncio.gather(*list(chat_router.active_tasks))
        for item in reversed(self.patches):
            item.stop()

    async def ask(self, prompt, session='a', **extra):
        response = await self.client.post('/api/chat', json={'prompt': prompt,
            'session_id': session, 'lat': 0, 'lon': 0, 'data': '2026-10-05T20:00:00Z', **extra})
        self.assertEqual(response.status_code, 200)
        return [json.loads(line) for line in response.text.splitlines()]

    async def test_followup_reaches_classifier_and_answer_with_session_history(self):
        first = await self.ask('Parlami della Luna')
        self.assertEqual([e['text'] for e in first if e['tipo'] == 'token'], ['La Luna ', 'è visibile.'])
        self.assertEqual(sum(e['tipo'] == 'final' for e in first), 1)
        await self.ask('E quando sarà visibile?')
        classify = self.llm.classification_inputs[-1]
        self.assertEqual([m.content for m in classify[1:]], ['Parlami della Luna', 'La Luna è visibile.', 'E quando sarà visibile?'])
        answer = self.llm.answer_inputs[-1]
        self.assertIn('Parlami della Luna', [m.content for m in answer])
        self.assertIn('lat=0.00°, lon=0.00°', answer[-1].content)

    async def test_other_sessions_do_not_receive_previous_conversation(self):
        await self.ask('Sessione A', 'a')
        await self.ask('Sessione B', 'b')
        self.assertEqual([m.content for m in self.llm.classification_inputs[-1][1:]], ['Sessione B'])

    async def test_invalid_input_is_rejected_before_graph_or_provider(self):
        for data in [{'prompt': ''}, {'prompt': 'x' * 2001}, {'prompt': 'ok', 'lat': 91},
                     {'prompt': 'ok', 'lon': 181}, {'prompt': 'ok', 'provider': 'unknown'}]:
            response = await self.client.post('/api/chat', json=data)
            self.assertEqual(response.status_code, 422)
        self.assertEqual(self.llm.classification_inputs, [])

    async def test_graph_setup_failure_emits_error_and_releases_session(self):
        async def failure(*args):
            raise RuntimeError('compile failed')
        with patch.object(chat_router, 'run_graph', side_effect=failure):
            events = await self.ask('first')
        self.assertEqual(events[-1]['tipo'], 'error')
        events = await self.ask('retry')
        self.assertEqual(events[-1]['tipo'], 'final')
        self.assertEqual([m.content for m in self.llm.classification_inputs[-1][1:]], ['retry'])

    async def test_per_request_checkpoints_are_released_after_completion(self):
        references = []
        def memory():
            obj = MemorySaver(); references.append(weakref.ref(obj)); return obj
        with patch.object(chat_router, 'MemorySaver', side_effect=memory):
            await self.ask('first'); await self.ask('second')
        gc.collect()
        self.assertTrue(all(ref() is None for ref in references))

    async def test_disconnect_cancels_emission_and_late_tokens_are_discarded(self):
        emitter = SseEmitter(); emitter.attach_loop(asyncio.get_running_loop())
        emitter.emit_sync('token', text='partial')
        stream = emitter.stream()
        self.assertIn('partial', await anext(stream))
        await stream.aclose()
        self.assertTrue(emitter.cancelled)
        emitter.emit_sync('token', text='late')
        await asyncio.sleep(0)
        self.assertTrue(emitter._queue.empty())

    async def test_duplicate_session_is_rejected_without_starting_second_graph(self):
        entered = asyncio.Event(); release = asyncio.Event()
        async def wait(state, emitter, memory, session):
            entered.set(); await release.wait()
            emitter.emit_sync('final', text='done'); emitter.close_sync()
            return {'answer_text': 'done'}
        with patch.object(chat_router, 'run_graph', side_effect=wait):
            first = asyncio.create_task(self.ask('first'))
            await entered.wait()
            second = await self.ask('second')
            self.assertEqual(second[-1]['tipo'], 'error')
            release.set(); await first


if __name__ == '__main__':
    unittest.main()
