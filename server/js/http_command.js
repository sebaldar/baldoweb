import fs from 'fs';

// Import dinamici: li faremo al bisogno nel metodo executeRequest.
// Ogni modulo lo importeremo con import().

// Classe WSS
class HTTP {

    constructor( jdata ) {
        this.jdata = jdata;
    }

    async executeRequest() {

        const jdata = this.jdata;

        // Funzione helper per importare moduli ES dinamicamente
        const load = async (path) => {
            const mod = await import(`file://${process.cwd()}/js/pagine/${path}.js`);
            return mod.default || mod; // supporta sia export default che named
        };

        const doc = jdata.doc;
        switch (doc) {
			case "session" :
				this.jdata.response.statusCode = 200;
				this.jdata.response.setHeader("Content-Type", `text/json`);	
				this.jdata.response.end( JSON.stringify( {'session' : jdata.SESSION } ))
			break;
			default:
				this.jdata.response.statusCode = 200;
				this.jdata.response.setHeader("Content-Type", `text/html`);	
				this.jdata.response.end("Nessuna richiesta gestita!")
			break;
			

        }
    }
}


// Esportazione del modulo principale
export default {
    async exe( jdata ) {
        const h = new HTTP(jdata);
        await h.executeRequest();
    }
};
