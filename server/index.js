// index.js
// Carica le variabili d'ambiente (dal file .env)
import * as dotenv from 'dotenv';
dotenv.config();

import { promises as fs } from 'fs';
import path from 'path';
 
import os from 'os';
import Server from './server_base.js'; 
import wss_command from './js/wss_command.js'; 
import http_command from './js/http_command.js'; 
import { loadSolarModule } from './js/services/native-loader.js';


let solar;
try {
    solar = loadSolarModule();
    console.log("✅ Modulo C++ Solar caricato correttamente");
} catch (err) {
    console.error("❌ Errore critico nel caricamento del modulo Solar:", err.message);
    process.exit(1);
}

class the_server extends Server {
    constructor(port) {
        super(port);
    }
     
    async http_command(  jdata) {
        await http_command.exe( jdata);
        super.http_command( jdata);
    }
     
    async wss_command(server, ws, jdata) {
        await wss_command.exe(server, ws, jdata);
        super.wss_command(server, ws, jdata);
    }
     
    async on_my_ws_connection(server, request, ws, jdata) {
        
	   super.on_my_ws_connection(server,  ws, jdata );
	   const id = server.uid.id();
       ws.clientData = {
            id: id,
            client_ip: jdata.client_ip,
            query: jdata.query,
            intervalId: null  // ← Per cleanup corretto
        };
        
        console.log(`Client ${id} connected from ${jdata.client_ip}`);
        solar.registerClient(id, JSON.stringify(jdata.query));
        
        // registra l'id nella session
        const session_dir = jdata.session_dir;
        const user_file = path.join(session_dir, 'user.json');

		try {
			// 1. Lettura (specificando utf8 per avere una stringa)
			const raw_data = await fs.readFile(user_file, 'utf8');
			const user_data = JSON.parse(raw_data);

			// 2. Modifica sicura
			if (!user_data.data) user_data.data = {};
			user_data.data.planetarium = { id: id };

			// 3. Scrittura (con formattazione per facilitare il debug nel container)
			await fs.writeFile(user_file, JSON.stringify(user_data, null, 2));
			
			console.log(`✅ File sessione aggiornato per ID: ${id}`);
		} catch (err) {
			console.error(`❌ Errore aggiornamento sessione (${user_file}):`, err.message);
		}
        
        const sec_websocket_key = request.headers['sec-websocket-key'];
        // The base class tracks each socket by its unique handshake key.
          
        const refresh = 500;
        ws.clientData.intervalId = setInterval(() => {
            if (!ws.clientData || ws.readyState !== ws.OPEN) {
                clearInterval(ws.clientData?.intervalId);
                return;
            }

            try {
                const command = solar.do_sendLoop(id);
                // do_sendLoop() è agganciato a OGNI connessione WS del server,
                // non solo a quelle del planetario (l'endpoint /ws è condiviso
                // da tutte le pagine, incluso admin/manager.html) — per un
                // client senza una vera sessione astronomica registrata
                // restituisce una stringa vuota ogni 500ms. Inviarla comunque
                // manda un frame WS vuoto a QUALUNQUE pagina connessa, che poi
                // fallisce JSON.parse(event.data) lato client (osservato:
                // "Unexpected end of JSON input" ripetuto ogni 500ms sul
                // pannello admin). Non inviamo nulla se non c'è davvero un
                // comando da mandare.
                if (command) {
                    ws.send(command);
                }
            } catch (err) {
                console.error(`Error sending to client ${id}:`, err.message);
                clearInterval(ws.clientData.intervalId);
            }
        }, refresh);
    }
     
    async on_my_ws_close(server, ws, request) {
        
        if (!ws.clientData) return;
        
		super.on_my_ws_close(server,  ws, request );

        const id = ws.clientData.id;
        
        // Cleanup timer
        if (ws.clientData.intervalId) {
            clearInterval(ws.clientData.intervalId);
        }
        
        // Cleanup modulo C++
        try {
            solar.unregisterClient(id);
        } catch (err) {
            console.error(`Error unregistering client ${id}:`, err.message);
        }
        
        // Cleanup server
        server.uid.del(id);
        
        console.log(`Client ${id} disconnected`);
    }
     
    thread() {
        // Logica per thread
    }
     
    async inizDB() {
        // Logica per inizializzazione DB
    }
}

// Avvio del server con Top-Level await
(async () => {
    try {
        console.log('🚀 Avvio applicazione...');
        
        const server = new the_server(process.env.WSS_PORT);
        
        await server.init();
        
        console.log('✅ Inizializzazione completata');
        
        server.listen();
        server.thread();
        
        console.log(`✅ Server avviato sulla porta ${process.env.WSS_PORT}`);
        
    } catch (error) {
        console.error('❌ ERRORE FATALE:', error);
        process.exit(1);
    }
})();
