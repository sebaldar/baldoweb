// Notifiche non bloccanti al posto di window.alert(). Uso: showToast('Testo', 'error'|'warning'|'success'|'info', ms)
function showToast(message, type = 'info', duration = 5000) {
    if (!document.getElementById('toast-style')) {
        const style = document.createElement('style');
        style.id = 'toast-style';
        style.textContent = `
            #toastContainer { position: fixed; top: calc(20px + env(safe-area-inset-top)); left: 50%; transform: translateX(-50%); z-index: 3000;
                display: flex; flex-direction: column; gap: 10px; width: min(92vw, 460px); pointer-events: none; }
            .toast { display: flex; align-items: flex-start; gap: 12px; padding: 14px 16px; border-radius: 14px;
                background: #fff; color: #2c3e50; border-left: 6px solid #667eea; box-shadow: 0 10px 30px rgba(0,0,0,.3);
                font: inherit; font-size: 1.05em; line-height: 1.4; pointer-events: auto; animation: toastIn .3s ease; }
            .toast.toast-error { border-left-color: #f44336; }
            .toast.toast-warning { border-left-color: #ff9800; }
            .toast.toast-success { border-left-color: #4caf50; }
            .toast.toast-hide { opacity: 0; transform: translateY(-10px); transition: all .25s ease; }
            .toast-msg { flex: 1; }
            .toast .toast-close { all: unset; cursor: pointer; padding: 0 6px; font-size: 1.4em; line-height: 1; opacity: .6; }
            .toast .toast-close:hover { opacity: 1; }
            .toast .toast-close:focus-visible { outline: 3px solid #f39c12; border-radius: 4px; }
            body.nightmode .toast { background: #2c3e50; color: #fff; }
            @keyframes toastIn { from { opacity: 0; transform: translateY(-20px); } to { opacity: 1; transform: translateY(0); } }
            @media (prefers-reduced-motion: reduce) { .toast { animation: none; } .toast.toast-hide { transition: none; } }
        `;
        document.head.appendChild(style);
    }

    let container = document.getElementById('toastContainer');
    if (!container) {
        container = document.createElement('div');
        container.id = 'toastContainer';
        container.setAttribute('role', 'status');
        container.setAttribute('aria-live', 'polite');
        document.body.appendChild(container);
    }

    const toast = document.createElement('div');
    toast.className = 'toast toast-' + type;
    if (type === 'error') toast.setAttribute('role', 'alert');

    const msg = document.createElement('span');
    msg.className = 'toast-msg';
    msg.textContent = message;

    const close = document.createElement('button');
    close.type = 'button';
    close.className = 'toast-close';
    close.setAttribute('aria-label', 'Chiudi messaggio');
    close.textContent = '×';

    const dismiss = () => {
        toast.classList.add('toast-hide');
        setTimeout(() => toast.remove(), 250);
    };
    close.addEventListener('click', dismiss);
    toast.append(msg, close);
    container.appendChild(toast);
    if (duration > 0) setTimeout(dismiss, duration);
}
