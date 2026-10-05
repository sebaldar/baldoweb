import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import { EventEmitter } from 'node:events';
import { computeSnapshot, simulationConfig } from '../js/services/planetarium-snapshot.js';
import { handleChat, streamChat } from '../js/services/planetarium-chat.js';

const stateSource = readFileSync(new URL('../../planetarium/planetarium-state.js', import.meta.url), 'utf8');
const html = readFileSync(new URL('../../planetarium/index.html', import.meta.url), 'utf8');
function state() {
    const context = vm.createContext({});
    vm.runInContext(stateSource, context);
    return context.PlanetariumSimulation.create({ data: '2026-10-05T20:10:30Z' });
}
const plain = value => JSON.parse(JSON.stringify(value));
const xml = values => ({ querySelector: selector => {
    const key = selector.match(/id="([^"]+)"/)?.[1] || selector;
    return key in values ? { getAttribute: () => String(values[key]) } : null;
} });

test('selected observer including zero coordinates is also used by chat', () => {
    const sim = state();
    sim.trackCommand('observe 0 0 S');
    assert.deepEqual(plain(sim.chatContext()), { lat: 0, lon: 0, data: '2026-10-05T20:10:30Z', az: 180, alt: 0 });
    assert.throws(() => sim.setLocation(91, 0), /Coordinate/);
});

test('reconnect restores position, UTC time, view and running state', () => {
    const sim = state();
    ['observe 12.5 41.9 E', 'date 06-10-2026 01:02:03 UT', 'view 123 45',
        'zoom 2', 'fov 60', 'light off', 'horizont false', 'execute_loop off'].forEach(sim.trackCommand);
    const restored = state();
    sim.restorationCommands().forEach(restored.trackCommand);
    assert.deepEqual(plain(restored.snapshot()), plain(sim.snapshot()));
    assert.equal(sim.restorationCommands().filter(c => c.startsWith('date ')).length, 1);
});

test('rendered coordinates and clock update chat while active date editing is preserved', () => {
    const sim = state();
    sim.updateFromXml(xml({ latitudine: 0, longitudine: 12, azimut: 270, height: 30, the_date: '6-10-2026 1:2:3' }));
    assert.deepEqual(plain(sim.chatContext()), { lat: 0, lon: 12, az: 270, alt: 30, data: '2026-10-06T01:02:03Z' });
    sim.updateFromXml(xml({ the_date: '1-1-2020 0:0:0' }), true);
    assert.equal(sim.snapshot().data, '2026-10-06T01:02:03Z');
    sim.updateFromXml(xml({ 'camera zoom': 1.5, 'camera fov': 55 }));
    assert.equal(sim.snapshot().zoom, 1.5);
    assert.equal(sim.snapshot().fov, 55);
});

test('native config parses old timezone-less timestamps explicitly as UTC', () => {
    assert.equal(simulationConfig({ lat: 0, lon: 0, data: '2026-10-05T12:13:14' }).date, '05-10-2026 12:13:14');
    assert.equal(simulationConfig({ lat: 0, lon: 0, data: '2026-10-05T14:13:14+02:00' }).date, '05-10-2026 12:13:14');
    assert.throws(() => simulationConfig({ lat: 0, lon: 0, data: 'invalid' }), /Data/);
    assert.throws(() => simulationConfig({ lat: null, lon: 0, data: '2026-10-05' }), /Coordinate/);
});

test('temporary native calculations preserve live client 100 and clean up on failure', () => {
    const clients = new Map([[100, 'live']]);
    const solar = { registerClient(id) { clients.set(id, 'temporary'); },
        computeCelestialPositions(id, config) {
            assert.ok(id < 0);
            assert.equal(JSON.parse(config).save_snapshot, false);
            throw new Error('native failure');
        },
        unregisterClient(id) { clients.delete(id); } };
    assert.throws(() => computeSnapshot(solar, {}), /native failure/);
    assert.deepEqual([...clients], [[100, 'live']]);
});

