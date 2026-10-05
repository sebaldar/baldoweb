// Generazione opzionale e visualizzazione delle illustrazioni.
import { appState } from './state.js';
import { sendMessage } from './connection.js';

// Generazione immagini disattivata in demo: costo per chiamata troppo
// alto per un pulsante libero senza autenticazione. Riattivare quando
// sarà disponibile il login (vedi generateIllustration più sotto).
export const ILLUSTRAZIONE_DISPONIBILE = false;

// Illustration Generation
export function generateIllustration() {
    if (!ILLUSTRAZIONE_DISPONIBILE) {
        showToast('La creazione di illustrazioni sarà disponibile per gli utenti registrati. Per ora è disattivata in questa versione demo.', 'info', 7000);
        return;
    }

    if (!appState.currentStory || !appState.currentStory.text) return;

    sendMessage('generate_illustration', {
        text: appState.currentStory.text,
        storyId: appState.currentStory.timestamp
    });

    document.getElementById('illustrateBtn').disabled = true;
    document.getElementById('illustrateBtn').textContent = '⏳ Creando...';
}

export function displayIllustration(imageUrl) {
    const img = document.getElementById('storyIllustration');
    img.src = imageUrl;
    img.style.display = 'block';
    img.onload = () => {
        img.scrollIntoView({ behavior: 'smooth', block: 'center' });
    };

    document.getElementById('illustrateBtn').disabled = false;
    document.getElementById('illustrateBtn').textContent = '🎨 Crea Illustrazione';

    if (appState.currentStory) {
        appState.currentStory.illustration = imageUrl;
    }
}
