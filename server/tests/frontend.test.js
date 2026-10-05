import test from 'node:test';
import assert from 'node:assert/strict';
import { contextFor } from './helpers/browser-context.js';
import { readFileSync } from 'node:fs';

function fillForm(browser) {
    browser.run("appState.selectedAge = 4; appState.chosen.genres = ['amicizia']; appState.chosen.settingKeys = ['bosco'];");
    for (const [id, value] of Object.entries({ userPrompt: 'Un drago cerca un amico', childName: 'Ada', favoriteAnimal: 'volpe', favoriteColor: 'blu' })) {
        browser.elements.get(id).value = value;
    }
}

async function readyBrowser() {
    const browser = await contextFor('index.html');
    await browser.context.connect();
    browser.Socket.last.open();
    fillForm(browser);
    return browser;
}

function assertFinished(browser) {
    assert.equal(browser.run('appState.isGenerating'), false);
    assert.equal(browser.elements.get('generateBtn').disabled, false);
    assert.equal([...browser.timers.values()].some(timer => timer.ms === 240000 || timer.ms === 45000), false);
}

test('account updates retain the name element across guest, user and logout states', async () => {
    const { context, elements } = await contextFor('index.html');
    const span = elements.get('accountName');
    context.updateAccountButton();
    context.handleResponse({ action: 'user_status', data: { user: { name: 'Ada', email: 'ada@example.test' } } });
    assert.equal(elements.get('accountName'), span);
    assert.equal(span.textContent, 'A');
    assert.equal(span.style.display, 'inline');
    context.handleResponse({ action: 'user_status', data: { user: null } });
    assert.equal(span.style.display, 'none');
    context.handleResponse({ action: 'user_status', data: { user: { email: 'leo@example.test' } } });
    assert.equal(span.textContent, 'L');
    assert.equal(elements.get('accountBtn').getAttribute('aria-label'), 'Account: leo@example.test');
});

test('offline or disconnected creation preserves the current story without showing a spinner', async () => {
    const browser = await contextFor('index.html');
    fillForm(browser);
    browser.run("appState.currentStory = { text: 'Una favola già letta', age: 4, prompt: 'Una volpe' };");
    browser.context.displayStory('Una favola già letta');
    const html = browser.elements.get('storyDisplay').innerHTML;
    for (const online of [false, true]) {
        browser.context.navigator.onLine = online;
        browser.context.generateStory();
        assert.equal(browser.elements.get('storyDisplay').innerHTML, html);
        assert.equal(browser.run('appState.currentStory.text'), 'Una favola già letta');
        assert.equal(browser.sent.length, 0);
        assertFinished(browser);
    }
    assert.equal(browser.toasts.length, 2);
});

test('only one story request can run and partial output cannot be saved or printed', async () => {
    const browser = await readyBrowser();
    browser.context.generateStory();
    browser.context.generateStory();
    browser.context.regenerateStory();
    browser.context.newStory();
    browser.context.loadFavorite(0);
    assert.equal(browser.sent.length, 1);
    assert.equal(browser.run('appState.isGenerating'), true);
    for (const id of ['generateBtn', 'regenerateBtn', 'newStoryBtn', 'readBtn', 'printBtn']) assert.equal(browser.elements.get(id).disabled, true);
    browser.context.handleResponse({ tipo: 'token', testo: 'Una bozza...' });
    browser.context.toggleFavorite();
    assert.equal(browser.run('appState.favorites.length'), 0);
    browser.context.handleResponse({ tipo: 'fine', racconto: 'Una favola completa.', storia_id: 'complete' });
    assertFinished(browser);
    assert.equal(browser.elements.get('printBtn').disabled, false);
    assert.equal(browser.elements.get('readBtn').disabled, false);
    assert.equal(browser.run('appState.currentStory.text'), 'Una favola completa.');
    browser.context.handleResponse({ tipo: 'token', testo: 'Un vecchio evento' });
    assert.equal(browser.run('appState.currentStory.text'), 'Una favola completa.');
});

