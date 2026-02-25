import 'dotenv/config';
import { createRequire } from 'module';
import path from 'path';

// Inizializziamo require per caricare il binario .node
const require = createRequire(import.meta.url);

// Percorso assoluto basato sulla root del container (/app)
const SOLAR_PATH = '/app/server/native/build/Release/solar.node';

let solar;
try {
    solar = require(SOLAR_PATH);
    console.log("✅ Modulo C++ Solar caricato correttamente");
} catch (err) {
    console.error("❌ Errore critico nel caricamento del modulo Solar:", err.message);
    process.exit(1);
}

export default {
    async exe(server, ws, message) {
        const doc = message.doc;
        
        switch (doc) {
            case "command": {
                const command = `<data azione="command" data="${message.data}" />`;

                if (!ws.clientData) {
                    console.error("Client non trovato");
                    return;
                }

                // Chiamata al core C++
                try {

                    const responseXml = solar.handleClient(ws.clientData.id, command);
                    
                    ws.send(JSON.stringify({
                        tipo: "command",
                        xml: responseXml
                    }));
                } catch (e) {
                    console.error("Errore durante l'esecuzione C++ handleClient:", e);
                }
                break;
            }
            case "init": {
                ws.send(JSON.stringify({ tipo: "init" }));
                break;
            }
        }
    }
};
