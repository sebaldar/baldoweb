/**
 * planetarium-ui.js
 * Interfaccia di contorno: ricerca oggetti, etichette sul cielo, striscia di stato,
 * gesti touch (trascinamento e pinch) e layout iniziale per smartphone.
 * Usa solo comandi già noti al server: `observe <lon> <lat> <corpo>`, `move`, `zoom`
 * e, per stelle e costellazioni, l'assistente (chatbotSendText).
 */
const skyUI = (function () {
    'use strict';

    // ── Catalogo ricerca ──────────────────────────────────────────────────────
    // body: puntamento diretto con `observe`; chat: richiesta all'assistente.
    const BODIES = [
        ['sun', 'Sole'], ['moon', 'Luna'], ['mercury', 'Mercurio'], ['venus', 'Venere'],
        ['mars', 'Marte'], ['jupiter', 'Giove'], ['saturn', 'Saturno'],
        ['uranus', 'Urano'], ['neptune', 'Nettuno'],
    ].map(([id, name]) => ({ name, kind: 'Corpo', body: id }));

    const STARS = ['Sirio', 'Canopo', 'Arturo', 'Vega', 'Capella', 'Rigel', 'Procione', 'Betelgeuse',
        'Altair', 'Aldebaran', 'Antares', 'Spica', 'Polluce', 'Fomalhaut', 'Deneb', 'Regolo', 'Polare']
        .map(name => ({ name, kind: 'Stella', chat: `Punta verso ${name}` }));

    const CONSTELLATIONS = ['Orione', 'Orsa Maggiore', 'Orsa Minore', 'Cassiopea', 'Scorpione', 'Leone',
        'Cigno', 'Lira', 'Aquila', 'Toro', 'Gemelli', 'Cancro', 'Vergine', 'Bilancia', 'Sagittario',
        'Capricorno', 'Acquario', 'Pesci', 'Ariete', 'Pegaso', 'Andromeda', 'Perseo', 'Croce del Sud']
        .map(name => ({ name, kind: 'Costellazione', chat: `Punta verso ${name}` }));

    const CATALOG = [...BODIES, ...STARS, ...CONSTELLATIONS];

    const fold = (text) => String(text).normalize('NFD').replace(/[̀-ͯ]/g, '').toLowerCase();

    function matches(query) {
        const q = fold(query.trim());
        if (!q) return [];
        const starts = [], contains = [];
        for (const item of CATALOG) {
            const n = fold(item.name);
            if (n.startsWith(q)) starts.push(item);
            else if (n.includes(q)) contains.push(item);
        }
        return [...starts, ...contains].slice(0, 8);
    }

    // ── Feedback di puntamento ────────────────────────────────────────────────
    let pointing = null; // { since, last, stable }
    function showPointing(name) {
        const el = document.getElementById('pointing-status');
        if (!el) return;
        el.textContent = `Sto puntando ${name}…`;
        el.hidden = false;
        pointing = { since: performance.now(), last: null, stable: 0 };
    }
    function trackPointing(view) {
        if (!pointing) return;
        const elapsed = performance.now() - pointing.since;
        pointing.stable = view === pointing.last ? pointing.stable + 1 : 0;
        pointing.last = view;
        // fine quando la vista smette di cambiare, comunque entro 8 secondi
        if ((elapsed > 1200 && pointing.stable >= 2) || elapsed > 8000) {
            document.getElementById('pointing-status').hidden = true;
            pointing = null;
        }
    }

    function goTo(item) {
        showPointing(item.name);
        if (item.body) {
            const { lon, lat } = simulation.snapshot();
            sendCommand(`observe ${lon} ${lat} ${item.body}`);
        } else if (typeof chatbotSendText === 'function') {
            chatbotSendText(item.chat);
        }
    }

    function initSearch() {
        const input = document.getElementById('sky-search');
        const list = document.getElementById('sky-search-results');
        if (!input || !list) return;
        let items = [], active = -1;

        function close() {
            list.hidden = true; active = -1;
            input.setAttribute('aria-expanded', 'false');
            input.removeAttribute('aria-activedescendant');
        }
        function highlight(i) {
            active = i;
            [...list.children].forEach((li, idx) => li.setAttribute('aria-selected', String(idx === i)));
            if (i >= 0) input.setAttribute('aria-activedescendant', list.children[i].id);
        }
        function choose(item) {
            input.value = '';
            close();
            input.blur();
            goTo(item);
        }
        function render() {
            items = matches(input.value);
            list.replaceChildren();
            items.forEach((item, i) => {
                const li = document.createElement('li');
                li.id = `sky-result-${i}`;
                li.setAttribute('role', 'option');
                li.setAttribute('aria-selected', 'false');
                li.append(item.name);
                const kind = document.createElement('small');
                kind.textContent = item.kind;
                li.append(kind);
                // pointerdown: scatta prima del blur dell'input
                li.addEventListener('pointerdown', (e) => { e.preventDefault(); choose(item); });
                list.appendChild(li);
            });
            if (!items.length && input.value.trim()) {
                const li = document.createElement('li');
                li.className = 'sky-no-result';
                li.setAttribute('role', 'status');
                li.textContent = 'Nessun risultato';
                list.appendChild(li);
                list.hidden = false;
            } else {
                list.hidden = items.length === 0;
            }
            input.setAttribute('aria-expanded', String(items.length > 0));
            active = -1;
        }

        input.addEventListener('input', render);
        input.addEventListener('blur', () => setTimeout(close, 120));
        input.addEventListener('keydown', (e) => {
            e.stopPropagation(); // evita le scorciatoie da tastiera del planetario
            if (e.key === 'ArrowDown' && items.length) { e.preventDefault(); highlight((active + 1) % items.length); }
            else if (e.key === 'ArrowUp' && items.length) { e.preventDefault(); highlight((active - 1 + items.length) % items.length); }
            else if (e.key === 'Enter' && items.length) { e.preventDefault(); choose(items[Math.max(active, 0)]); }
            else if (e.key === 'Escape') { input.value = ''; close(); input.blur(); }
        });
    }

    // ── Etichette sul cielo ───────────────────────────────────────────────────
    // 0 = spente, 1 = solo Sole/Luna/pianeti, 2 = tutti gli oggetti con nome
    const LABEL_NAMES = Object.fromEntries(BODIES.map(b => [b.body, b.name]));
    const NOT_LABELLED = new Set(['sky', 'eclittica', 'ecliptic', 'equatore', 'equator',
        'hor_circle', 'hor_line', 'hor_nord', 'grid', 'axis', 'sunlight']);
    const LEVEL_TEXT = ['spente', 'corpi del sistema solare', 'tutti gli oggetti'];
    const LEVEL_BADGE = ['Off', 'Corpi', 'Tutti'];
    let labelLevel = 0;
    const labelEls = new Map();
    const _pos = typeof THREE !== 'undefined' ? new THREE.Vector3() : null;

    function setLevel(level) {
        labelLevel = level;
        const btn = document.getElementById('toggle-labels');
        if (btn) {
            btn.setAttribute('aria-pressed', String(level > 0));
            btn.dataset.level = String(level);
            btn.title = `Etichette: ${LEVEL_TEXT[level]} (T)`;
            btn.setAttribute('aria-label', `Etichette sul cielo: ${LEVEL_TEXT[level]}`);
            const badge = btn.querySelector('.label-state');
            if (badge) badge.textContent = LEVEL_BADGE[level];
        }
        if (level === 0) { document.getElementById('sky-labels').replaceChildren(); labelEls.clear(); }
        if (typeof logToPage === 'function') logToPage(`🏷️ Etichette: ${LEVEL_TEXT[level]}`, 'info');
    }

    function updateLabels() {
        if (!labelLevel || typeof webgl === 'undefined' || !webgl.camera || !webgl.renderer) return;
        const layer = document.getElementById('sky-labels');
        const w = webgl.renderer.domElement.clientWidth, h = webgl.renderer.domElement.clientHeight;
        const seen = new Set();

        for (const obj of webgl.click_objects || []) {
            const id = obj.name;
            if (!id || !obj.visible || NOT_LABELLED.has(id)) continue;
            if (labelLevel === 1 && !LABEL_NAMES[id]) continue;
            obj.getWorldPosition(_pos);
            _pos.project(webgl.camera);
            if (_pos.z > 1 || Math.abs(_pos.x) > 1 || Math.abs(_pos.y) > 1) continue;

            let el = labelEls.get(id);
            if (!el) {
                el = document.createElement('div');
                el.className = 'sky-label';
                el.textContent = LABEL_NAMES[id] || id;
                layer.appendChild(el);
                labelEls.set(id, el);
            }
            el.style.left = ((_pos.x + 1) / 2 * w) + 'px';
            el.style.top = ((1 - _pos.y) / 2 * h) + 'px';
            seen.add(id);
        }
        for (const [id, el] of labelEls) {
            if (!seen.has(id)) { el.remove(); labelEls.delete(id); }
        }
    }

    function cycleLabels() { setLevel((labelLevel + 1) % 3); }

    // ── Striscia di stato ─────────────────────────────────────────────────────
    function updateStrip() {
        const text = (id) => document.getElementById(id)?.textContent?.trim() || '--';
        const val = (id) => document.getElementById(id)?.value || '';
        const date = val('sim-date'), time = val('sim-time');
        const [y, m, d] = date.split('-');
        document.getElementById('strip-datetime').textContent =
            date && time ? `${d}/${m}/${y} ${time} UT` : '--';
        document.getElementById('strip-place').textContent = `${text('latitude')} ${text('longitude')}`.replace(/--\s--/, '--');
        const view = `Az ${text('azimut')} · Alt ${text('height')}`;
        document.getElementById('strip-view').textContent = view;
        trackPointing(view);
    }

    // ── Gesti touch: un dito ruota la vista, due dita fanno zoom ──────────────
    function initTouch() {
        const container = document.getElementById('canvas-container');
        if (!container) return;
        const pointers = new Map();
        let lastDist = 0, accAz = 0, accAlt = 0, lastSend = 0;
        const DEG_PER_PX = 0.2, MIN_INTERVAL = 80;

        const blocked = () => typeof controls !== 'undefined' && (controls.orbitEnabled || controls.commandFocused);
        const dist = () => { const [a, b] = [...pointers.values()]; return Math.hypot(a.x - b.x, a.y - b.y); };

        container.addEventListener('pointerdown', (e) => {
            if (e.pointerType !== 'touch') return;
            pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
            if (pointers.size === 2) lastDist = dist();
        });

        container.addEventListener('pointermove', (e) => {
            if (e.pointerType !== 'touch' || !pointers.has(e.pointerId) || blocked()) return;
            const prev = pointers.get(e.pointerId);
            const dx = e.clientX - prev.x, dy = e.clientY - prev.y;
            pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });

            if (pointers.size === 2) {
                const d = dist();
                if (lastDist && d / lastDist > 1.15) { sendCommand('zoom +'); lastDist = d; }
                else if (lastDist && d / lastDist < 0.87) { sendCommand('zoom -'); lastDist = d; }
                return;
            }
            // "afferra il cielo": trascinare a destra riduce l'azimut, verso il basso alza lo sguardo
            accAz -= dx * DEG_PER_PX;
            accAlt += dy * DEG_PER_PX;
            const now = performance.now();
            if (now - lastSend < MIN_INTERVAL) return;
            const az = Math.trunc(accAz), alt = Math.trunc(accAlt);
            if (!az && !alt) return;
            accAz -= az; accAlt -= alt;
            lastSend = now;
            sendCommand(`move ${alt} ${az}`);
        });

        const release = (e) => {
            if (e.pointerType !== 'touch') return;
            pointers.delete(e.pointerId);
            if (pointers.size < 2) lastDist = 0;
            if (pointers.size === 0) { accAz = accAlt = 0; }
        };
        container.addEventListener('pointerup', release);
        container.addEventListener('pointercancel', release);
    }

    // ── Finestre modali: focus, Esc, sfondo inerte ────────────────────────────
    // Osserva display di #location-dialog e #help-modal, così funziona con le
    // funzioni di apertura/chiusura esistenti senza riscriverle.
    function initModals() {
        const closers = {
            'location-dialog': () => typeof closeLocationDialog === 'function' && closeLocationDialog(),
            'help-modal': () => typeof toggleHelpModal === 'function' && toggleHelpModal(),
        };
        const FOCUSABLE = 'a[href],button:not([disabled]),input:not([disabled]),select:not([disabled]),textarea:not([disabled]),[tabindex]:not([tabindex="-1"])';
        const stack = []; // { el, opener }
        const isOpen = (el) => el.style.display !== 'none' && getComputedStyle(el).display !== 'none';

        function refreshInert() {
            const top = stack[stack.length - 1]?.el;
            for (const child of document.body.children) {
                if (['SCRIPT', 'STYLE', 'LINK'].includes(child.tagName)) continue;
                child.inert = !!top && child !== top;
            }
        }
        function onOpen(el) {
            if (stack.some(m => m.el === el)) return;
            stack.push({ el, opener: document.activeElement });
            refreshInert();
            (el.querySelector('[autofocus]') || el.querySelector(FOCUSABLE) || el).focus?.();
        }
        function onClose(el) {
            const i = stack.findIndex(m => m.el === el);
            if (i < 0) return;
            const [{ opener }] = stack.splice(i, 1);
            refreshInert();
            if (opener && document.contains(opener)) opener.focus?.();
        }

        Object.keys(closers).forEach((id) => {
            const el = document.getElementById(id);
            if (!el) return;
            el.tabIndex = -1;
            const sync = () => (isOpen(el) ? onOpen(el) : onClose(el));
            new MutationObserver(sync).observe(el, { attributes: true, attributeFilter: ['style', 'class'] });
            sync();
        });

        document.addEventListener('keydown', (e) => {
            const top = stack[stack.length - 1];
            if (!top) return;
            if (e.key === 'Escape') {
                e.preventDefault(); e.stopPropagation();
                closers[top.el.id]?.();
            } else if (e.key === 'Tab') {
                const items = [...top.el.querySelectorAll(FOCUSABLE)].filter(n => n.offsetParent !== null);
                if (!items.length) { e.preventDefault(); return; }
                const first = items[0], last = items[items.length - 1];
                if (e.shiftKey && (document.activeElement === first || !top.el.contains(document.activeElement))) { e.preventDefault(); last.focus(); }
                else if (!e.shiftKey && (document.activeElement === last || !top.el.contains(document.activeElement))) { e.preventDefault(); first.focus(); }
            }
        }, true);
    }

    // ── Avvio ─────────────────────────────────────────────────────────────────
    function init() {
        initSearch();
        initTouch();
        initModals();
        document.getElementById('toggle-labels')?.addEventListener('click', cycleLabels);
        document.getElementById('toggle-console')?.addEventListener('click', () => {
            const open = document.getElementById('console-log').classList.contains('visible');
            document.getElementById('toggle-console').classList.toggle('active', open);
        });
        document.getElementById('toggle-help')?.addEventListener('click', () => {
            const open = document.getElementById('keyboard-help').style.display === 'block';
            document.getElementById('toggle-help').classList.toggle('active', open);
        });

        // Su smartphone il pannello dati parte chiuso per lasciare libero il cielo
        if (window.matchMedia('(max-width: 768px)').matches) {
            document.getElementById('info-panel-content')?.classList.add('collapsed');
            const icon = document.getElementById('toggle-info-icon');
            icon?.classList.add('collapsed');
            if (icon) icon.textContent = '▶';
        }

        updateStrip();
        setInterval(updateStrip, 500);
        (function frame() { updateLabels(); requestAnimationFrame(frame); })();
    }

    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
    else init();

    return { cycleLabels, matches };
})();
window.skyUI = skyUI;
