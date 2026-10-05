// Comfort di lettura, preparazione della voce e controlli audio.
import { appState } from './state.js';
import { sendMessage } from './connection.js';

// ── Comfort di lettura: dimensione del testo e lettura serale ─────────
export const TEXT_SIZES = [1.1, 1.3, 1.6, 1.9];

export function applyTextSize() {
    document.documentElement.style.setProperty('--story-size', `${TEXT_SIZES[appState.textSizeIdx]}em`);
    document.getElementById('sizeDown').disabled = appState.textSizeIdx === 0;
    document.getElementById('sizeUp').disabled = appState.textSizeIdx === TEXT_SIZES.length - 1;
}

export function changeTextSize(delta) {
    appState.textSizeIdx = Math.min(TEXT_SIZES.length - 1, Math.max(0, appState.textSizeIdx + delta));
    applyTextSize();
    try { localStorage.setItem('bardo_text_size', String(appState.textSizeIdx)); } catch (e) {}
}

export function loadTextSize() {
    try {
        const saved = parseInt(localStorage.getItem('bardo_text_size'), 10);
        if (saved >= 0 && saved < TEXT_SIZES.length) appState.textSizeIdx = saved;
    } catch (e) {}
    applyTextSize();
}

export function enterReadingMode() {
    appState.readingMode = true;
    document.body.classList.add('reading-mode');
    document.getElementById('readingModeBtn').setAttribute('aria-pressed', 'true');
    document.getElementById('exitReadingBtn').hidden = false;
    acquireWakeLock(); // il genitore legge: lo schermo deve restare acceso
    window.scrollTo({ top: 0 });
}

export function exitReadingMode() {
    appState.readingMode = false;
    document.body.classList.remove('reading-mode');
    document.getElementById('readingModeBtn').setAttribute('aria-pressed', 'false');
    document.getElementById('exitReadingBtn').hidden = true;
    if (!appState.currentAudio || appState.currentAudio.paused) releaseWakeLock();
    document.getElementById('readingModeBtn').focus();
}

export function toggleReadingMode() { appState.readingMode ? exitReadingMode() : enterReadingMode(); }

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && appState.readingMode && !document.querySelector('dialog[open]')) exitReadingMode();
});

export async function acquireWakeLock() {
    try {
        if ('wakeLock' in navigator && !appState.wakeLock) {
            appState.wakeLock = await navigator.wakeLock.request('screen');
            appState.wakeLock.addEventListener('release', () => { appState.wakeLock = null; });
        }
    } catch (e) { appState.wakeLock = null; }
}

export function releaseWakeLock() {
    if (appState.wakeLock) { appState.wakeLock.release().catch(() => {}); appState.wakeLock = null; }
}

// Il browser rilascia il blocco quando la pagina va in background: lo riprendo al ritorno
document.addEventListener('visibilitychange', () => {
    if (document.visibilityState === 'visible' && (appState.readingMode || (appState.currentAudio && !appState.currentAudio.paused))) acquireWakeLock();
});

export function releaseWakeLockUnlessNeeded() { if (!appState.readingMode) releaseWakeLock(); }

export function setReadingUI(state) { // 'idle' | 'loading' | 'playing' | 'paused'
    const loading = state === 'loading';
    const active = state !== 'idle';
    document.getElementById('readBtn').style.display = active ? 'none' : 'block';
    document.getElementById('readBtn').disabled = appState.isGenerating || !appState.currentStory?.text || loading;
    document.getElementById('pauseBtn').style.display = active && !loading ? 'block' : 'none';
    document.getElementById('stopBtn').style.display = active ? 'block' : 'none';
    document.getElementById('stopBtn').textContent = loading ? '✕ Annulla audio' : '⏹️ Ferma Lettura';
    document.getElementById('audioLoading').style.display = loading ? 'block' : 'none';
    document.getElementById('pauseBtn').textContent = state === 'paused' ? '▶️ Riprendi' : '⏸️ Pausa';
    document.getElementById('audioVisualizer').classList.toggle('active', state === 'playing');
}

