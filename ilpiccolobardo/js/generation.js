// Personalizzazione, generazione e visualizzazione delle favole.
import { appState } from './state.js';
import { saveProfile, updateFavoriteButton } from './favorites.js';
import { exitReadingMode, stopReading } from './reading.js';
import { getSessionId, sendMessage } from './connection.js';
import { GENRE_PHRASE, SETTING_PHRASE, normalizeStoryChoices } from './choices.js';

export function pickChip(group, btn) {
    const field = group === 'genre' ? 'genres' : 'settingKeys';
    const key = group === 'genre' ? btn.dataset.value : btn.dataset.key;
    const options = group === 'genre' ? GENRE_PHRASE : SETTING_PHRASE;
    if (!Object.hasOwn(options, key)) return;
    const on = btn.getAttribute('aria-pressed') !== 'true'; // un secondo tocco deseleziona
    btn.setAttribute('aria-pressed', String(on));
    const remaining = appState.chosen[field].filter(value => value !== key);
    appState.chosen[field] = on ? [...remaining, key] : remaining;
}

export function buildPrompt(freeText) {
    const text = freeText.trim();
    const { genres, settingKeys } = appState.chosen;
    if (!text && !genres.length && !settingKeys.length) return '';
    const parts = [text || 'Racconta una storia.'];
    const themes = genres.filter(key => key !== 'nanna').map(key => GENRE_PHRASE[key]);
    if (themes.length) parts.push(`Intreccia questi temi nella storia: ${themes.join(', ')}.`);
    if (settingKeys.length) {
        parts.push(`Elementi dell'ambientazione: ${settingKeys.map(key => SETTING_PHRASE[key]).join('; ')}.`);
        if (settingKeys.length > 1) {
            parts.push('Combina gli elementi scelti in un\'ambientazione coerente: possono convivere nello stesso luogo, senza richiedere una tappa del viaggio per ciascuno.');
        }
    }
    if (genres.includes('nanna')) {
        parts.push('Mantieni un ritmo dolce e rilassante, adatto ad addormentarsi, anche insieme ad avventura o magia.');
    }
    return parts.join(' ');
}

export function selectAge(age, btn) {
    appState.selectedAge = age;
    document.querySelectorAll('.age-btn').forEach(b => {
        b.classList.remove('selected');
        b.setAttribute('aria-pressed', 'false');
    });
    btn.classList.add('selected');
    btn.setAttribute('aria-pressed', 'true');
}

export function setGenerationState(active) {
    appState.isGenerating = active;
    for (const id of ['generateBtn', 'regenerateBtn', 'newStoryBtn']) {
        document.getElementById(id).disabled = active;
    }
    document.getElementById('generateBtn').textContent = active ? '⏳ Creo la favola...' : '✨ Crea la Favola';
    if (!active) {
        clearTimeout(appState.generationTimer);
        appState.generationTimer = null;
        stopProgress();
    }
}

export function refreshGenerationTimeout() {
    clearTimeout(appState.generationTimer);
    // È un limite di inattività: ogni evento ricevuto riavvia l'attesa.
    appState.generationTimer = setTimeout(() => {
        failGeneration('Non ricevo più aggiornamenti dal Bardo. Riprova tra poco.');
        // Chiude anche il vecchio stream, così non si mescola a un nuovo tentativo.
        if (appState.ws) appState.ws.close();
    }, 240000);
}

export function failGeneration(message, retry = true) {
    setGenerationState(false);
    if (appState.currentStory) appState.currentStory.text = '';
    for (const id of ['readBtn', 'illustrateBtn', 'printBtn']) document.getElementById(id).disabled = true;
    document.getElementById('nextActions').hidden = true;
    document.getElementById('storyDisplay').innerHTML = `
        <div class="loading" role="alert">
            <p>${escapeHTML(message)}</p>
            ${retry ? '<button class="btn-primary" onclick="regenerateStory()">🔁 Riprova</button>' : ''}
        </div>
    `;
}

