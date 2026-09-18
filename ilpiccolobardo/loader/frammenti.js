/**
 * Modulo di Gestione Archivio - Il Piccolo Bardo
 */

const CONFIG = {
    API_URL: '/api/ILPICCOLOBARDO',
    PAGINA: 'IL_PICCOLO_BARDO'
};

let currentFileData = null;

// --- Inizializzazione ---
document.addEventListener('DOMContentLoaded', () => {
    initEventListeners();
    fetchStats();
});

function initEventListeners() {
    const fileInput = document.getElementById('file-input');
    const dropZone = document.getElementById('drop-zone');
    
    // Gestione File
    fileInput.addEventListener('change', (e) => handleFileSelect(e.target.files[0]));
    dropZone.addEventListener('dragover', (e) => { e.preventDefault(); dropZone.classList.add('active'); });
    dropZone.addEventListener('dragleave', () => dropZone.classList.remove('active'));
    dropZone.addEventListener('drop', (e) => {
        e.preventDefault();
        handleFileSelect(e.dataTransfer.files[0]);
    });

    // Pulsanti
    document.getElementById('btn-upload').onclick = uploadData;
    document.getElementById('btn-refresh').onclick = fetchList;
    document.getElementById('btn-stats').onclick = fetchStats;
    document.getElementById('btn-delete').onclick = deleteFragments;
}

// --- Funzioni Core ---

async function callApi(action, payload = {}, fileElement = null) {
    try {
        const formData = new FormData();
        
        // Aggiungiamo i campi testuali
        formData.append('action', action);
        formData.append('pagina', CONFIG.PAGINA);
        
        // Se il payload è un oggetto, lo trasformiamo in stringa JSON 
        // per non perdere la struttura
        formData.append('data', JSON.stringify(payload));

        // Se c'è un file (es. da un input type="file")
        if (fileElement && fileElement.files[0]) {
            formData.append('file_grimorio', fileElement.files[0]);
        }

        const response = await AdminAccess.fetch(CONFIG.API_URL, {
            method: 'POST',
            // NOTA: Non impostare 'Content-Type'. 
            // Il browser lo farà da solo includendo il "boundary" corretto.
            body: formData 
        });

        if (!response.ok) throw new Error(`Errore Server: ${response.status}`);
        return await response.json();
    } catch (err) {
        updateLog(`Errore API: ${err.message}`, 'err');
        throw err;
    }
}


let rawFileObject = null; // Memorizza l'oggetto File originale

function handleFileSelect(file) {
    if (!file) return;
    rawFileObject = file; // <--- IMPORTANTE: tieni il riferimento al file binario

    const reader = new FileReader();
    reader.onload = (e) => {
        try {
            const previewData = JSON.parse(e.target.result);
            document.getElementById('selected-filename').textContent = `✓ ${file.name}`;
            document.getElementById('btn-upload').disabled = false;
            updateLog(`Pronto: ${file.name}`, 'ok');
        } catch (err) {
            updateLog(`Errore JSON: ${err.message}`, 'err');
        }
    };
    reader.readAsText(file);
}

async function uploadData() {
    if (!rawFileObject) return;

    updateLog('Caricamento in corso...', 'load');

    const formData = new FormData();
    // Aggiungi i campi testuali
    formData.append('action', 'admin_bulk_load');
    formData.append('pagina', CONFIG.PAGINA);
    formData.append('sovrascrivi', document.getElementById('overwrite-check').checked);

    // Aggiungi il file BINARIO (rawFileObject è l'oggetto File di sistema)
    formData.append('file_grimorio', rawFileObject);

    try {
        const response = await AdminAccess.fetch(CONFIG.API_URL, {
            method: 'POST',
            // NOTA: Non impostare 'Content-Type': 'application/json' qui!
            // Il browser imposterà automaticamente 'multipart/form-data' con il boundary corretto.
            body: formData
        });
        const res = await response.json();
        if (!response.ok || res.status === 'error') {
            throw new Error(res.message || `Errore Server: ${response.status}`);
        }
        updateLog(`Caricati: ${res.caricati}, saltati: ${res.saltati}, errori: ${res.errori?.length || 0}`, 'ok');
        fetchStats();
        fetchList();
    } catch (err) {
        updateLog(`Errore caricamento: ${err.message}`, 'err');
    }
}

async function fetchStats() {
    try {
        const res = await callApi('admin_stats');
        const l = res.labels || {};
        document.getElementById('stat-fragments').textContent = l.PlotFragment || 0;
        document.getElementById('stat-stories').textContent = l.Story || 0;
        document.getElementById('stat-characters').textContent = l.Character || 0;
        document.getElementById('stat-emotions').textContent = l.Emotion || 0;
    } catch (e) { console.error("Stats fail", e); }
}

async function fetchList() {
    updateLog('Sincronizzazione archivio...', 'load');
    try {
        const res = await callApi('admin_list_fragments');
        renderGrid(res.frammenti || []);
        updateLog(`Archivio aggiornato: ${res.frammenti?.length} elementi.`, 'ok');
    } catch (e) {}
}

async function deleteFragments() {
    const rawIds = document.getElementById('delete-ids').value.trim();
    if (!rawIds) return updateLog('Inserire ID validi', 'err');

    const ids = rawIds.split(/[\n,]+/).map(s => s.trim()).filter(Boolean);
    if (!confirm(`Eliminare definitivamente ${ids.length} frammenti?`)) return;

    try {
        const res = await callApi('admin_delete_fragments', { ids });
        updateLog(`Eliminati con successo: ${res.eliminati}`, 'ok');
        document.getElementById('delete-ids').value = '';
        fetchStats();
        fetchList();
    } catch (e) {}
}

// --- Rendering ---

function renderGrid(fragments) {
    const container = document.getElementById('fragment-list');
    document.getElementById('fragment-count').textContent = `${fragments.length} frammenti`;

    if (fragments.length === 0) {
        container.innerHTML = '<div class="empty-state">L\'archivio è vuoto.</div>';
        return;
    }

    container.innerHTML = fragments.map(f => `
        <article class="fcard">
            <header class="fid">
                <strong>${escapeHTML(f.id || 'N/A')}</strong>
                <small>${escapeHTML(f.archetipo || '')}</small>
            </header>
            <p class="ftxt">${escapeHTML(f.text || '')}</p>
            <footer class="ftags">
                ${f.setting ? `<span class="tag ts">📍 ${escapeHTML(f.setting)}</span>` : ''}
                ${(f.characters || []).map(c => `<span class="tag tc">${escapeHTML(c)}</span>`).join('')}
                ${(f.emotions || []).map(e => `<span class="tag te">${escapeHTML(e)}</span>`).join('')}
            </footer>
        </article>
    `).join('');
}

function updateLog(msg, type) {
    const el = document.getElementById('log-display');
    el.textContent = msg;
    el.className = `log-output ${type || ''}`;
}
