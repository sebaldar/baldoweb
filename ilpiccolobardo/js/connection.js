// Sessione, connessione WebSocket e instradamento delle risposte.
import { appState } from './state.js';
import { failSpeech, receiveSpeech } from './reading.js';
import { advanceProgress, displayStory, failGeneration, refreshGenerationTimeout, stopProgress } from './generation.js';
import { displayIllustration } from './illustrations.js';
import { renderCompanyInfo, updateAccountButton } from './account.js';

export async function initGeo() {
  // In sviluppo senza HTTPS la geo via rete fallisce quasi sempre
  // Non vale la pena mostrare errori o aspettare il timeout
  if (!navigator.geolocation || location.protocol !== 'https:') {
    console.info('Geo: skip in contesto non sicuro, userà fallback IP')
    return
  }

  navigator.geolocation.getCurrentPosition(
    (pos) => {
      appState.geoCache = { lat: pos.coords.latitude, lon: pos.coords.longitude }
      console.info(`Geo acquisita: ${appState.geoCache.lat}, ${appState.geoCache.lon}`)
    },
    (err) => {
      console.info(`Geo non disponibile (code ${err.code}), userà fallback IP`)
      appState.geoCache = { lat: 40, lon: 10 }
    },
    { timeout: 5000, maximumAge: 300_000 }
  )
}

// ── API init (per impostare cookies) ─────────────────────────────────
export async function initAPI() {
    try {
        const response = await fetch('/api/session', {
            credentials: 'include'
        });

        if (response.ok) {
            const data = await response.json();
            console.log('📄 API Response:', data);

            if (data.session) {
                // ← FIX: Usa sessionStorage invece di cookie
                // Funziona sempre, anche cross-origin
                try { sessionStorage.setItem('SESSIONID', data.session); } catch (e) {
                    console.warn('Sessione non salvabile nel browser; uso il cookie.');
                }
                console.log('✅ SESSIONID salvato in sessionStorage:', data.session);

                // Verifica
                const saved = getSessionId();
                console.log('🔍 Verifica lettura:', saved);


                // settiamo anche il cookie
                // Imposta cookie con expire tra 24 ora
                const expires = new Date();
                expires.setHours(expires.getHours() + 24);

                document.cookie = `SESSIONID=${data.session}; expires=${expires.toUTCString()}; path=/; SameSite=None; Secure`;

            } else {
                console.error('❌ Response non contiene campo "session"');
            }

        } else {
            console.warn(`API risposta: ${response.status}`);
        }
    } catch (error) {
        console.warn('API non disponibile:', error);
    }



}

export function getSessionId() {
    try { return sessionStorage.getItem('SESSIONID'); } catch (e) { return null; }
}

// WebSocket Connection
export async function connect() {
    if (navigator.onLine === false || appState.reconnecting
        || (appState.ws && (appState.ws.readyState === WebSocket.OPEN || appState.ws.readyState === WebSocket.CONNECTING))) return;
    appState.reconnecting = true;
    try {

        updateStatus('connecting');

        await initAPI();
        if (navigator.onLine === false) return;
        initGeo();

        const socket = new WebSocket('wss://www.ilpiccolobardo.it/ws');
        appState.ws = socket;

        appState.ws.onopen = () => {
            if (appState.ws !== socket) return;
            clearInterval(appState.reconnectInterval);
            appState.reconnectInterval = null;
            updateStatus('connected');
            console.log('✓ Connesso al server!');
        };

        appState.ws.onmessage = (event) => {
            if (appState.ws !== socket) return;
            try {
                const data = JSON.parse(event.data);
                handleResponse(data);
            } catch (e) {
     //                   console.error('Errore parsing risposta:', e);
            }
        };

        appState.ws.onerror = (error) => {
            console.error('Errore WebSocket:', error);
        };

        appState.ws.onclose = () => {
            if (appState.ws !== socket) return;
            console.log('Disconnesso dal server!');
            updateStatus('disconnected');
            if (appState.pendingSpeech) failSpeech('La connessione si è interrotta mentre preparavo la voce. Riprova quando torna disponibile.');
            if (appState.isGenerating) failGeneration('La connessione si è interrotta. Quando torna disponibile, premi Riprova.');
            if (!appState.reconnectInterval) {
                appState.reconnectInterval = setInterval(() => {
                    if (appState.ws.readyState === WebSocket.CLOSED) {
                        connect();
                    }
                }, 5000);
            }
        };
    } finally {
        appState.reconnecting = false;
    }
}

export function sendMessage(action, data = {}) {
    if (navigator.onLine !== false && appState.ws && appState.ws.readyState === WebSocket.OPEN) {
        const message = {
            pagina: "IL_PICCOLO_BARDO",
            action: action,
            data: data
        };
        try {
            appState.ws.send(JSON.stringify(message));
            console.log(`→ Inviato: ${action}`);
            return true;
        } catch (error) {
            showToast('Non riesco a inviare la richiesta. Riprova quando la connessione torna disponibile.', 'error');
        }
    } else {
        showToast(navigator.onLine === false
            ? 'Sei offline: per questa azione serve la connessione.'
            : 'Non connesso al server! Riprova tra qualche secondo.', 'error');
    }
    return false;
}