// Generate Story
export function generateStory() {
    if (appState.isGenerating) return;
    if (!appState.selectedAge) {
        showToast('Seleziona prima l\'età del bambino! 🎂', 'warning');
        return;
    }

    const prompt = buildPrompt(document.getElementById('userPrompt').value);
    if (!prompt) {
        showToast('Scegli che storia vuoi, oppure scrivi cosa deve succedere! 💭', 'warning');
        return;
    }
    const childName = document.getElementById('childName').value;
    const favoriteAnimal = document.getElementById('favoriteAnimal').value;
    const favoriteColor = document.getElementById('favoriteColor').value;

    let enrichedPrompt = prompt;
    if (childName) enrichedPrompt += ` Il protagonista si chiama ${childName}.`;
    if (favoriteAnimal) enrichedPrompt += ` Nella storia c'è un ${favoriteAnimal}.`;
    if (favoriteColor) enrichedPrompt += ` Usa il colore ${favoriteColor} nella narrazione.`;

    const started = startStoryGeneration({
        prompt: enrichedPrompt, age: appState.selectedAge, childName: childName || null,
        favoriteAnimal, favoriteColor, genres: appState.chosen.genres, settingKeys: appState.chosen.settingKeys,
    });
    if (started) saveProfile();
}

export function startStoryGeneration(story) {
    if (appState.isGenerating) return false;
    if (navigator.onLine === false || !appState.ws || appState.ws.readyState !== WebSocket.OPEN) {
        showToast(navigator.onLine === false
            ? 'Sei offline: per creare una favola serve la connessione.'
            : 'Il Bardo si sta riconnettendo. Riprova tra qualche secondo.', 'warning');
        return false;
    }
    if (!story || !story.prompt || ![3, 4, 5, 6].includes(Number(story.age))) {
        showToast('Questa favola non contiene la richiesta originale completa. Scegli una nuova storia nel modulo.', 'warning');
        return false;
    }
    stopReading();
    document.getElementById('nextActions').hidden = true;
    if (appState.readingMode) exitReadingMode();
    // Una nuova copia evita di modificare la favola già salvata nei preferiti.
    appState.currentStory = {
        prompt: story.prompt, age: Number(story.age), childName: story.childName || null,
        favoriteAnimal: story.favoriteAnimal || null, favoriteColor: story.favoriteColor || null,
        ...normalizeStoryChoices(story),
        timestamp: new Date().toISOString(), text: '',
    };
    setGenerationState(true);

    // Prepara il DOM PRIMA di inviare, così loadingStatus esiste
    // quando arrivano i primi eventi nodo_start dal server
    document.getElementById('storyCard').style.display = 'block';
    document.getElementById('storyDisplay').innerHTML = `
        <button class="favorite-btn" id="favoriteBtn" disabled aria-label="Aggiungi ai preferiti" aria-pressed="false">🤍</button>
        <div class="loading" id="loadingIndicator">
            <div class="loading-spinner"></div>
            <p id="loadingStatus" role="status" aria-live="polite">🪄 Il Piccolo Bardo si sveglia...</p>
            <div class="progress" role="progressbar" aria-label="Avanzamento della favola" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" id="loadingProgress"><div class="progress-fill" id="loadingProgressFill"></div></div>
        </div>
        <div id="streamingText" style="display:none; white-space: pre-wrap; line-height: 1.8;"></div>
    `;
    startProgress();
    refreshGenerationTimeout();
    document.getElementById('readBtn').disabled = true;
    document.getElementById('illustrateBtn').disabled = true;
    document.getElementById('printBtn').disabled = true;
    document.getElementById('storyIllustration').style.display = 'none';
    document.getElementById('storyCard').scrollIntoView({ behavior: 'smooth' });

    // Invia a Node via WebSocket con geo
    const sent = sendMessage('generate_story', {
        endpoint : "fastapi",
        userPrompt: appState.currentStory.prompt,
        age: appState.currentStory.age,
        geo: appState.geoCache,           // lat/lon del dispositivo, o null
        session_id: getSessionId(),
        // Campi distinti oltre al testo intrecciato sopra: servono al
        // report YAML amministrativo di ogni storia generata.
        nome: appState.currentStory.childName,
        coloreP: appState.currentStory.favoriteColor,
        animaleP: appState.currentStory.favoriteAnimal,
        genere: appState.currentStory.genre,
        ambientazione: appState.currentStory.settingKey,
    });
    if (!sent) {
        failGeneration('La richiesta non è partita. Riprova quando la connessione torna disponibile.');
        if (appState.ws) appState.ws.close();
    }
    return sent;
}

