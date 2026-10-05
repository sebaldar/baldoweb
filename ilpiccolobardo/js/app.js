// Avvio del frontend e collegamento dei comandi della pagina.
import { connect } from './connection.js';
import { deleteFavorite, displayFavorites, exportFavorites, forgetProfile, importFavorites, loadFavorites, loadProfile, toggleFavorite } from './favorites.js';
import { changeTextSize, exitReadingMode, listenStory, loadTextSize, stopReading, togglePause, toggleReadingMode } from './reading.js';
import { ILLUSTRAZIONE_DISPONIBILE, generateIllustration } from './illustrations.js';
import { closeAccountModal, login, logout, openAccountModal, updateAccountButton } from './account.js';
import { appState } from './state.js';
import { installApp } from './pwa.js';
import { switchTab, toggleNightMode } from './ui.js';
import { generateStory, newStory, pickChip, regenerateStory, selectAge } from './generation.js';
import { printStory } from './pdf.js';
import { bindActions } from './actions.js';

bindActions({
    openAccountModal, installApp, toggleNightMode, exitReadingMode, switchTab, forgetProfile, selectAge, pickChip,
    generateStory, changeTextSize, toggleReadingMode, toggleFavorite, listenStory, generateIllustration, printStory,
    togglePause, stopReading, regenerateStory, newStory, displayFavorites, exportFavorites, closeAccountModal,
    deleteFavorite, login, logout,
    importFavorites: (arg, input) => importFavorites(input),
    openImportPicker: () => document.getElementById('importFavorites').click(),
});

// Initialize
window.addEventListener('load', () => {
    connect();
    loadFavorites();
    loadProfile();
    loadTextSize();
    document.getElementById('illustrateBtn').hidden = !ILLUSTRAZIONE_DISPONIBILE;
    updateAccountButton(); // iniziale

    // Carica voci per TTS in anticipo (importante per la fluidità)
    appState.speechSynthesis?.getVoices();

    if (appState.speechSynthesis && appState.speechSynthesis.onvoiceschanged !== undefined) {
        appState.speechSynthesis.onvoiceschanged = () => {
            const voices = appState.speechSynthesis.getVoices();
            console.log('🎤 Voci disponibili:', voices.length);

            // Log delle voci italiane per debug
            const italianVoices = voices.filter(v => v.lang.startsWith('it'));
            console.log('🇮🇹 Voci italiane:', italianVoices.map(v => `${v.name} (${v.lang})`));
        };
    }
});