export function playNarration(url) {
    stopReading();
    const audio = new Audio(url);
    appState.currentAudio = audio;
    audio.addEventListener('play', () => { setReadingUI('playing'); acquireWakeLock(); setMediaSessionState('playing'); });
    audio.addEventListener('pause', () => {
        if (audio.ended || appState.currentAudio !== audio) return;
        setReadingUI('paused'); releaseWakeLockUnlessNeeded(); setMediaSessionState('paused');
    });
    audio.addEventListener('ended', () => { if (appState.currentAudio === audio) stopReading(); });
    audio.addEventListener('error', () => {
        if (appState.currentAudio !== audio) return;
        stopReading();
        showToast('Non riesco a riprodurre l\'audio. Riprova.', 'error');
    });
    setupMediaSession();
    setReadingUI('paused'); // se il browser blocca l'avvio automatico resta "Riprendi"
    audio.play().catch(() => {});
}

export function togglePause() {
    if (!appState.currentAudio) return;
    if (appState.currentAudio.paused) appState.currentAudio.play().catch(() => {});
    else appState.currentAudio.pause();
}

export function setupMediaSession() {
    if (!('mediaSession' in navigator)) return;
    navigator.mediaSession.metadata = new MediaMetadata({
        title: 'Una favola del Piccolo Bardo',
        artist: 'Il Piccolo Bardo',
        artwork: [
            { src: '/public/icons/icon-192.png', sizes: '192x192', type: 'image/png' },
            { src: '/public/icons/icon-512.png', sizes: '512x512', type: 'image/png' },
        ],
    });
    navigator.mediaSession.setActionHandler('play', () => appState.currentAudio?.play().catch(() => {}));
    navigator.mediaSession.setActionHandler('pause', () => appState.currentAudio?.pause());
    navigator.mediaSession.setActionHandler('stop', () => stopReading());
}

export function setMediaSessionState(state) {
    if ('mediaSession' in navigator) navigator.mediaSession.playbackState = state;
}

export function stopReading() {
    if (appState.pendingSpeech) {
        clearTimeout(appState.pendingSpeech.timer);
        appState.pendingSpeech = null;
    }
    if (appState.currentAudio) {
        const audio = appState.currentAudio;
        appState.currentAudio = null;
        audio.pause();
        audio.removeAttribute('src');
        audio.load();
    }
    if (appState.speechSynthesis?.speaking) {
        appState.speechSynthesis.cancel();
    }
    releaseWakeLockUnlessNeeded();
    setMediaSessionState('none');
    setReadingUI('idle');
}

export function listenStory() {
    if (appState.isGenerating || !appState.currentStory?.text || appState.currentAudio || appState.pendingSpeech) return;
    if (navigator.onLine === false || !appState.ws || appState.ws.readyState !== WebSocket.OPEN) {
        showToast('Per preparare la voce serve la connessione. Puoi leggere la favola sullo schermo.', 'warning');
        return;
    }
    const request = {
        id: `speech-${Date.now()}-${++appState.speechSequence}`,
        story: appState.currentStory.timestamp, text: appState.currentStory.text,
    };
    appState.pendingSpeech = request;
    if (!sendMessage('synthesize_speech', { text: request.text, requestId: request.id })) {
        appState.pendingSpeech = null;
        return;
    }
    setReadingUI('loading');
    request.timer = setTimeout(() => failSpeech('La preparazione della voce sta impiegando troppo tempo. Riprova.'), 120000);
}

export function failSpeech(message) {
    if (!appState.pendingSpeech) return;
    clearTimeout(appState.pendingSpeech.timer);
    appState.pendingSpeech = null;
    setReadingUI('idle');
    showToast(message, 'error');
}

export function receiveSpeech(response) {
    if (!appState.pendingSpeech || response.requestId !== appState.pendingSpeech.id) return;
    if (appState.pendingSpeech.story !== appState.currentStory?.timestamp || appState.pendingSpeech.text !== appState.currentStory?.text) {
        stopReading();
        return;
    }
    if (typeof response.audioUrl !== 'string' || !response.audioUrl.trim()) {
        failSpeech('La voce non è arrivata completa. Riprova.');
        return;
    }
    clearTimeout(appState.pendingSpeech.timer);
    appState.pendingSpeech = null;
    playNarration(response.audioUrl);
}

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    stopReading();
});