// Ricezione dati dal WebSocket
export function handleResponse(response) {
    console.log('← Ricevuto:', response);

    const storyEvent = ['nodo_start', 'nodo_end', 'token', 'fine', 'errore'].includes(response.tipo)
        || response.action === 'generated_story';
    if (storyEvent) {
        if (!appState.isGenerating) return;
        refreshGenerationTimeout();
    }

    if (response.action === 'generated_story') {
        displayStory(response.data.story);
    } else if (response.action === 'illustration_generated') {
        displayIllustration(response.data.imageUrl);
    } else if (response.action === 'speech_generated') {
        receiveSpeech(response);
    } else if (response.action === 'image_generated') {
        displayIllustration(response.img);
    } else if (response.action === 'title_generated') {
        if (appState.titoloResolver) {
            appState.titoloResolver(response.title);
            appState.titoloResolver = null;
        }
    } else if (response.action === 'error') {
        if (response.requestAction === 'synthesize_speech') {
            if (appState.pendingSpeech && response.requestId === appState.pendingSpeech.id) failSpeech(response.message || 'Non riesco a preparare la voce. Riprova.');
            return;
        }
        if (appState.pendingSpeech && !appState.isGenerating) {
            failSpeech(response.message || 'Non riesco a preparare la voce. Riprova.');
            return;
        }
        if (appState.isGenerating) {
            failGeneration(response.message || 'Non riesco a creare la favola. Riprova tra poco.');
            return;
        }
        // Sblocca eventuali bottoni rimasti in stato "in corso" e avvisa l'utente
        document.getElementById('audioLoading').style.display = 'none';
        const illustrateBtn = document.getElementById('illustrateBtn');
        illustrateBtn.disabled = false;
        illustrateBtn.textContent = '🎨 Crea Illustrazione';
        showToast('Si è verificato un errore: ' + (response.message || 'errore sconosciuto'), 'error');
    }
    // Nuovi messaggi dal server
    else if (response.action === 'user_status') {
        appState.currentUser = response.data.user || null;
        updateAccountButton();
    }
    else if (response.action === 'company_info') {
        try { localStorage.setItem('bardo_company', JSON.stringify(response.data)); } catch (e) {}
        renderCompanyInfo(response.data);
    // ── Nuovi eventi SSE ritrasmessi da Node ────────────────────────
    } else if (response.tipo === 'nodo_start') {
        const statusEl = document.getElementById('loadingStatus');
        if (statusEl) {
            const messaggi = {
                'avvio':                 '🪄 Il Piccolo Bardo si sveglia...',
                'analizza_prompt':       '🔍 Leggo la tua idea e cerco i protagonisti...',
                'valuta_prompt':         '🛡️ Verifico che la storia sia adatta ai bambini...',
                'decide_tools':          '🛠️ Scelgo quali magie usare per la favola...',
                'fetch_contesto_fisico': '🌍 Meteo e cielo stellato in arrivo...',
                'query_neo4j':           '📚 Sfoglio il libro dei ricordi del Bardo...',
                'valuta_frammenti':      '🔎 Valuto i ricordi trovati...',
                'componi_prompt':        '🖊️ Intreccio personaggi, stelle e ricordi...',
                'genera_draft':          '✍️ Il Bardo prende la penna... la storia sta nascendo!',
                'valuta_draft':          '🧐 Rileggo la bozza con occhio critico...',
                'correggi_draft':        '🔧 Limo gli spigoli del racconto...',
                'rifinisci':             '💎 Scelgo ogni parola per le orecchie dei bambini...',
                'salva_memoria':         '💾 Affido la storia alla memoria del Bardo...',
                'nodo_errore':           '😔 Qualcosa non va... riprovo!',
            };
            const testo = messaggi[response.nodo] ?? `✨ ${response.nodo.replace(/_/g, ' ')}...`;
            advanceProgress(response.nodo);
            statusEl.style.transition = 'opacity 0.2s';
            statusEl.style.opacity = '0';
            setTimeout(() => { statusEl.textContent = testo; statusEl.style.opacity = '1'; }, 200);
        }

    } else if (response.tipo === 'token') {
        // Mostra il testo in streaming token per token
        const loadingEl = document.getElementById('loadingIndicator');
        const streamEl = document.getElementById('streamingText');

        if (loadingEl) loadingEl.style.display = 'none';
        stopProgress();
        if (streamEl) {
            streamEl.style.display = 'block';
            streamEl.textContent += response.testo;
            appState.currentStory.text += response.testo;  // accumula per TTS e preferiti
        }

    } else if (response.tipo === 'fine') {
        // Un prompt rifiutato da valuta_prompt arriva qui con
        // tipo 'fine' e racconto vuoto, non come evento 'errore' —
        // prima di questo controllo restava silenzioso: il
        // bambino/genitore vedeva solo una card vuota, senza
        // nessuna spiegazione del perché.
        if (response.errore) {
            failGeneration('Non posso scrivere questa storia così com\'è: '
                + (response.motivo_rifiuto || response.errore) + ' Prova a cambiare la tua richiesta.', false);
            return;
        }
        if (typeof response.racconto !== 'string' || !response.racconto.trim()) {
            failGeneration('La favola non è arrivata completa. Riprova.');
            return;
        }

        // Racconto completo ricevuto
        appState.currentStory.text = response.racconto;
        appState.currentStory.storia_id = response.storia_id;

        displayStory(response.racconto);

        console.log(`✅ Favola completata in ${response.iterazioni} iterazioni`);

    } else if (response.tipo === 'errore') {
        failGeneration(response.messaggio || 'Qualcosa è andato storto. Riprova!');
    }

}

export function updateStatus(status) {
    const statusEl = document.getElementById('connectionStatus');
    statusEl.className = `connection-badge ${status}`;
    statusEl.textContent = status === 'connected' ? '🟢 Connesso' :
                          status === 'connecting' ? '🟡 Connessione...' :
                          '🔴 Disconnesso';
}