test('failed sends stop loading and expose a retry action', async () => {
    const browser = await readyBrowser();
    browser.Socket.last.send = () => { throw new Error('network failure'); };
    browser.context.generateStory();
    assertFinished(browser);
    assert.match(browser.elements.get('storyDisplay').innerHTML, /Riprova/);
    assert.equal(browser.elements.get('printBtn').disabled, true);
    assert.equal(browser.sent.length, 0);
});

test('overlapping reconnect triggers keep one connection and do not replace an active stream', async () => {
    const browser = await contextFor('index.html');
    let releaseSession, calls = 0;
    browser.context.fetch = () => {
        calls++;
        return new Promise(resolve => { releaseSession = resolve; });
    };
    const first = browser.context.connect();
    const second = browser.context.connect();
    browser.emit('online');
    assert.equal(calls, 1);
    releaseSession({ ok: true, json: async () => ({ session: 'test-session' }) });
    await Promise.all([first, second]);
    const socket = browser.Socket.last;
    await browser.context.connect();
    assert.equal(browser.Socket.last, socket);
    socket.open();
    fillForm(browser);
    browser.context.generateStory();
    browser.emit('online');
    await browser.context.connect();
    assert.equal(browser.Socket.last, socket);
    assert.equal(calls, 1);
    socket.receive({ tipo: 'fine', racconto: 'Una favola' });
    assertFinished(browser);
});

for (const response of [
    { tipo: 'errore', messaggio: '<img src=x onerror=alert(1)>' },
    { action: 'error', message: 'Servizio non disponibile' },
    { tipo: 'fine', errore: 'rifiutato', motivo_rifiuto: 'Richiesta non adatta' },
    { tipo: 'fine', racconto: '' },
]) {
    test(`generation recovers from ${response.tipo || response.action}: ${response.errore || response.messaggio || response.message || 'empty story'}`, async () => {
        const browser = await readyBrowser();
        browser.context.generateStory();
        browser.context.handleResponse({ tipo: 'token', testo: 'Testo parziale' });
        browser.context.handleResponse(response);
        assertFinished(browser);
        assert.equal(browser.run('appState.currentStory.text'), '');
        assert.equal(browser.elements.get('nextActions').hidden, true);
        for (const id of ['readBtn', 'illustrateBtn', 'printBtn']) assert.equal(browser.elements.get(id).disabled, true);
        const html = browser.elements.get('storyDisplay').innerHTML;
        assert.ok(!html.includes('<img'));
        assert.equal(html.includes('onclick="regenerateStory()"'), !response.errore);
    });
}

test('socket interruption permits retry and ignores messages from the previous connection', async () => {
    const browser = await readyBrowser();
    browser.context.generateStory();
    const original = browser.sent[0].data;
    const oldSocket = browser.Socket.last;
    oldSocket.close();
    assertFinished(browser);
    assert.match(browser.elements.get('storyDisplay').innerHTML, /connessione si è interrotta/);
    await browser.context.connect();
    const newSocket = browser.Socket.last;
    newSocket.open();
    browser.elements.get('userPrompt').value = 'Un altro modulo';
    browser.context.regenerateStory();
    assert.deepEqual(browser.sent[1].data, original);
    oldSocket.receive({ tipo: 'fine', racconto: 'Risposta obsoleta' });
    assert.equal(browser.run('appState.isGenerating'), true);
    assert.equal(browser.run('appState.currentStory.text'), '');
    newSocket.receive({ tipo: 'fine', racconto: 'La nuova favola' });
    assertFinished(browser);
    assert.equal(browser.run('appState.currentStory.text'), 'La nuova favola');
});

test('going offline during creation stops loading and allows retry', async () => {
    const browser = await readyBrowser();
    browser.context.generateStory();
    browser.context.navigator.onLine = false;
    browser.emit('offline');
    assertFinished(browser);
    assert.equal(browser.elements.get('offlineBanner').hidden, false);
    assert.match(browser.elements.get('storyDisplay').innerHTML, /Sei offline/);
    assert.equal(browser.Socket.last.readyState, browser.Socket.CLOSED);
});

