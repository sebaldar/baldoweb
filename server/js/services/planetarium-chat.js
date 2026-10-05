import { computeSnapshot, simulationConfig } from './planetarium-snapshot.js';

export const CHAT_TIMEOUT_MS = 120_000;
const send = (ws, event) => {
    if (ws.readyState === ws.OPEN) ws.send(JSON.stringify(event));
};

export async function streamChat(ws, payload, { fetchImpl = fetch, timeoutMs = CHAT_TIMEOUT_MS } = {}) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(new Error('Tempo di risposta AI scaduto')), timeoutMs);
    const disconnect = () => controller.abort(new Error('Connessione chiusa'));
    ws.once('close', disconnect);
    let reader;
    let completed = false;
    try {
        const response = await fetchImpl('http://baldo-fastapi-planetarium:8000/api/chat', {
            method: 'POST', headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload), signal: controller.signal,
        });
        if (!response.ok || !response.body) throw new Error(`Backend AI HTTP ${response.status}`);
        reader = response.body.getReader();
        const decoder = new TextDecoder();
        let buffer = '';
        function processLine(raw) {
            const line = raw.trim().replace(/^data:\s*/, '');
            if (!line || line === '[DONE]' || line.startsWith(':') || /^(event|id|retry):/.test(line)) return;
            const event = JSON.parse(line);
            if (!['thinking', 'cmd', 'rag_result', 'weather', 'token', 'final', 'error'].includes(event.tipo)) return;
            if (completed) return;
            if (event.tipo === 'final' || event.tipo === 'error') completed = true;
            send(ws, { ...event, action: 'chat_result', request_id: payload.request_id });
        }
        while (true) {
            const { done, value } = await readChunk(reader, controller.signal);
            if (done) break;
            buffer += decoder.decode(value, { stream: true });
            if (buffer.length > 1_000_000) throw new Error('Evento AI troppo grande');
            const lines = buffer.split('\n');
            buffer = lines.pop() || '';
            lines.forEach(processLine);
            if (completed) break;
        }
        buffer += decoder.decode();
        if (buffer.trim() && !completed) processLine(buffer);
        if (!completed) throw new Error('Risposta AI interrotta');
    } finally {
        clearTimeout(timeout);
        ws.off('close', disconnect);
        if (reader) {
            await reader.cancel().catch(() => {});
            reader.releaseLock();
        }
    }
}

function readChunk(reader, signal) {
    if (signal.aborted) return Promise.reject(signal.reason);
    return new Promise((resolve, reject) => {
        const abort = () => reject(signal.reason);
        signal.addEventListener('abort', abort, { once: true });
        reader.read().then(resolve, reject).finally(() => signal.removeEventListener('abort', abort));
    });
}

export async function handleChat(solar, ws, message, options) {
    const requestId = typeof message.request_id === 'string' ? message.request_id.slice(0, 80) : undefined;
    if (ws.planetariumChatPending) {
        send(ws, { action: 'chat_result', tipo: 'error', request_id: requestId,
            text: 'Attendi la risposta precedente prima di inviare un’altra domanda.' });
        return;
    }
    ws.planetariumChatPending = true;
    try {
        if (!ws.clientData?.sessionId) throw new Error('Sessione assente');
        if (typeof message.text !== 'string' || !message.text.trim() || message.text.length > 2000)
            throw new Error('La domanda deve contenere da 1 a 2000 caratteri');
        const provider = message.provider || 'ionos';
        if (!['ionos', 'anthropic', 'openai', 'gemini'].includes(provider)) throw new Error('Provider non valido');
        const config = simulationConfig(message);
        const raw = computeSnapshot(solar, config);
        const payload = { prompt: message.text.trim(), provider, data: config.utc,
            session_id: ws.clientData.sessionId, request_id: requestId,
            lat: config.latitude, lon: config.longitude,
            motore_astronomico: raw.replace(/-?nan\b/g, 'null') };
        await streamChat(ws, payload, options);
    } catch (error) {
        console.error('[PLANETARIUM] Chat:', error.message);
        send(ws, { action: 'chat_result', tipo: 'error', request_id: requestId,
            text: 'Non è stato possibile completare la risposta. Riprova tra un momento.' });
    } finally {
        ws.planetariumChatPending = false;
    }
}
