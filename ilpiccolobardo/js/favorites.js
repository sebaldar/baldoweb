// Archivio locale, importazione/esportazione e profilo sul dispositivo.
import { appState } from './state.js';
import { displayStory, selectAge } from './generation.js';
import { TITOLO_PREDEFINITO, richiediTitolo } from './pdf.js';
import { stopReading } from './reading.js';
import { switchTab } from './ui.js';
import { normalizeStoryChoices } from './choices.js';

// Carica preferiti dal localStorage
export function loadFavorites() {
    try {
        const stored = localStorage.getItem('bardo_favorites');
        const loaded = stored ? JSON.parse(stored) : [];
        if (!Array.isArray(loaded) || loaded.some(fav => !fav || typeof fav.text !== 'string' || typeof fav.timestamp !== 'string')) {
            throw new Error('Archivio non valido');
        }
        appState.favorites = loaded;
        appState.favoritesReadable = true;
    } catch (e) {
        appState.favorites = [];
        appState.favoritesReadable = false;
        showToast('Non riesco a leggere le favole salvate su questo dispositivo. Puoi importare una copia esportata.', 'error', 8000);
    }
    if (!Array.isArray(appState.favorites)) appState.favorites = [];
    displayFavorites();
}

export function saveFavorites(next = appState.favorites, recover = false) {
    if (!appState.favoritesReadable && !recover) {
        showToast('L\'archivio sul dispositivo non è leggibile. Importa una copia esportata prima di salvare nuove favole.', 'error', 8000);
        return false;
    }
    try {
        localStorage.setItem('bardo_favorites', JSON.stringify(next));
        appState.favorites = next;
        appState.favoritesReadable = true;
        return true;
    } catch (error) {
        showToast('Non riesco a salvare le favole su questo dispositivo: lo spazio potrebbe essere esaurito o il salvataggio bloccato. Usa Esporta per fare una copia.', 'error', 8000);
        return false;
    }
}

// ── Profilo ricordato su questo dispositivo ──────────────────────────
export function saveProfile() {
    try {
        const name = document.getElementById('childName').value.trim();
        localStorage.setItem('bardo_profile', JSON.stringify({ name, age: appState.selectedAge }));
    } catch (e) {
        document.getElementById('profileHint').hidden = true;
        showToast('La favola verrà creata, ma non riesco a ricordare nome ed età su questo dispositivo.', 'warning');
        return;
    }
    document.getElementById('profileHint').hidden = false;
}

export function loadProfile() {
    let profile = null;
    try { profile = JSON.parse(localStorage.getItem('bardo_profile')); } catch (e) {}
    if (!profile) return;
    if (profile.name) document.getElementById('childName').value = profile.name;
    if (profile.age) {
        const btn = [...document.querySelectorAll('.age-btn')].find(b => b.textContent.trim().startsWith(String(profile.age)));
        if (btn) selectAge(profile.age, btn);
    }
    document.getElementById('profileHint').hidden = false;
}

export function forgetProfile() {
    try { localStorage.removeItem('bardo_profile'); } catch (e) {
        showToast('Non riesco a cancellare il profilo salvato su questo dispositivo. Riprova.', 'error');
        return;
    }
    document.getElementById('childName').value = '';
    appState.selectedAge = null;
    document.querySelectorAll('.age-btn').forEach(b => { b.classList.remove('selected'); b.setAttribute('aria-pressed', 'false'); });
    document.getElementById('profileHint').hidden = true;
    showToast('Ho dimenticato nome ed età.', 'info');
}

// Favorites Management
export function toggleFavorite() {
    if (appState.isGenerating || !appState.currentStory || !appState.currentStory.text) return;

    const index = appState.favorites.findIndex(f => f.timestamp === appState.currentStory.timestamp);

    const next = appState.favorites.slice();
    if (index > -1) {
        // Rimuovi dai preferiti
        next.splice(index, 1);
    } else {
        // Aggiungi ai preferiti
        next.unshift(appState.currentStory);
    }

    if (!saveFavorites(next)) return;
    updateFavoriteButton();
    displayFavorites();
    if (index === -1) nameFavorite(appState.currentStory);
}

export async function nameFavorite(story) {
    if (story.title || !story.text || navigator.onLine === false || appState.titoloResolver) return;
    const titolo = await richiediTitolo(story.text);
    if (titolo && titolo !== TITOLO_PREDEFINITO && appState.favorites.includes(story)) {
        const next = appState.favorites.map(fav => fav === story ? { ...fav, title: titolo } : fav);
        if (!saveFavorites(next)) return;
        if (appState.currentStory === story) appState.currentStory = next.find(fav => fav.timestamp === story.timestamp);
        displayFavorites();
    }
}

export function favoriteTitle(fav) {
    return fav.title || (fav.childName ? `Favola di ${fav.childName}` : 'Favola Magica');
}

