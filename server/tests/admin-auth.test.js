import test from 'node:test';
import assert from 'node:assert/strict';
import { canRunAction, isAdminToken } from '../js/services/admin-auth.js';
import loader from '../js/api/ILPICCOLOBARDO/loader.js';
import bardo from '../js/pagine/IL_PICCOLO_BARDO/main.js';

const secret = 'test-admin-secret-with-more-than-32-characters';

test('privileged actions fail closed and public generation remains accessible', () => {
    process.env.ADMIN_TOKEN = secret;
    for (const action of ['clear_database', 'delete_fragment', 'set_model_routing', 'list_fragments', 'future_admin_action']) {
        assert.equal(canRunAction(action), false);
        assert.equal(canRunAction(action, 'wrong'), false);
        assert.equal(canRunAction(action, secret), true);
    }
    assert.equal(canRunAction('generate_story'), true);
    assert.equal(isAdminToken('', ''), false);
    assert.equal(isAdminToken('short', 'short'), false);
    assert.equal(isAdminToken({ token: secret }, secret), false);
    delete process.env.ADMIN_TOKEN;
    assert.equal(canRunAction('clear_database', secret), false);
});

test('WebSocket rejects destructive commands before opening database connections', async () => {
    process.env.ADMIN_TOKEN = secret;
    const messages = [];
    const ws = { OPEN: 1, readyState: 1, send: value => messages.push(JSON.parse(value)) };
    await bardo.exe({}, ws, { action: 'clear_database', data: {} });
    assert.equal(messages[0].code, 'ADMIN_REQUIRED');
});

function response() {
    return {
        writeHead(status, headers) { this.statusCode = status; this.headers = headers; },
        setHeader() {},
        end(body) { this.body = body; },
    };
}

test('HTTP proxy rejects missing credentials and mutating GET requests', async () => {
    process.env.ADMIN_TOKEN = secret;
    for (const [method, headers, expected] of [
        ['POST', {}, 403],
        ['POST', { authorization: 'Bearer invalid' }, 403],
        ['GET', { authorization: `Bearer ${secret}` }, 405],
    ]) {
        const res = response();
        await loader.exe({ request: { method, headers }, response: res, query: { action: 'admin_delete_fragments' } });
        assert.equal(res.statusCode, expected);
    }
});

test('authorized HTTP proxy forwards service credentials and payload', async t => {
    process.env.ADMIN_TOKEN = secret;
    const calls = [];
    t.mock.method(globalThis, 'fetch', async (url, options) => {
        calls.push({ url, options });
        return { ok: true, json: async () => ({ eliminati: 1 }) };
    });
    const res = response();
    await loader.exe({
        request: { method: 'POST', headers: { authorization: `Bearer ${secret}` } },
        response: res, query: { action: 'admin_delete_fragments', data: { ids: ['fragment-1'] } },
    });
    assert.equal(res.statusCode, 200);
    assert.equal(calls[0].options.headers.Authorization, `Bearer ${secret}`);
    assert.deepEqual(JSON.parse(calls[0].options.body), { ids: ['fragment-1'] });
});
