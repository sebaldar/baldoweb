// Pannello account e informazioni nel footer.
import { appState } from './state.js';

// Nuove funzioni per account
export function openAccountModal() {
    const modal = document.getElementById('accountModal');
    const title = document.getElementById('modalTitle');
    const body = document.getElementById('modalBody');

    if (appState.currentUser) {
        title.textContent = "Impostazioni Account";
        body.innerHTML = `
            <p>Ciao <strong>${escapeHTML(appState.currentUser.name || appState.currentUser.email)}</strong>! 👋</p>
            <hr style="margin: 20px 0; border-color: #ddd;">
            <p><strong>Email:</strong> ${escapeHTML(appState.currentUser.email)}</p>
            ${appState.currentUser.childName ? `<p><strong>Bambino:</strong> ${escapeHTML(appState.currentUser.childName)}</p>` : ''}
            <br>
            <button class="btn-danger" onclick="logout()" style="width:100%; padding:15px;">Esci</button>
        `;
    } else {
        title.textContent = "Accedi";
        body.innerHTML = `
            <p>Per salvare le favole preferite nel cloud e sincronizzarle tra dispositivi, accedi al tuo account.</p>
            <br>
            <button class="btn-primary" onclick="login()" style="width:100%; padding:15px;">Accedi / Registrati</button>
            <p style="margin-top:20px; font-size:0.9em; color:#666;">Funzionalità in arrivo!</p>
        `;
    }

    appState.modalOpener = document.activeElement;
    modal.classList.add('active');
    setBackgroundInert(true);
    modal.querySelector('.close-modal').focus();
}

export function setBackgroundInert(on) {
    for (const el of document.body.children) {
        if (['SCRIPT', 'STYLE', 'LINK'].includes(el.tagName) || el.id === 'accountModal' || el.id === 'toastContainer') continue;
        el.inert = on;
    }
}

export function closeAccountModal() {
    document.getElementById('accountModal').classList.remove('active');
    setBackgroundInert(false);
    if (appState.modalOpener && document.contains(appState.modalOpener)) appState.modalOpener.focus();
    appState.modalOpener = null;
}

document.addEventListener('keydown', (e) => {
    const modal = document.getElementById('accountModal');
    if (!modal.classList.contains('active') || document.querySelector('dialog[open]')) return;
    if (e.key === 'Escape') { closeAccountModal(); return; }
    if (e.key !== 'Tab') return;
    const items = [...modal.querySelectorAll('button, a[href], input, select, textarea')].filter(n => !n.disabled);
    if (!items.length) return;
    const first = items[0], last = items[items.length - 1];
    if (e.shiftKey && document.activeElement === first) { e.preventDefault(); last.focus(); }
    else if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus(); }
});

export function login() {
    showToast("Funzionalità di login in arrivo! Per ora le favole sono salvate solo localmente.", 'info', 7000);
    closeAccountModal();
}

export async function logout() {
    if (await confirmDialog("Vuoi uscire dall'account? Le favole preferite rimarranno salvate localmente.", { title: 'Esci', confirmText: 'Esci' })) {
        appState.currentUser = null;
        updateAccountButton();
        closeAccountModal();
    }
}

export function updateAccountButton() {
    const btn = document.getElementById('accountBtn');
    const nameSpan = document.getElementById('accountName');

    if (appState.currentUser) {
        const initial = (appState.currentUser.name || appState.currentUser.email || '').charAt(0).toUpperCase();
        nameSpan.style.display = 'inline';
        nameSpan.textContent = initial;
        btn.setAttribute('aria-label', 'Account: ' + (appState.currentUser.name || appState.currentUser.email || 'utente'));
    } else {
        nameSpan.style.display = 'none';
        nameSpan.textContent = '';
        btn.setAttribute('aria-label', 'Account');
    }
}

// ── Footer: dati aziendali dal server, con copia locale per l'offline ──
export function renderCompanyInfo(data) {
    document.getElementById('companyInfo').innerHTML = `
        <strong>${escapeHTML(data.name)}</strong><br>
        P.IVA: ${escapeHTML(data.vat)}<br>
        ${escapeHTML(data.address)}<br>
        Email: <a href="mailto:${escapeHTML(data.email)}" style="color:#667eea;">${escapeHTML(data.email)}</a><br>
        © ${new Date().getFullYear()} Tutti i diritti riservati.
    `;
}

(function showCachedCompany() {
    try {
        const cached = JSON.parse(localStorage.getItem('bardo_company'));
        if (cached && cached.name) renderCompanyInfo(cached);
    } catch (e) {}
})();
