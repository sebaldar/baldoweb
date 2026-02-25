// index.js
// Carica le variabili d'ambiente (dal file .env)
import * as dotenv from 'dotenv';
dotenv.config();

import fs from 'fs'; 
import os from 'os';
import Server from './server_base.js'; 
import wss_command from './js/wss_command.js'; 
import http_command from './js/http_command.js'; 
import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const SOLAR_PATH = '/app/server/native/build/Release/solar.node';

let solar;
try {
    solar = require(SOLAR_PATH);
    console.log("✅ Modulo C++ Solar caricato correttamente");
} catch (err) {
    console.error("❌ Errore critico nel caricamento del modulo Solar:", err.message);
    process.exit(1);
}

class the_server extends Server {
    constructor(port) {
        super(port);
    }
     
    async http_command(jdata) {
        await http_command.exe(jdata);
        super.http_command(jdata);
    }
     
    wss_command(server, ws, jdata) {
        wss_command.exe(server, ws, jdata);
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
        
        const sec_websocket_key = request.headers['sec-websocket-key'];
        server.CLIENTS[sec_websocket_key] = ws.clientData;
          
        const refresh = 500;
        ws.clientData.intervalId = setInterval(() => {
            if (!ws.clientData || ws.readyState !== ws.OPEN) {
                clearInterval(ws.clientData?.intervalId);
                return;
            }
            
            try {
                const command = solar.do_sendLoop(id);
                ws.send(command);
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
