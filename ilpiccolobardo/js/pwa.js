// Installazione, cache offline e riconnessione.
import { connect } from './connection.js';
import { appState } from './state.js';
import { failSpeech } from './reading.js';
import { failGeneration } from './generation.js';

// ── PWA: service worker, stato rete, riconnessione ───────────────────
if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
        navigator.serviceWorker.register('/sw.js').catch((e) => console.warn('Service worker non registrato:', e));
    });
}

export function updateOnlineBanner() {
    document.getElementById('offlineBanner').hidden = navigator.onLine !== false;
}

export function reconnectIfNeeded() {
    connect();
}

window.addEventListener('online', () => { updateOnlineBanner(); reconnectIfNeeded(); });

window.addEventListener('offline', () => {
    updateOnlineBanner();
    if (appState.pendingSpeech) failSpeech('Sei offline: la preparazione della voce si è interrotta. Riprova quando torna la connessione.');
    if (appState.isGenerating) {
        failGeneration('Sei offline: la creazione si è interrotta. Quando torna la connessione, premi Riprova.');
        if (appState.ws) appState.ws.close();
    }
});

document.addEventListener('visibilitychange', () => {
    if (document.visibilityState === 'visible') reconnectIfNeeded();
});

updateOnlineBanner();

export const isStandalone = () => window.matchMedia('(display-mode: standalone)').matches || navigator.standalone === true;

export const isIOS = /iphone|ipad|ipod/i.test(navigator.userAgent) && !window.MSStream;

window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    appState.installPrompt = e;
    document.getElementById('installBtn').hidden = false;
});

window.addEventListener('appinstalled', () => {
    appState.installPrompt = null;
    document.getElementById('installBtn').hidden = true;
    showToast('App installata! La trovi nella schermata Home. ✨', 'success');
});

// Su iPhone e iPad non esiste il prompt: si mostra un'istruzione
if (isIOS && !isStandalone()) document.getElementById('installBtn').hidden = false;

export async function installApp() {
    if (appState.installPrompt) {
        appState.installPrompt.prompt();
        await appState.installPrompt.userChoice.catch(() => {});
        appState.installPrompt = null;
        document.getElementById('installBtn').hidden = true;
    } else {
        showToast('Per installare: tocca Condividi, poi "Aggiungi a Home".', 'info', 8000);
    }
}
