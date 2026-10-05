import { readFileSync } from 'node:fs';
import vm from 'node:vm';

const site = new URL('../../../ilpiccolobardo/', import.meta.url);

export async function contextFor(file) {
    const source = readFileSync(new URL(file, site), 'utf8');
    const elements = new Map(), timers = new Map(), events = new Map(), sent = [], toasts = [];
    let nextTimer = 1;
    function element() {
        const attributes = new Map(), classes = new Set();
        let html = '';
        return {
            textContent: '', value: '', children: [], hidden: false, disabled: false,
            style: { setProperty() {} }, dataset: {},
            get innerHTML() { return html; },
            set innerHTML(value) {
                for (const match of html.matchAll(/\bid="([^"]+)"/g)) elements.delete(match[1]);
                html = value;
                for (const match of html.matchAll(/\bid="([^"]+)"/g)) elements.set(match[1], element());
            },
            appendChild(child) { this.children.push(child); },
            querySelectorAll() { return []; }, addEventListener() {}, focus() {}, scrollIntoView() {},
            setAttribute(name, value) { attributes.set(name, String(value)); },
            getAttribute(name) { return attributes.get(name) ?? null; },
            removeAttribute(name) { attributes.delete(name); if (name === 'src') delete this.src; },
            classList: {
                add(name) { classes.add(name); }, remove(name) { classes.delete(name); },
                contains(name) { return classes.has(name); },
                toggle(name, on = !classes.has(name)) { on ? classes.add(name) : classes.delete(name); },
            },
        };
    }
    for (const match of source.matchAll(/\bid="([^"]+)"/g)) elements.set(match[1], element());
    if (file === 'index.html') {
        elements.get('accountBtn').innerHTML = '👤 <span id="accountName"></span>';
        for (const id of ['readBtn', 'illustrateBtn', 'printBtn']) elements.get(id).disabled = true;
        elements.get('storyCard').style.display = 'none';
        elements.get('nextActions').hidden = true;
    }
    const fallback = element();
    const storage = () => {
        const values = new Map();
        return { getItem: key => values.get(key) ?? null, setItem: (key, value) => values.set(key, String(value)), removeItem: key => values.delete(key) };
    };
    class Socket {
        static CONNECTING = 0; static OPEN = 1; static CLOSING = 2; static CLOSED = 3;
        constructor() { this.readyState = Socket.CONNECTING; Socket.last = this; }
        send(value) { if (this.readyState !== Socket.OPEN) throw new Error('disconnected'); sent.push(JSON.parse(value)); }
        open() { this.readyState = Socket.OPEN; this.onopen?.(); }
        close() { this.readyState = Socket.CLOSED; this.onclose?.(); }
        receive(response) { this.onmessage?.({ data: JSON.stringify(response) }); }
    }
    class TestAudio {
        static instances = [];
        constructor(url) { this.url = url; this.paused = true; this.ended = false; this.listeners = new Map(); TestAudio.instances.push(this); }
        addEventListener(name, fn) { this.listeners.set(name, fn); }
        emit(name) { this.listeners.get(name)?.(); }
        play() { this.paused = false; this.emit('play'); return Promise.resolve(); }
        pause() { this.paused = true; this.emit('pause'); }
        removeAttribute() {} load() {}
    }
    const context = vm.createContext({
        console: { log() {}, info() {}, warn() {}, error() {} },
        window: {
            addEventListener(name, fn) { if (!events.has(name)) events.set(name, []); events.get(name).push(fn); },
            matchMedia: () => ({ matches: false, addEventListener() {} }), scrollTo() {},
            speechSynthesis: { speaking: false, cancel() {}, getVoices: () => [] },
        },
        navigator: { onLine: true, userAgent: 'test' }, location: { protocol: 'http:' },
        localStorage: storage(), sessionStorage: storage(), WebSocket: Socket, Audio: TestAudio, Blob, URL,
        fetch: async () => ({ ok: true, json: async () => ({ session: 'test-session' }) }),
        setTimeout(fn, ms) { const id = nextTimer++; timers.set(id, { fn, ms }); return id; },
        clearTimeout(id) { timers.delete(id); },
        setInterval(fn, ms) { const id = nextTimer++; timers.set(id, { fn, ms }); return id; },
        clearInterval(id) { timers.delete(id); },
        showToast: (...args) => toasts.push(args),
        confirmDialog: async () => true,
        document: {
            body: element(), documentElement: element(), addEventListener() {}, createElement: element,
            querySelector(selector) { return selector === 'dialog[open]' ? null : fallback; },
            querySelectorAll(selector) {
                const ids = selector === '.tab' ? ['tab-create', 'tab-favorites', 'tab-chi_sono']
                    : selector === '.tab-content' ? ['create-tab', 'favorites-tab', 'chi_sono-tab'] : [];
                return ids.map(id => elements.get(id));
            },
            getElementById(id) {
                if (!elements.has(id) && !file.endsWith('.html')) elements.set(id, element());
                return elements.get(id) ?? null;
            },
        },
    });
    vm.runInContext(readFileSync(new URL('safe-html.js', site), 'utf8'), context);
    const modules = new Map();
    function createModule(url) {
        const id = url.href;
        if (!modules.has(id)) {
            modules.set(id, new vm.SourceTextModule(readFileSync(url, 'utf8'), { context, identifier: id }));
        }
        return modules.get(id);
    }
    if (file === 'index.html') {
        vm.runInContext(readFileSync(new URL('js/theme-init.js', site), 'utf8'), context);
        const entryPath = source.match(/<script\s+type="module"\s+src="([^"]+)"/)[1];
        const entry = createModule(new URL(entryPath.replace(/^\//, ''), site));
        await entry.link((specifier, referencing) => createModule(new URL(specifier, referencing.identifier)));
        await entry.evaluate();
        for (const module of modules.values()) Object.assign(context, module.namespace);
    } else {
        const scripts = file.endsWith('.html')
            ? [...source.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/g)].map(m => m[1]).join('\n') : source;
        vm.runInContext(scripts, context);
    }
    return { context, elements, timers, events, sent, toasts, Socket, Audio: TestAudio, modules,
        run: script => vm.runInContext(script, context),
        emit: name => events.get(name)?.forEach(fn => fn()),
    };
}
