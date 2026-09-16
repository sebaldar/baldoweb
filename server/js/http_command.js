import fs from 'fs';
import path from 'path';

import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const SOLAR_PATH = '/app/server/native/build/Release/solar.node';


let solar;
try {
    solar = require(SOLAR_PATH);
    console.log("✅ Modulo C++ Solar caricato correttamente");
} catch (err) {
    console.error("❌ Errore critico modulo Solar:", err.message);
}

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
            case "ILPICCOLOBARDO": {
                const module = await load("ILPICCOLOBARDO/loader");
                await module.exe( jdata);
            }
            break;
            case "DATI_ASTRONOMICI": {
				
				const id = 100;

				// Dati per il calcolo
				const config = {
					lookfrom: "earth",
					latitude: jdata.query.lat,
					longitude: jdata.query.lon,
					height: 0,
					azimut: 0,
					date: `${jdata.query.data} ${jdata.query.ora}`
				};
				
				solar.registerClient(id, "{}");
				const res_raw = solar.computeCelestialPositions(id, JSON.stringify(config)); 
				solar.unregisterClient(id);

				// 1. Pulizia NaN e Parsing
				const res_cleaned = res_raw.replace(/-?nan/g, "null");
				let raw_json;
				try {
					raw_json = JSON.parse(res_cleaned);
				} catch (e) {
					raw_json = { bodies: [] };
				}
				
				
				const response = jdata.response;
				response.statusCode = 200;
				response.setHeader("Content-Type", "application/json");
						
				response.end( res_cleaned) ;
			}
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
