// Comandi dichiarativi: gli elementi indicano l'azione con data-on-click,
// data-on-change o data-on-input (più un eventuale data-arg) e un unico
// listener sul documento li smista. Funziona anche per l'HTML creato da JS.
const EVENTS = ['click', 'change', 'input'];

function parseArg(value) {
    if (value === undefined) return undefined;
    return value !== '' && !Number.isNaN(Number(value)) ? Number(value) : value;
}

export function bindActions(actions) {
    for (const type of EVENTS) {
        const attribute = `data-on-${type}`;
        document.addEventListener(type, event => {
            const element = event.target?.closest?.(`[${attribute}]`);
            if (!element) return;
            const handler = actions[element.getAttribute(attribute)];
            if (typeof handler !== 'function') {
                console.warn(`Azione sconosciuta: ${element.getAttribute(attribute)}`);
                return;
            }
            handler(parseArg(element.dataset.arg), element, event);
        });
    }
}
