/* The administrator key stays in memory, never in URLs or browser storage. */
window.AdminAccess = (() => {
    let token = null;
    let pending = null;
    async function getToken() {
        if (token) return token;
        if (pending) return pending;
        pending = new Promise(resolve => {
            const dialog = document.createElement('dialog');
            dialog.innerHTML = '<form method="dialog"><h2>Accesso amministratore</h2>' +
                '<label>Chiave amministratore <input type="password" required autocomplete="off"></label>' +
                '<p><button value="login">Accedi</button> <button value="cancel" formnovalidate>Annulla</button></p></form>';
            document.body.appendChild(dialog);
            dialog.addEventListener('close', () => {
                token = dialog.returnValue === 'login' ? dialog.querySelector('input').value : null;
                dialog.remove();
                pending = null;
                resolve(token);
            }, { once: true });
            dialog.showModal();
        });
        return pending;
    }
    return {
        getToken,
        reset() { token = null; },
        async fetch(url, options = {}) {
            const key = await getToken();
            if (!key) throw new Error('Accesso amministratore annullato');
            const headers = new Headers(options.headers);
            headers.set('Authorization', `Bearer ${key}`);
            const response = await fetch(url, { ...options, headers });
            if (response.status === 401 || response.status === 403) token = null;
            return response;
        },
    };
})();