test('an inactive stream times out, while received events renew the wait', async () => {
    const browser = await readyBrowser();
    browser.context.generateStory();
    const initialTimer = [...browser.timers].find(([, timer]) => timer.ms === 240000)[0];
    browser.context.handleResponse({ tipo: 'nodo_end', nodo: 'analizza_prompt' });
    assert.equal(browser.timers.has(initialTimer), false);
    const [, timer] = [...browser.timers].find(([, entry]) => entry.ms === 240000);
    timer.fn();
    assertFinished(browser);
    assert.match(browser.elements.get('storyDisplay').innerHTML, /Non ricevo più aggiornamenti/);
    assert.equal(browser.Socket.last.readyState, browser.Socket.CLOSED);
});

test('opening favorites enables PDF and replaces or clears the previous illustration', async () => {
    const browser = await contextFor('index.html');
    browser.run(`appState.favorites = [
        { text: 'Una volpe', age: 4, prompt: 'Una volpe', timestamp: '2026-01-01', illustration: '/fox.png' },
        { text: 'Un drago', age: 5, prompt: 'Un drago', timestamp: '2026-01-02' }
    ];`);
    browser.context.loadFavorite(0);
    assert.equal(browser.elements.get('printBtn').disabled, false);
    assert.equal(browser.elements.get('storyIllustration').src, '/fox.png');
    assert.equal(browser.elements.get('storyIllustration').style.display, 'block');
    browser.context.loadFavorite(1);
    assert.equal(browser.elements.get('printBtn').disabled, false);
    assert.equal(browser.elements.get('storyIllustration').style.display, 'none');
    assert.equal(browser.elements.get('storyIllustration').src, undefined);
    assert.match(browser.elements.get('storyDisplay').innerHTML, /Un drago/);
});

test('another similar story uses the opened favorite and leaves its saved contents intact', async () => {
    const browser = await readyBrowser();
    browser.run(`appState.favorites = [{ text: 'Una volpe nel castello', prompt: 'Una volpe nel castello', age: 6,
        childName: 'Leo', favoriteAnimal: 'volpe', favoriteColor: 'rosso', genre: 'avventura', settingKey: 'castello',
        timestamp: '2026-01-01', title: 'La volpe', illustration: '/fox.png' }];`);
    browser.context.loadFavorite(0);
    browser.context.regenerateStory();
    assert.equal(browser.sent[0].data.userPrompt, 'Una volpe nel castello');
    assert.equal(browser.sent[0].data.age, 6);
    assert.equal(browser.sent[0].data.nome, 'Leo');
    assert.equal(browser.sent[0].data.genere, 'avventura');
    assert.equal(browser.sent[0].data.ambientazione, 'castello');
    browser.context.handleResponse({ tipo: 'fine', racconto: 'La volpe trova un amico' });
    assert.equal(browser.run('appState.favorites[0].text'), 'Una volpe nel castello');
    assert.equal(browser.run('appState.favorites[0].illustration'), '/fox.png');
    assert.equal(browser.run('appState.currentStory.title'), undefined);
    assert.equal(browser.run('appState.currentStory.illustration'), undefined);
    assert.equal(browser.elements.get('storyIllustration').style.display, 'none');
});

test('legacy favorites use their saved prompt and disable regeneration when the prompt is absent', async () => {
    const browser = await readyBrowser();
    browser.run("appState.favorites = [{ text: 'Una favola', prompt: 'Un drago rosso', age: 3, timestamp: '2026-01-01' }];");
    browser.context.loadFavorite(0);
    browser.context.regenerateStory();
    assert.equal(browser.sent[0].data.userPrompt, 'Un drago rosso');
    assert.equal(browser.sent[0].data.age, 3);
    browser.context.handleResponse({ tipo: 'fine', racconto: 'Un drago nuovo' });
    browser.run("appState.favorites.push({ text: 'Solo testo', timestamp: '2026-01-02' });");
    browser.context.loadFavorite(1);
    assert.equal(browser.elements.get('regenerateBtn').disabled, true);
    assert.equal(browser.elements.get('printBtn').disabled, false);
    browser.context.regenerateStory();
    assert.equal(browser.sent.length, 1);
});