export function exportFavorites() {
    if (!appState.favorites.length) { showToast('Non ci sono ancora favole da esportare.', 'info'); return; }
    const blob = new Blob([JSON.stringify(appState.favorites, null, 2)], { type: 'application/json' });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = `favole-piccolo-bardo-${new Date().toISOString().slice(0, 10)}.json`;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

export async function importFavorites(input) {
    const file = input.files[0];
    input.value = '';
    if (!file) return;
    try {
        if (file.size > 5 * 1024 * 1024) throw new Error('file troppo grande');
        const data = JSON.parse(await file.text());
        if (!Array.isArray(data)) throw new Error('formato non valido');
        const next = appState.favorites.slice();
        const known = new Set(appState.favorites.map(f => f.timestamp));
        let added = 0;
        for (const f of data) {
            if (!f || typeof f.text !== 'string' || typeof f.timestamp !== 'string' || known.has(f.timestamp)) continue;
            next.push({
                text: f.text.slice(0, 20000),
                timestamp: f.timestamp,
                age: Number.isFinite(Number(f.age)) ? Number(f.age) : null,
                childName: typeof f.childName === 'string' ? f.childName.slice(0, 60) : null,
                title: typeof f.title === 'string' ? f.title.slice(0, 120) : undefined,
                prompt: typeof f.prompt === 'string' ? f.prompt.slice(0, 8000) : '',
                favoriteAnimal: typeof f.favoriteAnimal === 'string' ? f.favoriteAnimal.slice(0, 120) : null,
                favoriteColor: typeof f.favoriteColor === 'string' ? f.favoriteColor.slice(0, 120) : null,
                ...normalizeStoryChoices(f),
            });
            known.add(f.timestamp);
            added++;
        }
        if (!saveFavorites(next, true)) return;
        displayFavorites();
        showToast(added ? `Importate ${added} favole.` : 'Nessuna favola nuova da importare.', added ? 'success' : 'info');
    } catch (e) {
        showToast('Non riesco a leggere questo file: ' + e.message, 'error');
    }
}

export function updateFavoriteButton() {
    const btn = document.getElementById('favoriteBtn');
    if (!btn) return;

    const isFavorite = appState.favorites.some(f => f.timestamp === appState.currentStory.timestamp);
    btn.textContent = isFavorite ? '❤️' : '🤍';
    btn.classList.toggle('active', isFavorite);
    btn.setAttribute('aria-pressed', String(isFavorite));
    btn.setAttribute('aria-label', isFavorite ? 'Rimuovi dai preferiti' : 'Aggiungi ai preferiti');
}

export function displayFavorites() {
    const container = document.getElementById('favoritesContainer');

    if (appState.favorites.length === 0) {
        container.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-icon">📚</div>
                <p>Ancora nessuna favola preferita</p>
                <p style="font-size: 0.9em;">Crea una favola e mettile un cuore per salvarla qui!</p>
            </div>
        `;
        return;
    }

    const tools = document.getElementById('favoritesTools');
    tools.hidden = appState.favorites.length < 4;
    const query = tools.hidden ? '' : document.getElementById('favoritesSearch').value.trim().toLowerCase();
    const sort = tools.hidden ? 'recent' : document.getElementById('favoritesSort').value;
    let list = appState.favorites.map((fav, index) => ({ fav, index }));
    if (query) list = list.filter(({ fav }) => `${favoriteTitle(fav)} ${fav.text}`.toLowerCase().includes(query));
    if (sort === 'old') list.sort((a, b) => a.fav.timestamp.localeCompare(b.fav.timestamp));
    else if (sort === 'title') list.sort((a, b) => favoriteTitle(a.fav).localeCompare(favoriteTitle(b.fav), 'it'));
    else list.sort((a, b) => b.fav.timestamp.localeCompare(a.fav.timestamp));

    container.innerHTML = '';
    if (!list.length) {
        container.innerHTML = '<div class="empty-state"><p>Nessuna favola corrisponde alla ricerca.</p></div>';
        return;
    }
    list.forEach(({ fav, index }) => {
        const card = document.createElement('div');
        card.className = 'favorite-card';
        card.setAttribute('role', 'button');
        card.tabIndex = 0;
        card.innerHTML = `
            <button class="delete-btn" aria-label="Elimina questa favola" data-on-click="deleteFavorite" data-arg="${index}">×</button>
            <h3>${escapeHTML(favoriteTitle(fav))}</h3>
            <p class="favorite-meta" style="font-size: 0.85em; margin-bottom: 10px;">
                📅 ${new Date(fav.timestamp).toLocaleDateString('it-IT')} |
                🎂 ${escapeHTML(fav.age)} anni
            </p>
            <p>${escapeHTML(fav.text.substring(0, 150))}...</p>
        `;
        card.onclick = (e) => { if (!e.target.closest('.delete-btn')) loadFavorite(index); };
        card.addEventListener('keydown', (e) => {
            if (e.target === card && (e.key === 'Enter' || e.key === ' ')) { e.preventDefault(); loadFavorite(index); }
        });
        container.appendChild(card);
    });
}

export function loadFavorite(index) {
    if (appState.isGenerating) {
        showToast('Attendi che la favola sia pronta prima di aprirne un\'altra.', 'info');
        return;
    }
    stopReading();
    appState.currentStory = appState.favorites[index];
    switchTab('create');

    document.getElementById('storyCard').style.display = 'block';
    displayStory(appState.currentStory.text);

    document.getElementById('storyCard').scrollIntoView({ behavior: 'smooth' });
}

export async function deleteFavorite(index) {
    const target = appState.favorites[index];
    if (await confirmDialog('Vuoi davvero eliminare questa favola dai preferiti?', { title: 'Elimina favola', confirmText: 'Elimina', danger: true })) {
        if (!saveFavorites(appState.favorites.filter(fav => fav !== target))) return;
        displayFavorites();
        if (appState.currentStory) updateFavoriteButton();
    }
}