function socket() {
    const ws = new EventEmitter();
    Object.assign(ws, { OPEN: 1, readyState: 1, events: [], clientData: { sessionId: 'trusted-session' },
        send(raw) { this.events.push(JSON.parse(raw)); } });
    return ws;
}
function response(lines) {
    return new Response(lines.map(line => typeof line === 'string' ? line : JSON.stringify(line)).join('\n'));
}

test('stream forwards tokens, source events and final with request correlation', async () => {
    const ws = socket();
    await streamChat(ws, { request_id: 'r1' }, { fetchImpl: async () => response([
        { tipo: 'thinking', text: 'Analizzo' }, { tipo: 'rag_result', found: 2 },
        { tipo: 'token', text: 'Luna è' }, { tipo: 'final', text: 'Luna è visibile' }]) });
    assert.deepEqual(ws.events.map(e => e.tipo), ['thinking', 'rag_result', 'token', 'final']);
    assert.ok(ws.events.every(e => e.request_id === 'r1' && e.action === 'chat_result'));
    assert.equal(ws.listenerCount('close'), 0);
});

test('stream decodes split UTF-8, SSE metadata and a final line without newline', async () => {
    const bytes = new TextEncoder().encode('event: message\ndata: {"tipo":"token","text":"è"}\n\ndata: {"tipo":"final","text":"è"}');
    const body = new ReadableStream({ start(controller) {
        for (const byte of bytes) controller.enqueue(Uint8Array.of(byte));
        controller.close();
    } });
    const ws = socket();
    await streamChat(ws, {}, { fetchImpl: async () => new Response(body) });
    assert.deepEqual(ws.events.map(e => e.text), ['è', 'è']);
});

test('timeout aborts upstream and removes the socket close listener', async () => {
    const ws = socket();
    await assert.rejects(streamChat(ws, {}, { timeoutMs: 5, fetchImpl: (_url, { signal }) =>
        new Promise((_resolve, reject) => signal.addEventListener('abort', () => reject(signal.reason), { once: true })) }), /scaduto/);
    assert.equal(ws.listenerCount('close'), 0);
});

test('socket close aborts a stalled stream', async () => {
    const ws = socket();
    const pending = streamChat(ws, {}, { fetchImpl: async () => {
        queueMicrotask(() => ws.emit('close'));
        return new Response(new ReadableStream({}));
    } });
    await assert.rejects(pending, /chiusa/);
    assert.equal(ws.listenerCount('close'), 0);
});

test('EOF without final is an error instead of leaving a request waiting', async () => {
    await assert.rejects(streamChat(socket(), {}, { fetchImpl: async () => response([{ tipo: 'token', text: 'partial' }]) }), /interrotta/);
});

test('chat uses the authenticated socket session and preserves zero coordinates', async () => {
    const ws = socket();
    let body;
    const solar = { registerClient() {}, unregisterClient() {}, computeCelestialPositions() { return '{"bodies":[]}'; } };
    await handleChat(solar, ws, { text: 'Luna?', data: '2026-10-05T20:00:00Z', lat: 0, lon: 0,
        session: 'forged-session', request_id: 'r1' }, { fetchImpl: async (_url, options) => {
        body = JSON.parse(options.body); return response([{ tipo: 'final', text: 'Risposta' }]);
    } });
    assert.equal(body.session_id, 'trusted-session');
    assert.equal(body.lat, 0); assert.equal(body.lon, 0);
    assert.equal(body.data, '2026-10-05T20:00:00.000Z');
    assert.equal(ws.planetariumChatPending, false);
});

test('native and backend failures produce the same recoverable error event', async t => {
    t.mock.method(console, 'error', () => {});
    for (const nativeFailure of [true, false]) {
        const ws = socket();
        const solar = { registerClient() {}, unregisterClient() {}, computeCelestialPositions() {
            if (nativeFailure) throw new Error('native'); return '{}';
        } };
        await handleChat(solar, ws, { text: 'Luna?', lat: 1, lon: 1, data: '2026-10-05', request_id: 'r1' },
            { fetchImpl: async () => new Response('', { status: 503 }) });
        assert.equal(ws.events[0].tipo, 'error');
        assert.equal(ws.events[0].request_id, 'r1');
        assert.equal(ws.planetariumChatPending, false);
    }
});