test('imported favorites retain generation options for another similar story', async () => {
    const browser = await readyBrowser();
    const story = { text: 'Una favola', prompt: 'Una volpe', timestamp: '2026-01-01', age: 5,
        favoriteAnimal: 'volpe', favoriteColor: 'blu', genre: 'amicizia', settingKey: 'bosco' };
    await browser.context.importFavorites({ value: 'stories.json', files: [{ size: 100, text: async () => JSON.stringify([story]) }] });
    browser.context.loadFavorite(0);
    browser.context.regenerateStory();
    for (const [field, value] of Object.entries({ userPrompt: 'Una volpe', age: 5, animaleP: 'volpe', coloreP: 'blu', genere: 'amicizia', ambientazione: 'bosco' })) {
        assert.equal(browser.sent[0].data[field], value);
    }
});

function showSavedStory(browser) {
    browser.run("appState.currentStory = { text: 'La volpe trova un amico', prompt: 'Una volpe', age: 4, timestamp: '2026-01-01', title: 'La volpe' };");
    browser.context.displayStory('La volpe trova un amico');
}

test('failed favorite writes preserve both the saved archive and the heart state', async () => {
    const browser = await readyBrowser();
    showSavedStory(browser);
    browser.context.localStorage.setItem('bardo_favorites', '[]');
    browser.context.localStorage.setItem = () => { throw new Error('QuotaExceededError'); };
    browser.context.toggleFavorite();
    assert.equal(browser.run('appState.favorites.length'), 0);
    assert.equal(browser.elements.get('favoriteBtn').getAttribute('aria-pressed'), 'false');
    assert.equal(browser.context.localStorage.getItem('bardo_favorites'), '[]');
    assert.match(browser.toasts.at(-1)[0], /Esporta/);
    assert.equal(browser.sent.length, 0);
});

test('failed deletion or import does not mutate the favorites list or announce success', async () => {
    const browser = await readyBrowser();
    showSavedStory(browser);
    browser.context.toggleFavorite();
    const archive = browser.context.localStorage.getItem('bardo_favorites');
    browser.context.localStorage.setItem = () => { throw new Error('Storage blocked'); };
    await browser.context.deleteFavorite(0);
    assert.equal(browser.run('appState.favorites.length'), 1);
    assert.equal(browser.elements.get('favoriteBtn').getAttribute('aria-pressed'), 'true');
    await browser.context.importFavorites({ value: '', files: [{ size: 100, text: async () => JSON.stringify([{ text: 'Una seconda favola', timestamp: '2026-01-02' }]) }] });
    assert.equal(browser.run('appState.favorites.length'), 1);
    assert.equal(browser.context.localStorage.getItem('bardo_favorites'), archive);
    assert.equal(browser.toasts.some(([, type]) => type === 'success'), false);
});

test('an unreadable archive is preserved until the user imports a valid backup', async () => {
    const browser = await readyBrowser();
    browser.context.localStorage.setItem('bardo_favorites', '[broken');
    browser.context.loadFavorites();
    showSavedStory(browser);
    browser.context.toggleFavorite();
    assert.equal(browser.context.localStorage.getItem('bardo_favorites'), '[broken');
    assert.equal(browser.run('appState.favorites.length'), 0);
    await browser.context.importFavorites({ value: '', files: [{ size: 100, text: async () => JSON.stringify([{ text: 'Il backup', timestamp: '2026-01-01' }]) }] });
    assert.equal(browser.run('appState.favoritesReadable'), true);
    assert.equal(browser.run('appState.favorites[0].text'), 'Il backup');
    assert.equal(JSON.parse(browser.context.localStorage.getItem('bardo_favorites'))[0].text, 'Il backup');
});

