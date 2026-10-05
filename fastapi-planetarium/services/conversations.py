"""Bounded conversation history. Graph checkpoints remain local to each request."""
from collections import OrderedDict
from dataclasses import dataclass, field
from threading import RLock
from time import monotonic


@dataclass
class Conversation:
    messages: list = field(default_factory=list)
    updated: float = 0
    busy: bool = False


class ConversationStore:
    def __init__(self, max_sessions=256, max_turns=8, max_chars=16000, ttl=3600, clock=monotonic):
        self.max_sessions = max_sessions
        self.max_turns = max_turns
        self.max_chars = max_chars
        self.ttl = ttl
        self.clock = clock
        self._sessions = OrderedDict()
        self._lock = RLock()

    def acquire(self, session_id):
        """Reserve a session; reject overlapping turns without accumulating waiters."""
        with self._lock:
            now = self.clock()
            for key, item in list(self._sessions.items()):
                if not item.busy and now - item.updated >= self.ttl:
                    del self._sessions[key]
            item = self._sessions.get(session_id)
            if item and item.busy:
                raise RuntimeError("Una risposta è già in corso per questa sessione.")
            if item is None:
                if len(self._sessions) >= self.max_sessions:
                    oldest = next((key for key, value in self._sessions.items() if not value.busy), None)
                    if oldest is None:
                        raise RuntimeError("Il servizio è occupato. Riprova tra un momento.")
                    del self._sessions[oldest]
                item = Conversation(updated=now)
                self._sessions[session_id] = item
            item.busy = True
            self._sessions.move_to_end(session_id)
            return [dict(message) for message in item.messages]

    def release(self, session_id, prompt=None, answer=None):
        with self._lock:
            item = self._sessions.get(session_id)
            if item is None:
                return
            if prompt and answer:
                item.messages.extend([{"role": "user", "content": prompt},
                                      {"role": "assistant", "content": answer[:self.max_chars]}])
                while len(item.messages) > self.max_turns * 2 or sum(len(m["content"]) for m in item.messages) > self.max_chars:
                    del item.messages[:2]
            item.busy = False
            item.updated = self.clock()


conversations = ConversationStore()