function frontend() {
    const elements = new Map();
    function element() {
        return { value: '', innerHTML: '', textContent: '', style: {}, children: [], disabled: false,
            classList: { add() {}, remove() {}, toggle() {} }, addEventListener() {}, focus() {},
            appendChild(child) { child.parent = this; this.children.push(child); },
            remove() { this.parent.children.splice(this.parent.children.indexOf(this), 1); } };
    }
    const sent = [], commands = [], timers = new Map();
    const context = { console, simulation: state(), wsIsOpen: () => true,
        wsSend: payload => sent.push(payload), sendCommand: cmd => commands.push(cmd),
        setTimeout(fn) { const id = Symbol(); timers.set(id, fn); return id; },
        clearTimeout(id) { timers.delete(id); },
        document: { createElement: element, getElementById(id) {
            if (!elements.has(id)) elements.set(id, element()); return elements.get(id);
        } } };
    context.window = context;
    vm.createContext(context);
    const start = html.indexOf('(function()', html.indexOf('// ── Chatbot'));
    const end = html.indexOf('// ── Formattazione', start);
    vm.runInContext(html.slice(start, end), context);
    elements.get('chatbot-input') ?? context.document.getElementById('chatbot-input');
    const ask = (text = 'Luna?') => { elements.get('chatbot-input').value = text; context.chatbotSend(); };
    return { context, elements, sent, commands, timers, ask };
}

test('frontend does not execute rag_result and keeps waiting until a terminal event', () => {
    const f = frontend(); f.ask();
    f.context.chatbotReceive({ tipo: 'rag_result', found: 2 });
    assert.deepEqual(f.commands, []);
    assert.equal(f.elements.get('chatbot-send-btn').disabled, true);
    f.ask('Seconda domanda'); assert.equal(f.sent.length, 1);
    f.context.chatbotReceive({ tipo: 'final', text: 'Risposta' });
    assert.equal(f.elements.get('chatbot-send-btn').disabled, false);
    assert.equal(f.timers.size, 0);
});

test('frontend renders tokens safely, deduplicates final and accepts zero declination', () => {
    const f = frontend(); f.ask();
    f.context.chatbotReceive({ tipo: 'token', text: '<img onerror=x>' });
    const messages = f.elements.get('chatbot-messages');
    assert.equal(messages.children.at(-1).textContent, '<img onerror=x>');
    f.context.chatbotReceive({ tipo: 'final', text: 'Risposta', extra: { ra: 0, dec: 0, target: 'Test' } });
    assert.equal(messages.children.length, 2);
    assert.equal(f.commands[0], 'sky 0 0');
    f.context.chatbotReceive({ tipo: 'final', text: 'duplicata' });
    assert.equal(messages.children.length, 2);
});

test('frontend displays legacy errors and ignores stale events after reconnect', () => {
    const f = frontend(); f.ask();
    const old = f.sent[0].request_id;
    f.context.chatbotReceive({ action: 'chat_error', message: 'Backend assente' });
    assert.ok(f.elements.get('chatbot-messages').children.at(-1).innerHTML.includes('Backend assente'));
    f.ask();
    f.context.chatbotReceive({ tipo: 'cmd', cmd: 'view S', request_id: old });
    assert.deepEqual(f.commands, []);
    f.context.chatbotDisconnected();
    assert.equal(f.elements.get('chatbot-send-btn').disabled, false);
    assert.equal(f.timers.size, 0);
});

test('frontend timeout releases controls and chat sends selected simulation context', () => {
    const f = frontend();
    f.context.simulation.trackCommand('observe 12 42 W'); f.ask();
    assert.equal(f.sent[0].lat, 42); assert.equal(f.sent[0].lon, 12);
    [...f.timers.values()][0]();
    assert.equal(f.elements.get('chatbot-send-btn').disabled, false);
});

test('inline scripts, renderer and controls parse successfully', () => {
    for (const match of html.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/g)) new vm.Script(match[1]);
    for (const file of ['planetarium-controls.js', 'planetarium-webgl.js'])
        new vm.Script(readFileSync(new URL('../../planetarium/' + file, import.meta.url), 'utf8'));
});
