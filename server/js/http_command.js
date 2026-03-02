import fs from 'fs';
import path from 'path';

// Import dinamici: li faremo al bisogno nel metodo executeRequest.
// Ogni modulo lo importeremo con import().

class HTTP {

    constructor( jdata ) {
        this.jdata = jdata;
        this.response = jdata.response;
    }

    async executeRequest() {

        const jdata = this.jdata;

        // Funzione helper per importare moduli ES dinamicamente
        const load = async (pagePath) => {
            // Usiamo un percorso assoluto basato sulla struttura del Docker (/app/server/js/pagine/)
            const fullPath = `file://${path.resolve('/app/server/js/api', `${pagePath}.js`)}`;
            
            try {
                const mod = await import(fullPath);
                return mod.default || mod;
            } catch (err) {
                console.error(`❌ Impossibile caricare il modulo in: ${fullPath}`, err.message);
                throw err;
            }
        };

        const doc = jdata.doc;
        switch (doc) {
			case "session" :
				this.response.statusCode = 200;
				this.response.setHeader("Content-Type", `text/json`);	
				this.response.end( JSON.stringify( {'session' : jdata.SESSION } ))
			break;
            case "PLANETARIUM": {
                const module = await load("PLANETARIUM/dati_astronomici");
                await module.exe( jdata);
            }
            break;
			default:
				this.response.statusCode = 200;
				this.response.setHeader("Content-Type", `text/html`);	
				this.response.end("Nessuna richiesta gestita!")
			break;
			

        }
    }
}


// Esportazione del modulo principale
export default {
    async exe( response, jdata ) {
        const h = new HTTP( response,jdata);
        await h.executeRequest();
    }
};
