import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, writeFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import Server from '../server_base.js';
import { validateSolarEnvironment } from '../js/services/native-loader.js';

test('missing native configuration is rejected before loading C++', () => {
    assert.throws(() => validateSolarEnvironment({}), /Configurazione Solar mancante: VSOP_DIR/);
    assert.doesNotThrow(() => validateSolarEnvironment({
        VSOP_DIR: '/test', DB_HOST: 'test', DB_USER: 'test', DB_PASS: 'test', DB_NAME: 'test',
    }));
});

test('closing one socket preserves another socket in the same visitor session', () => {
    const server = new Server(0);
    const first = {}, second = {};
    server.CLIENTS.first = first;
    server.CLIENTS.second = second;
    server.handleWsClose(first, { headers: { 'sec-websocket-key': 'first' } }, 'same-session');
    assert.equal(server.CLIENTS.first, undefined);
    assert.equal(server.CLIENTS.second, second);
    server.handleWsClose(second, { headers: { 'sec-websocket-key': 'second' } }, 'same-session');
    assert.equal(Object.keys(server.CLIENTS).length, 0);
});

test('a client cannot override the server configuration path', async () => {
    const directory = await mkdtemp(path.join(tmpdir(), 'baldo-config-'));
    const previous = process.env.SERVER_CONFIG_FILE;
    try {
        const config = path.join(directory, 'config.json');
        await writeFile(config, JSON.stringify({ marker: 'server-controlled' }));
        process.env.SERVER_CONFIG_FILE = config;
        const server = new Server(0);
        const context = await server.loadContext({
            url: '/api/session?config_file=/does-not-exist',
            connection: { encrypted: true }, headers: { host: 'localhost' },
        });
        assert.equal(context.impostazioni.marker, 'server-controlled');
    } finally {
        if (previous === undefined) delete process.env.SERVER_CONFIG_FILE;
        else process.env.SERVER_CONFIG_FILE = previous;
        await rm(directory, { recursive: true });
    }
});

test('concurrent WebSocket messages retain separate payloads', async () => {
    const server = new Server(0);
    const received = [];
    server.wss_command = async (_server, _ws, context) => {
        await new Promise(resolve => setImmediate(resolve));
        received.push(context.message.action);
    };
    const context = { SESSION: 'same-session' };
    await Promise.all(['first', 'second'].map(action =>
        server.handleWsMessage({}, Buffer.from(JSON.stringify({ action })), false, context)));
    assert.deepEqual(received, ['first', 'second']);
    assert.equal(context.message, undefined);
});

test('handler errors do not execute the command a second time', async t => {
    const server = new Server(0);
    let calls = 0;
    t.mock.method(console, 'error', () => {});
    server.wss_command = async () => { calls++; throw new Error('test failure'); };
    await server.handleWsMessage({}, Buffer.from('{"action":"update"}'), false, {});
    assert.equal(calls, 1);
});
