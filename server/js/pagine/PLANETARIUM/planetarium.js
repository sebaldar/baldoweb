import 'dotenv/config';
import { loadSolarModule } from '../../services/native-loader.js';

// Inizializziamo require per caricare il binario .node

// Percorso assoluto basato sulla root del container (/app)

let solar;
try {
    solar = loadSolarModule();
    console.log("✅ Modulo C++ Solar caricato correttamente");
} catch (err) {
    console.error("❌ Errore critico nel caricamento del modulo Solar:", err.message);
    process.exit(1);
}


// ── Utility ──────────────────────────────────────────────────
const safeSend = (ws, payload) => {
    if (ws.readyState === ws.OPEN) {
        ws.send(JSON.stringify(payload));
    }
};

// ── Streaming NDJSON/SSE helper ──────────────────────────────
// Legge il ReadableStream di FastAPI riga per riga e ritrasmet-
// te ogni evento JSON al client WebSocket. La history è gestita
// interamente da LangGraph (MemorySaver) tramite session_id:
// Node fa solo da proxy trasparente.
async function streamChatbot(ws, sessionId, payload) {
    
    const sessionTag = (sessionId || 'no-session').slice(0, 8);
	
	const response = await fetch('http://baldo-fastapi-planetarium:8000/api/chat', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify(payload),
    });

    if (!response.ok)   throw new Error(`FastAPI HTTP ${response.status}`);
    if (!response.body) throw new Error('Response body assente');

    const contentType = response.headers.get('content-type') || '';
    console.log(`[SSE] Inizio streaming session=${sessionTag}… (${contentType})`);

    const reader  = response.body.getReader();
    const decoder = new TextDecoder();
    let buffer    = '';

    // ── Processa una singola riga JSON ────────────────────────
    function processLine(raw) {
        
        const trimmed = raw.trim();
        if (!trimmed || trimmed === '[DONE]') return;

        // Supporta sia NDJSON grezzo che SSE "data: {...}"
        const jsonStr = trimmed.startsWith('data: ')
            ? trimmed.slice(6).trim()
            : trimmed;

        if (!jsonStr || jsonStr === '[DONE]') return;

        let evt;
        try {
            evt = JSON.parse(jsonStr);
        } catch {
            console.warn(`[NDJSON] Riga non parsabile: ${jsonStr.slice(0, 80)}`);
            return;
        }

        console.log(`[NDJSON] → tipo=${evt.tipo} session=${sessionTag}…`);
        
        evt.action = "chat_result";
        if ( evt.tipo=="final"  ) console.log(evt.extra);
        if ( evt.tipo=="cmd"  ) console.log(evt.cmd);
        if (evt.tipo !== "token" )
			safeSend(ws, evt);

    }

    // ── Loop principale lettura stream ────────────────────────
    while (true) {
        const { done, value } = await reader.read();
        if (done) {
            console.log(`[SSE] Fine stream session=${sessionTag}…`);
            break;
        }

        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop() || '';   // ultima riga potenzialmente incompleta

        for (const line of lines) {
            processLine(line);
        }
    }

    // ── Flush buffer residuo (riga finale senza \n) ───────────
    if (buffer.trim()) {
        processLine(buffer);
    }
}


export default {
    async exe(server, ws, message) {
        const doc = message.doc ? message.doc : message.action;

        switch (doc) {

            case 'command': {
                if (!ws.clientData) {
                    console.error('Client non trovato');
                    return;
                }
                const command = `<data azione="command" data="${message.data}" />`;
                try {
                    const responseXml = solar.handleClient(ws.clientData.id, command);
                    ws.send(JSON.stringify({ tipo: 'command', xml: responseXml }));
                } catch (e) {
                    console.error('Errore durante l\'esecuzione C++ handleClient:', e);
                }
                break;
            }

            case 'init': {
                ws.send(JSON.stringify({ tipo: 'init' }));
                break;
            }

            case 'CHATBOT': {
                
                const sessionId = message.session || null;
                

				const date = new Date(message.data);
				// Estraiamo la data (aggiungendo lo 0 davanti se serve)
				const day = String(date.getDate()).padStart(2, '0');
				const month = String(date.getMonth() + 1).padStart(2, '0'); // +1 perché i mesi partono da 0
				const year = date.getFullYear();

				// Estraiamo l'orario
				const hours = String(date.getHours()).padStart(2, '0');
				const minutes = String(date.getMinutes()).padStart(2, '0');
				const seconds = String(date.getSeconds()).padStart(2, '0');

				// Assembliamo il tutto nel formato richiesto: dd-mm-yyyy hh:mm:ss
				const formattedDate = `${day}-${month}-${year} ${hours}:${minutes}:${seconds}`;				
				
				// Dati astronomici in quel dato istante
				const config = {
					lookfrom: "earth",
					latitude: message.lat,
					longitude: message.lon ,
					height: 0,
					azimut: 0,
					date: formattedDate
				};
				
				
				const id = 100;
				solar.registerClient(id, "{}");
				const res_raw = solar.computeCelestialPositions(id, JSON.stringify(config)); 
				solar.unregisterClient(id);
				

				// 1. Pulizia NaN e Parsing
				const res_cleaned = res_raw.replace(/-?nan/g, "null");
                
                // Payload verso FastAPI — la history è gestita da LangGraph
                // tramite session_id usato come thread_id in MemorySaver
                const payload = {
                    prompt:     message.text,
                    provider:   message.provider  || 'ionos',
                    data:       message.data       || null,
                    session_id: sessionId,
                    lat:        message.lat        || null,
                    lon:        message.lon        || null,
                    motore_astronomico : res_cleaned
                };

                try {
                    await streamChatbot(ws, sessionId, payload);
                } catch (err) {
                    console.error(`[WS] errore fetch SSE: ${err.message}`);

                    safeSend(ws, {
						action : "chat_error",
                        tipo:    'chat_result',
                        status:  'error',
                        message: 'Il backend AI non è raggiungibile. Riprova tra un momento.',
                    });
                }
                break;
            }
        }
    }
};
