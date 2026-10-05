// Tema e navigazione tra le sezioni.
import { appState } from './state.js';

// Night Mode
export function applyTheme(dark) {
    appState.isNightMode = dark;
    document.body.classList.toggle('nightmode', dark);
    const btn = document.querySelector('.nightmode-toggle');
    btn.textContent = dark ? '☀️' : '🌙';
    btn.setAttribute('aria-pressed', String(dark));
    btn.setAttribute('aria-label', dark ? 'Torna alla modalità giorno' : 'Attiva la modalità buonanotte');
    document.querySelector('meta[name="theme-color"]').setAttribute('content', dark ? '#1a1a2e' : '#667eea');
}

export function toggleNightMode() {
    applyTheme(!appState.isNightMode);
    try { localStorage.setItem('bardo_theme', appState.isNightMode ? 'dark' : 'light'); } catch (e) {}
}

applyTheme(appState.isNightMode);

// Finché l'utente non sceglie, il tema segue quello del dispositivo
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', (e) => {
    let saved = null;
    try { saved = localStorage.getItem('bardo_theme'); } catch (err) {}
    if (!saved) applyTheme(e.matches);
});

// Tab Switching
export function switchTab(tabName) {
    document.querySelectorAll('.tab').forEach(tab => {
        const on = tab.id === `tab-${tabName}`;
        tab.classList.toggle('active', on);
        tab.setAttribute('aria-selected', String(on));
        tab.tabIndex = on ? 0 : -1;
    });
    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
    document.getElementById(`${tabName}-tab`).classList.add('active');
    window.scrollTo({ top: 0 });
}

// Frecce sinistra/destra tra le schede, come da pattern WAI-ARIA
document.querySelector('.tabs').addEventListener('keydown', (e) => {
    if (e.key !== 'ArrowRight' && e.key !== 'ArrowLeft') return;
    const tabs = [...document.querySelectorAll('.tab')];
    const i = tabs.indexOf(document.activeElement);
    if (i < 0) return;
    const next = tabs[(i + (e.key === 'ArrowRight' ? 1 : tabs.length - 1)) % tabs.length];
    next.focus();
    next.click();
});
