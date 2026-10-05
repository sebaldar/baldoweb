// Dialog di conferma al posto di window.confirm(). Uso:
//   if (await confirmDialog('Eliminare?', { danger: true, confirmText: 'Elimina' })) { ... }
function confirmDialog(message, options = {}) {
    const { title = 'Conferma', confirmText = 'Conferma', cancelText = 'Annulla', danger = false } = options;

    if (!document.getElementById('confirm-dialog-style')) {
        const style = document.createElement('style');
        style.id = 'confirm-dialog-style';
        style.textContent = `
            dialog.confirm-dialog { border: none; border-radius: 16px; padding: 24px; width: min(92vw, 420px);
                background: #fff; color: #2c3e50; font-family: inherit; box-shadow: 0 20px 50px rgba(0,0,0,.4); }
            dialog.confirm-dialog::backdrop { background: rgba(0,0,0,.55); }
            body.nightmode dialog.confirm-dialog, body:not(.nightmode) dialog.confirm-dialog.dark { background: #2c3e50; color: #ecf0f1; }
            dialog.confirm-dialog h3 { margin: 0 0 10px; font-size: 1.25em; }
            dialog.confirm-dialog p { margin: 0 0 20px; line-height: 1.5; white-space: pre-line; }
            dialog.confirm-dialog .confirm-actions { display: flex; gap: 10px; justify-content: flex-end; flex-wrap: wrap; }
            dialog.confirm-dialog button { padding: 10px 20px; border-radius: 10px; border: 2px solid transparent;
                font: inherit; font-weight: 700; cursor: pointer; }
            dialog.confirm-dialog .confirm-cancel { background: transparent; color: inherit; border-color: currentColor; opacity: .85; }
            dialog.confirm-dialog .confirm-ok { background: #667eea; color: #fff; }
            dialog.confirm-dialog .confirm-ok.danger { background: #c62828; }
            dialog.confirm-dialog button:focus-visible { outline: 3px solid #f39c12; outline-offset: 2px; }
        `;
        document.head.appendChild(style);
    }

    return new Promise(resolve => {
        const dialog = document.createElement('dialog');
        dialog.className = 'confirm-dialog';
        if (document.body.dataset.confirmDark === 'true') dialog.classList.add('dark');
        dialog.setAttribute('aria-labelledby', 'confirm-dialog-title');

        const heading = document.createElement('h3');
        heading.id = 'confirm-dialog-title';
        heading.textContent = title;

        const text = document.createElement('p');
        text.textContent = message;

        const cancel = document.createElement('button');
        cancel.type = 'button';
        cancel.className = 'confirm-cancel';
        cancel.textContent = cancelText;

        const ok = document.createElement('button');
        ok.type = 'button';
        ok.className = 'confirm-ok' + (danger ? ' danger' : '');
        ok.textContent = confirmText;

        const actions = document.createElement('div');
        actions.className = 'confirm-actions';
        actions.append(cancel, ok);
        dialog.append(heading, text, actions);

        let result = false;
        ok.addEventListener('click', () => { result = true; dialog.close(); });
        cancel.addEventListener('click', () => dialog.close());
        dialog.addEventListener('click', e => { if (e.target === dialog) dialog.close(); });
        dialog.addEventListener('close', () => { dialog.remove(); resolve(result); });

        document.body.appendChild(dialog);
        dialog.showModal();
        (danger ? cancel : ok).focus();
    });
}