test('blocked storage does not claim a saved profile and does not prevent generation', async () => {
    const browser = await readyBrowser();
    browser.context.localStorage.setItem = () => { throw new Error('Storage blocked'); };
    browser.context.sessionStorage.getItem = () => { throw new Error('Storage blocked'); };
    browser.context.generateStory();
    assert.equal(browser.sent[0].action, 'generate_story');
    assert.equal(browser.sent[0].data.session_id, null);
    assert.equal(browser.elements.get('profileHint').hidden, true);
    assert.equal(browser.run('appState.isGenerating'), true);
});

test('audio preparation sends one correlated request and clears loading before playback', async () => {
    const browser = await readyBrowser();
    showSavedStory(browser);
    browser.context.listenStory();
    browser.context.listenStory();
    assert.equal(browser.sent.length, 1);
    assert.equal(browser.sent[0].action, 'synthesize_speech');
    assert.equal(browser.elements.get('audioLoading').style.display, 'block');
    assert.equal(browser.elements.get('readBtn').disabled, true);
    browser.context.handleResponse({ action: 'speech_generated', requestId: browser.sent[0].data.requestId, audioUrl: '/voice.mp3' });
    assert.equal(browser.run('appState.pendingSpeech'), null);
    assert.equal(browser.elements.get('audioLoading').style.display, 'none');
    assert.equal(browser.Audio.instances.length, 1);
    assert.equal(browser.Audio.instances[0].url, '/voice.mp3');
    assert.equal(browser.elements.get('pauseBtn').style.display, 'block');
    assert.equal([...browser.timers.values()].some(timer => timer.ms === 120000), false);
});

test('offline, disconnected and failed-send audio requests do not leave a loading indicator', async () => {
    const browser = await readyBrowser();
    showSavedStory(browser);
    browser.context.navigator.onLine = false;
    browser.context.listenStory();
    browser.context.navigator.onLine = true;
    browser.Socket.last.close();
    browser.context.listenStory();
    await browser.context.connect();
    browser.Socket.last.open();
    browser.Socket.last.send = () => { throw new Error('disconnected'); };
    browser.context.listenStory();
    assert.equal(browser.run('appState.pendingSpeech'), null);
    assert.notEqual(browser.elements.get('audioLoading').style.display, 'block');
    assert.equal(browser.elements.get('readBtn').disabled, false);
    assert.equal(browser.sent.length, 0);
});

test('cancelling or timing out audio ignores its late response even after a new request', async () => {
    const browser = await readyBrowser();
    showSavedStory(browser);
    browser.context.listenStory();
    const firstId = browser.sent[0].data.requestId;
    browser.context.stopReading();
    browser.context.listenStory();
    const secondId = browser.sent[1].data.requestId;
    browser.context.handleResponse({ action: 'speech_generated', requestId: firstId, audioUrl: '/old.mp3' });
    assert.equal(browser.Audio.instances.length, 0);
    assert.equal(browser.run('appState.pendingSpeech.id'), secondId);
    const [, timer] = [...browser.timers].find(([, entry]) => entry.ms === 120000);
    timer.fn();
    assert.equal(browser.run('appState.pendingSpeech'), null);
    assert.equal(browser.elements.get('readBtn').disabled, false);
    browser.context.handleResponse({ action: 'speech_generated', requestId: secondId, audioUrl: '/late.mp3' });
    assert.equal(browser.Audio.instances.length, 0);
});

test('switching stories discards pending audio and correlated audio errors do not abort a new story', async () => {
    const browser = await readyBrowser();
    showSavedStory(browser);
    browser.context.listenStory();
    const requestId = browser.sent[0].data.requestId;
    browser.context.generateStory();
    browser.context.handleResponse({ action: 'error', requestAction: 'synthesize_speech', requestId, message: 'Old audio failed' });
    browser.context.handleResponse({ action: 'speech_generated', requestId, audioUrl: '/old.mp3' });
    assert.equal(browser.run('appState.isGenerating'), true);
    assert.equal(browser.Audio.instances.length, 0);
});

