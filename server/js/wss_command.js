import fs from 'fs';
import path from 'path';

class WSS {
    constructor(server, ws, data) {
        this.server = server;
        this.ws = ws;
        this.data = data;
    }

    async executeRequest() {
        const server = this.server;
        const ws = this.ws;
        const jdata = this.data;
        const pagina = jdata.message?.pagina; // Uso optional chaining per sicurezza

        if (!pagina) {
            console.error("❌ Errore: Pagina non specificata nel messaggio");
            return;
        }

        // Funzione helper migliorata
        const load = async (pagePath) => {
            // Usiamo un percorso assoluto basato sulla struttura del Docker (/app/server/js/pagine/)
            const fullPath = `file://${path.resolve('/app/server/js/pagine', `${pagePath}.js`)}`;
            
            try {
                const mod = await import(fullPath);
                return mod.default || mod;
            } catch (err) {
                console.error(`❌ Impossibile caricare il modulo in: ${fullPath}`, err.message);
                throw err;
            }
        };

        switch (pagina) {
            case "IL_PICCOLO_BARDO": {
                const module = await load("IL_PICCOLO_BARDO/main");
                // Assicurati di passare 'server' se il modulo ne ha bisogno
                await module.exe(server, ws, jdata.message);
            }
            break;

            case "PLANETARIUM": {
                const module = await load("PLANETARIUM/planetarium");
                // Qui passerà il comando al file che carica solar.node
                await module.exe(server, ws, jdata.message);
            }
            break;
            
            default:
                console.warn(`⚠️ Pagina non riconosciuta: ${pagina}`);
        }
    }
}

export default {
    // Rendiamo asincrono anche l'export per coerenza
    async exe(server, ws, jdata) {
        try {
            const h = new WSS(server, ws, jdata);
            await h.executeRequest();
        } catch (e) {
            console.error("💥 Errore fatale in WSS executeRequest:", e);
        }
    }
};