// ── Barra di avanzamento della generazione ───────────────────────────
export const PROGRESS_STEPS = ['avvio', 'analizza_prompt', 'valuta_prompt', 'decide_tools', 'fetch_contesto_fisico',
    'query_neo4j', 'valuta_frammenti', 'componi_prompt', 'genera_draft', 'valuta_draft', 'correggi_draft', 'rifinisci', 'salva_memoria'];

export function setProgress(pct) {
    const fill = document.getElementById('loadingProgressFill');
    const bar = document.getElementById('loadingProgress');
    if (fill) fill.style.width = `${pct}%`;
    if (bar) bar.setAttribute('aria-valuenow', String(Math.round(pct)));
}

export function startProgress() {
    appState.progressReached = 0;
    clearTimeout(appState.slowTimer);
    // se ci mette parecchio, avvisa che sta ancora lavorando
    appState.slowTimer = setTimeout(() => {
        const el = document.getElementById('loadingStatus');
        if (el) el.textContent = '⏳ Ci sta mettendo un po\'... il Bardo sta ancora lavorando, ancora un attimo ✨';
    }, 45000);
}

export function advanceProgress(node) {
    const i = PROGRESS_STEPS.indexOf(node);
    if (i < 0) return;
    appState.progressReached = Math.max(appState.progressReached, i + 1); // i nodi possono ripetersi: non si torna indietro
    setProgress(Math.min(95, (appState.progressReached / PROGRESS_STEPS.length) * 100));
}

export function stopProgress() { clearTimeout(appState.slowTimer); }

export function displayStory(story) {
    setGenerationState(false);
    appState.currentStory.text = story;

    const display = document.getElementById('storyDisplay');
    display.innerHTML = `
        <button class="favorite-btn" onclick="toggleFavorite()" id="favoriteBtn">🤍</button>
        ${escapeHTML(story).replace(/\n/g, '<br>')}
    `;

    document.getElementById('readBtn').disabled = false;
    document.getElementById('illustrateBtn').disabled = false;
    document.getElementById('printBtn').disabled = false;
    document.getElementById('nextActions').hidden = false;
    document.getElementById('regenerateBtn').disabled = !appState.currentStory.prompt
        || ![3, 4, 5, 6].includes(Number(appState.currentStory.age));
    const illustration = document.getElementById('storyIllustration');
    illustration.onload = null;
    illustration.style.display = 'none';
    illustration.removeAttribute('src');
    if (appState.currentStory.illustration) {
        illustration.src = appState.currentStory.illustration;
        illustration.style.display = 'block';
    }

    // Verifica se è già nei preferiti
    updateFavoriteButton();
}

// ── Dopo la storia: un'altra simile o una nuova ──────────────────────
export function regenerateStory() {
    startStoryGeneration(appState.currentStory);
}

export function newStory() {
    if (appState.isGenerating) return;
    if (appState.readingMode) exitReadingMode();
    stopReading();
    document.getElementById('nextActions').hidden = true;
    document.getElementById('storyCard').style.display = 'none';
    document.getElementById('userPrompt').value = '';
    document.querySelectorAll('#genreChips .chip, #settingChips .chip').forEach(c => c.setAttribute('aria-pressed', 'false'));
    appState.chosen.genres = [];
    appState.chosen.settingKeys = [];
    window.scrollTo({ top: 0, behavior: 'smooth' });
    document.querySelector('#genreChips .chip').focus({ preventScroll: true });
}
