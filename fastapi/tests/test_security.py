import asyncio
import gc
import os
import unittest
import weakref
from typing import TypedDict
from unittest.mock import patch

import httpx
from fastapi import Depends, FastAPI
from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import END, START, StateGraph

import main
from services.admin_auth import require_admin


class SecurityTests(unittest.IsolatedAsyncioTestCase):
    async def test_admin_routes_reject_anonymous_requests(self):
        async with httpx.AsyncClient(transport=httpx.ASGITransport(app=main.app), base_url="http://test") as client:
            for method, url in [("GET", "/admin/stats"), ("GET", "/admin/modelli"),
                                ("DELETE", "/admin/frammenti"), ("POST", "/admin/frammenti/bulk"),
                                ("PUT", "/admin/modelli/genera_draft")]:
                response = await client.request(method, url, json={})
                self.assertEqual(response.status_code, 403, (url, response.text))

    async def test_admin_auth_requires_configured_secret(self):
        app = FastAPI()

        @app.get("/protected", dependencies=[Depends(require_admin)])
        def protected():
            return {"ok": True}

        secret = "test-secret-with-at-least-32-characters"
        async with httpx.AsyncClient(transport=httpx.ASGITransport(app=app), base_url="http://test") as client:
            with patch.dict(os.environ, {"ADMIN_TOKEN": secret}):
                for token, status in [("", 403), ("wrong", 403), (secret, 200)]:
                    response = await client.get("/protected", headers={"Authorization": f"Bearer {token}"})
                    self.assertEqual(response.status_code, status)
            with patch.dict(os.environ, {"ADMIN_TOKEN": ""}):
                response = await client.get("/protected", headers={"Authorization": f"Bearer {secret}"})
                self.assertEqual(response.status_code, 403)

    async def test_streams_isolate_and_release_checkpoints(self):
        class State(TypedDict):
            prompt_originale: str
            racconto_finale: str

        memories = []

        def build_graph(**dependencies):
            memory = MemorySaver()
            memories.append(weakref.ref(memory))
            graph = StateGraph(State)

            async def story(state):
                await asyncio.sleep(0)
                return {"racconto_finale": state["prompt_originale"]}

            graph.add_node("story", story)
            graph.add_edge(START, "story")
            graph.add_edge("story", END)
            return graph.compile(checkpointer=memory)

        async with httpx.AsyncClient(transport=httpx.ASGITransport(app=main.app), base_url="http://test") as client:
            with patch.object(main, "build_graph", side_effect=build_graph), patch.object(main, "salva_report_storia"):
                responses = await asyncio.gather(*[
                    client.post("/racconto/stream", json={"prompt": prompt, "session_id": "same-browser"})
                    for prompt in ["prima storia", "seconda storia"]
                ])
        self.assertIn('"racconto": "prima storia"', responses[0].text)
        self.assertNotIn('"racconto": "seconda storia"', responses[0].text)
        self.assertIn('"racconto": "seconda storia"', responses[1].text)
        self.assertEqual(len(memories), 2)
        gc.collect()
        self.assertTrue(all(ref() is None for ref in memories))

    async def test_invalid_story_input_is_rejected_before_generation(self):
        async with httpx.AsyncClient(transport=httpx.ASGITransport(app=main.app), base_url="http://test") as client:
            for fields in [{"prompt": ""}, {"prompt": "x" * 8001}, {"prompt": "ok", "lat": 91},
                           {"prompt": "ok", "eta_bambino": -1}]:
                response = await client.post("/racconto/stream", json=fields)
                self.assertEqual(response.status_code, 422)