for (const failure of ['offline', 'disconnect', 'error', 'empty response']) {
    test(`audio controls recover from ${failure}`, async () => {
        const browser = await readyBrowser();
        showSavedStory(browser);
        browser.context.listenStory();
        const requestId = browser.sent[0].data.requestId;
        if (failure === 'offline') { browser.context.navigator.onLine = false; browser.emit('offline'); }
        else if (failure === 'disconnect') browser.Socket.last.close();
        else if (failure === 'error') browser.context.handleResponse({ action: 'error', requestAction: 'synthesize_speech', requestId, message: 'Voice unavailable' });
        else browser.context.handleResponse({ action: 'speech_generated', requestId, audioUrl: '' });
        assert.equal(browser.run('appState.pendingSpeech'), null);
        assert.equal(browser.elements.get('audioLoading').style.display, 'none');
        assert.equal(browser.elements.get('readBtn').disabled, false);
        assert.equal(browser.elements.get('stopBtn').style.display, 'none');
        assert.equal(browser.Audio.instances.length, 0);
    });
}

test('every native module and the stylesheet are included in the offline shell', async () => {
    const browser = await contextFor('index.html');
    const serviceWorker = readFileSync(new URL('../../ilpiccolobardo/sw.js', import.meta.url), 'utf8');
    for (const id of browser.modules.keys()) assert.ok(serviceWorker.includes(`'${new URL(id).pathname.split('/ilpiccolobardo')[1]}'`), id);
    assert.ok(serviceWorker.includes("'/js/theme-init.js'"));
    assert.ok(serviceWorker.includes("'/styles/app.css'"));
    for (const name of ['generateStory', 'toggleFavorite', 'listenStory', 'printStory', 'openAccountModal']) assert.equal(typeof browser.context.window[name], 'function');
});

function chip(browser, group, key) {
    const button = browser.context.document.createElement('button');
    button.dataset[group === 'genre' ? 'value' : 'key'] = key;
    button.setAttribute('aria-pressed', 'false');
    return button;
}

test('themes and settings can be selected independently and deselected one at a time', async () => {
    const browser = await contextFor('index.html');
    const friendship = chip(browser, 'genre', 'amicizia');
    const adventure = chip(browser, 'genre', 'avventura');
    const forest = chip(browser, 'setting', 'bosco');
    const castle = chip(browser, 'setting', 'castello');
    for (const [group, button] of [['genre', friendship], ['genre', adventure], ['setting', forest], ['setting', castle]]) {
        browser.context.pickChip(group, button);
    }
    assert.equal(friendship.getAttribute('aria-pressed'), 'true');
    assert.equal(adventure.getAttribute('aria-pressed'), 'true');
    assert.equal(forest.getAttribute('aria-pressed'), 'true');
    assert.equal(castle.getAttribute('aria-pressed'), 'true');
    browser.context.pickChip('genre', friendship);
    browser.context.pickChip('setting', forest);
    assert.equal(friendship.getAttribute('aria-pressed'), 'false');
    assert.equal(forest.getAttribute('aria-pressed'), 'false');
    assert.equal(adventure.getAttribute('aria-pressed'), 'true');
    assert.equal(castle.getAttribute('aria-pressed'), 'true');
    assert.equal(browser.run('appState.chosen.genres.join()'), 'avventura');
    assert.equal(browser.run('appState.chosen.settingKeys.join()'), 'castello');
});

test('the prompt combines every selected theme and setting with a calm bedtime rhythm', async () => {
    const browser = await contextFor('index.html');
    for (const key of ['amicizia', 'avventura', 'coraggio', 'nanna', 'magia']) browser.context.pickChip('genre', chip(browser, 'genre', key));
    for (const key of ['bosco', 'castello', 'cielo']) browser.context.pickChip('setting', chip(browser, 'setting', key));
    const prompt = browser.context.buildPrompt('La volpe incontra un drago.');
    assert.match(prompt, /^La volpe incontra un drago\./);
    for (const text of ['amicizia', 'avventura', 'coraggio', 'magia e sogni', 'bosco magico', 'castello antico', 'cielo stellato']) assert.ok(prompt.includes(text), text);
    assert.match(prompt, /ambientazione coerente/);
    assert.match(prompt, /ritmo dolce e rilassante, adatto ad addormentarsi/);
});

