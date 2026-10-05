import unittest
from services.conversations import ConversationStore


class ConversationTests(unittest.TestCase):
    def test_history_is_isolated_and_contains_complete_turns(self):
        store = ConversationStore()
        self.assertEqual(store.acquire('a'), [])
        store.release('a', 'Luna?', 'La Luna è visibile.')
        self.assertEqual(store.acquire('b'), [])
        store.release('b')
        history = store.acquire('a')
        self.assertEqual([m['role'] for m in history], ['user', 'assistant'])
        self.assertEqual(history[0]['content'], 'Luna?')
        history[0]['content'] = 'mutated copy'
        store.release('a')
        self.assertEqual(store.acquire('a')[0]['content'], 'Luna?')

    def test_failed_turn_is_not_remembered_and_session_is_released(self):
        store = ConversationStore()
        store.acquire('a')
        store.release('a', 'failed', None)
        self.assertEqual(store.acquire('a'), [])

    def test_overlapping_turn_is_rejected_without_dropping_active_history(self):
        store = ConversationStore(max_sessions=1)
        store.acquire('a')
        with self.assertRaisesRegex(RuntimeError, 'già in corso'):
            store.acquire('a')
        with self.assertRaisesRegex(RuntimeError, 'occupato'):
            store.acquire('b')
        store.release('a', 'question', 'answer')
        self.assertEqual(len(store.acquire('a')), 2)

    def test_expired_history_is_removed(self):
        now = [0]
        store = ConversationStore(ttl=10, clock=lambda: now[0])
        store.acquire('a'); store.release('a', 'question', 'answer')
        now[0] = 11
        self.assertEqual(store.acquire('a'), [])

    def test_limits_evict_old_idle_sessions_and_old_complete_turns(self):
        store = ConversationStore(max_sessions=2, max_turns=2, max_chars=20)
        for i in range(3):
            store.acquire('a'); store.release('a', str(i), 'reply')
        self.assertEqual([m['content'] for m in store.acquire('a')], ['1', 'reply', '2', 'reply'])
        store.release('a')
        for session in ['b', 'c']:
            store.acquire(session); store.release(session, 'hi', 'hello')
        self.assertEqual(store.acquire('a'), [])

    def test_character_limit_keeps_pairs_and_drops_oversized_turn(self):
        store = ConversationStore(max_chars=10)
        store.acquire('a'); store.release('a', 'x' * 100, 'y' * 100)
        self.assertEqual(store.acquire('a'), [])


if __name__ == '__main__':
    unittest.main()
