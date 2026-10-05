import test from 'node:test';
import assert from 'node:assert/strict';
import bardo from '../js/pagine/IL_PICCOLO_BARDO/main.js';
import PiccoloBardoManager from '../js/pagine/IL_PICCOLO_BARDO/PiccoloBardoManager.js';

for (const failure of [false, true]) {
    test(`speech ${failure ? 'errors' : 'responses'} echo the request ID`, async t => {
        const previous = process.env.OPENAI_API_KEY;
        process.env.OPENAI_API_KEY = 'test-key';
        t.mock.method(console, 'log', () => {});
        t.mock.method(console, 'error', () => {});
        t.mock.method(PiccoloBardoManager.prototype, 'initNeo4j', async () => {});
        t.mock.method(PiccoloBardoManager.prototype, 'synthesizeSpeech', async text => {
            assert.equal(text, 'Una favola');
            if (failure) throw new Error('Voice unavailable');
            return '/voice.mp3';
        });
        try {
            const messages = [];
            const ws = { OPEN: 1, readyState: 1, send: value => messages.push(JSON.parse(value)) };
            await bardo.exe({}, ws, { action: 'synthesize_speech', data: { text: 'Una favola', requestId: 'speech-test-1' } });
            assert.equal(messages.length, 1);
            assert.equal(messages[0].requestId, 'speech-test-1');
            assert.equal(messages[0].action, failure ? 'error' : 'speech_generated');
            if (failure) assert.equal(messages[0].requestAction, 'synthesize_speech');
            else assert.equal(messages[0].audioUrl, '/voice.mp3');
        } finally {
            if (previous === undefined) delete process.env.OPENAI_API_KEY;
            else process.env.OPENAI_API_KEY = previous;
        }
    });
}