test('an empty form needs a choice while themes or settings alone can start a prompt', async () => {
    const browser = await contextFor('index.html');
    assert.equal(browser.context.buildPrompt('  '), '');
    const friendship = chip(browser, 'genre', 'amicizia');
    browser.context.pickChip('genre', friendship);
    assert.match(browser.context.buildPrompt(''), /amicizia/);
    browser.context.pickChip('genre', friendship);
    browser.context.pickChip('setting', chip(browser, 'setting', 'montagna'));
    assert.match(browser.context.buildPrompt(''), /montagne/);
});

test('generation snapshots all choices for favorites and another similar story', async () => {
    const browser = await readyBrowser();
    browser.run("appState.chosen.genres = ['amicizia', 'avventura', 'nanna']; appState.chosen.settingKeys = ['bosco', 'castello'];");
    browser.context.generateStory();
    const original = browser.sent[0].data;
    assert.equal(original.genere, 'amicizia, avventura, nanna');
    assert.equal(original.ambientazione, 'bosco, castello');
    browser.run("appState.chosen.genres.push('magia'); appState.chosen.settingKeys.length = 0;");
    assert.equal(browser.run('appState.currentStory.genres.join()'), 'amicizia,avventura,nanna');
    assert.equal(browser.run('appState.currentStory.settingKeys.join()'), 'bosco,castello');
    browser.context.handleResponse({ tipo: 'fine', racconto: 'La favola completa' });
    browser.run("appState.currentStory.title = 'La favola';");
    browser.context.toggleFavorite();
    const saved = JSON.parse(browser.context.localStorage.getItem('bardo_favorites'))[0];
    assert.deepEqual(saved.genres, ['amicizia', 'avventura', 'nanna']);
    assert.deepEqual(saved.settingKeys, ['bosco', 'castello']);
    browser.context.loadFavorite(0);
    browser.context.regenerateStory();
    assert.deepEqual(browser.sent[1].data, original);
});

test('importing multi-choice favorites retains all known choices and discards malformed metadata', async () => {
    const browser = await readyBrowser();
    const saved = { text: 'La favola', prompt: 'Il prompt originale', age: 4, timestamp: '2026-01-01',
        genres: ['amicizia', 'nanna', 'amicizia', null, 'unknown'], settingKeys: ['bosco', 'castello', {}, 'unknown'] };
    await browser.context.importFavorites({ value: '', files: [{ size: 100, text: async () => JSON.stringify([saved]) }] });
    browser.context.loadFavorite(0);
    browser.context.regenerateStory();
    assert.equal(browser.sent[0].data.genere, 'amicizia, nanna');
    assert.equal(browser.sent[0].data.ambientazione, 'bosco, castello');
    assert.equal(browser.run('appState.currentStory.genres.join()'), 'amicizia,nanna');
    assert.equal(browser.run('appState.currentStory.settingKeys.join()'), 'bosco,castello');
});

test('a new story resets all selections and removes the bedtime tone from its prompt', async () => {
    const browser = await contextFor('index.html');
    browser.context.pickChip('genre', chip(browser, 'genre', 'nanna'));
    browser.context.pickChip('setting', chip(browser, 'setting', 'bosco'));
    browser.context.newStory();
    assert.equal(browser.run('appState.chosen.genres.length'), 0);
    assert.equal(browser.run('appState.chosen.settingKeys.length'), 0);
    assert.equal(browser.context.buildPrompt(''), '');
    assert.equal(browser.context.buildPrompt('Una nuova idea'), 'Una nuova idea');
});
