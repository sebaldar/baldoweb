// server_base.js (o server_base.mjs)

import 'dotenv/config';
import { promises as fs } from 'fs';
import https from 'https';
import http from 'http'   

import { URL } from 'url';
import path from 'path';
import { WebSocketServer } from 'ws';
import crypto from 'crypto';
import os from 'os';

//import MongoService from './js/services/service.mongo.js';
const MongoService = undefined;

import MySQL2ServiceSingleton, { MySQL2Service } from './js/services/service.mariadb.js';
import neo4jService from './js/services/service.neo4j.js';


// Espandi le variabili d'ambiente che contengono $HOME
const BASE_ROOT = process.env.BASE_ROOT;
const DOC_ROOT = process.env.DOC_ROOT;
const SERVER_ROOT = process.env.SERVER_ROOT;

const INIZ = {
    DIR_SERVER_CLASS: './classes',
    SSL: { 
        DIR: process.env.SSL_DIR, 
        cert: process.env.SSL_CERT, 
        key: process.env.SSL_KEY, 
        ca: process.env.SSL_CA, 
        passphrase: process.env.SSL_PASSPHRASE 
    },
    CONFIG: {
        base_root: BASE_ROOT,
        doc_root: DOC_ROOT,
        session_dir: process.env.SESSION_DIR,
        server_root: SERVER_ROOT 
    }
};

const tools = {
    getContent: async (file) => { /* LOGICA ASINCRONA */ return fs.readFile(file, 'utf8') },
    parseCookies: (cookieHeader) => { 
        // Logica semplificata per la demo
        if (!cookieHeader) return {};
        const cookies = {};
        cookieHeader.split(';').forEach(cookie => {
            const parts = cookie.trim().split('=');
            cookies[parts[0]] = parts[1];
        });
        return cookies;
    },
    setLog: (msg) => { console.log(`[LOG] ${msg}`) }
};

const manage_post = {
    exe: (request, binary) => {
        console.log('ATTENZIONE: parsing multipart/form-data delegato, implementare con libreria!');
        return {}; 
    }
};
// -------------------------------------------------------------------
// Crea le istanze multiple
const dbUser = new MySQL2Service({
    host: process.env.DB_HOST ,
    user: process.env.DB_USER,
    password: process.env.DB_PASS,
    database: process.env.DB_NAME 
});


// Array per tracciare tutte le connessioni
const mysqlConnections = [MySQL2ServiceSingleton, dbUser];

// Export per usarle altrove
export { dbUser };


// Variabile per tracciare se la chiusura è già in corso
let isShuttingDown = false;

async function handleSignal(signal) {
    if (isShuttingDown) {
        console.log('⚠️ Chiusura già in corso...');
        return;
    }
    
    isShuttingDown = true;
    console.log(`\n🛑 Ricevuto segnale ${signal}. Chiusura server in corso...`);
    
    const shutdownPromises = [];
    
    // Chiude MongoDB
    if (MongoService) {
        shutdownPromises.push(
            MongoService.close()
                .then(() => console.log("✅ MongoDB chiuso correttamente"))
                .catch(err => console.error("❌ Errore chiusura MongoDB:", err.message))
        );
    }
    
    // Chiude tutte le connessioni MySQL/MariaDB
    mysqlConnections.forEach((mysqlInstance, index) => {
        if (mysqlInstance && mysqlInstance.isConnected()) {
            shutdownPromises.push(
                mysqlInstance.close()
                    .then(() => console.log(`✅ MySQL/MariaDB ${index > 0 ? `#${index + 1}` : ''} chiuso correttamente`))
                    .catch(err => console.error(`❌ Errore chiusura MySQL/MariaDB ${index > 0 ? `#${index + 1}` : ''}:`, err.message))
            );
        }
    });
    
    // Chiude Neo4j
    if (neo4jService) {
        shutdownPromises.push(
            neo4jService.close()
                .then(() => console.log("✅ Neo4j chiuso correttamente"))
                .catch(err => console.error("❌ Errore chiusura Neo4j:", err.message))
        );
    }
    
    // Aspetta che tutte le connessioni vengano chiuse
    try {
        await Promise.allSettled(shutdownPromises);
        console.log("✅ Tutte le connessioni chiuse");
    } catch (err) {
        console.error("❌ Errore durante la chiusura:", err.message);
    }
    
    // Timeout di sicurezza: forza l'uscita dopo 10 secondi
    setTimeout(() => {
        console.log("⏱️ Timeout scaduto, uscita forzata");
        process.exit(1);
    }, 10000);
    

    process.exit(0);
}

// Gestione segnali
process.on('SIGINT', () => handleSignal('SIGINT'));
process.on('SIGTERM', () => handleSignal('SIGTERM'));

// Gestione errori non catturati (opzionale ma consigliato)
process.on('uncaughtException', (err) => {
    console.error('❌ Errore non catturato:', err);
    handleSignal('uncaughtException');
});

process.on('unhandledRejection', (reason, promise) => {
    console.error('❌ Promise rejection non gestita:', reason);
    handleSignal('unhandledRejection');
});

process.on('SIGINT', () => handleSignal('SIGINT'));
process.on('SIGTERM', () => handleSignal('SIGTERM'));

class UIDManager {
    constructor() {
        this.used = new Set();
        this.counter = 0;
    }
    id() {
        while (this.used.has(this.counter)) this.counter++;
        const id = this.counter++;
        this.used.add(id);
        return id;
    }
    del(id) {
        this.used.delete(id);
    }
}

export default class Server { // Export della classe (default)

    constructor(port) {
        this.host = '0.0.0.0';
        this.port = port;
        this.CLIENTS = {};
        this.srv_https = null; 
        this.wss = null;
        this.PING_INTERVAL = 30000; // Intervallo Ping Heartbeat (30 secondi)
        this.interval = null;
        this.uid = new UIDManager();
    }

    /**
     * @static Genera un ID di sessione crittograficamente sicuro.
     */
    static generateSessionId() {
        return crypto.randomBytes(16).toString('hex');
    }

    /**
     * Verifica se una stringa è un Session ID valido nel formato aspettato (32 caratteri hex).
     * @param {string} sessionId L'ID di sessione da verificare.
     * @returns {boolean} True se l'ID è nel formato corretto.
     */
     isValidSessionIdFormat(sessionId) {
        // Il pattern: ^ (inizio stringa), [0-9a-fA-F] (caratteri esadecimali), {32} (esattamente 32 volte), $ (fine stringa)
        const hex32Pattern = /^[0-9a-fA-F]{32}$/;

        // 1. Verifica che non sia null/undefined e che sia una stringa
        if (typeof sessionId !== 'string' || !sessionId) {
            return false;
        }

        // 2. Verifica tramite espressione regolare
        return hex32Pattern.test(sessionId);
    }
    
    /**
     * @static Gestisce la risposta Pong del client.
     */
    static pongHandler() {
        this.isAlive = true;
    }

    
    // Nel metodo init()
    async init() {
        const ssl_dir = INIZ.SSL.DIR;
        tools.setLog('Inizializzazione server e lettura chiavi SSL...');
        
        try {
            // --- 🔌 CONNESSIONE MONGODB ---
            if ( MongoService ) {
				await MongoService.connect(
					"mongodb://127.0.0.1:27017",
					"gestiondb"
				);
				tools.setLog("✅ MongoDB inizializzato");
			}
            
            // --- 🔌 CONNESSIONE MARIADB (TUTTE LE ISTANZE) ---
            const mysqlPromises = mysqlConnections.map(async (db, index) => {
                try {
                    await db.connect();
                    const name = index === 0 ? 'principale' : `#${index + 1}`;
                    tools.setLog(`✅ MariaDB ${name} inizializzato`);
                } catch (error) {
                    console.error(`❌ Errore connessione MariaDB ${index}:`, error.message);
                    throw error;
                }
            });
            
            await Promise.all(mysqlPromises);
            
            // --- 🔌 CONNESSIONE NEO4J ---
            try {
              // Connessione con retry automatico
              await neo4jService.connect(
                process.env.NEO4J_URI,
                process.env.NEO4J_USER,
                process.env.NEO4J_PASSWORD,
                process.env.NEO4J_DEFAULT_DB
              );
              tools.setLog("✅ Neo4j inizializzato");
              
              // Health check completo
              const health = await neo4jService.healthCheck();
              if (health.healthy) {
                tools.setLog("✅ Neo4j health check OK");
              } else {
                tools.setLog(`⚠️ Neo4j warning: ${health.error}`);
              }
              
              // Mostra info connessione
              const info = neo4jService.getConnectionInfo();
              tools.setLog(`📊 Database: ${info.database} @ ${info.uri}`);
              
            } catch (error) {
              tools.setLog(`❌ Errore Neo4j: ${error.message}`);
              throw error;
            }        
            
            
            const options = {
                cert: await fs.readFile(path.join(ssl_dir, INIZ.SSL.cert)),
                key: await fs.readFile(path.join(ssl_dir, INIZ.SSL.key)),
                ca: await fs.readFile(path.join(ssl_dir, INIZ.SSL.ca)),  // ← AGGIUNGI QUESTA RIGA
                passphrase: INIZ.SSL.passphrase
            };
                
            this.srv_https = https.createServer(options, this.handleHttpRequest.bind(this));
			// Server HTTP interno per chiamate Docker (niente SSL)
			// NON esposto all'esterno — solo nella rete Docker interna
			this.srv_http_internal = http.createServer(this.handleHttpRequest.bind(this));
                
            this.wss = new WebSocketServer({ 
              server: this.srv_https, 
              clientTracking: true 
            });
            
            this.wss.on('connection', this.handleWsConnection.bind(this));
            
        } catch (error) {
            console.error("❌ FATAL: Errore durante l'inizializzazione:", error.message);
            await this.cleanup();
            process.exit(1);
        }
    }

    async cleanup() {
      console.log("🧹 Pulizia risorse in corso...");
      
      const cleanupPromises = [];
      
      // MongoDB
      if (MongoService && MongoService.isConnected()) {
          cleanupPromises.push(
              MongoService.close()
                  .then(() => console.log("✅ MongoDB chiuso"))
                  .catch(err => console.error("❌ Errore chiusura MongoDB:", err.message))
          );
      }
      
      // Tutte le istanze MySQL
      mysqlConnections.forEach((db, index) => {
          if (db && db.isConnected()) {
              cleanupPromises.push(
                  db.close()
                      .then(() => console.log(`✅ MariaDB ${index === 0 ? 'principale' : `#${index + 1}`} chiuso`))
                      .catch(err => console.error(`❌ Errore chiusura MariaDB ${index}:`, err.message))
              );
          }
      });
      
      // Neo4j
      if (neo4jService) {
          cleanupPromises.push(
              neo4jService.close()
                  .then(() => console.log("✅ Neo4j chiuso"))
                  .catch(err => console.error("❌ Errore chiusura Neo4j:", err.message))
          );
      }
      
      await Promise.allSettled(cleanupPromises);
  }


    startHeartbeat() {
        this.interval = setInterval(() => {
            this.wss.clients.forEach((ws) => {
                if (ws.isAlive === false) {
                    tools.setLog(`Terminando client inattivo: ${ws.clientData['sec-websocket-key']}`);
                    return ws.terminate();
                }
                ws.isAlive = false;
                ws.ping();
            });
        }, this.PING_INTERVAL);
    }
    
	async loadContext(request) {
		const protocol = request.connection.encrypted ? 'https' : 'http';
		const parsedUrl = new URL(request.url, `${protocol}://${request.headers.host}`);		
		// Forza il percorso assoluto corretto per Docker
		// process.cwd() è /app/server, quindi cerchiamo /app/server/config.json
		let FILE_CONFIG = parsedUrl.searchParams.get('config_file') || path.resolve(process.cwd(), 'config.json');

		try {
			const data_config = await tools.getContent(FILE_CONFIG);
			const impostazioni = JSON.parse(data_config);
			const queryParams = Object.fromEntries(parsedUrl.searchParams.entries());

			return { impostazioni, queryParams, parsedUrl };
		} catch (err) {
			console.error(`❌ Errore critico nel caricamento del config (${FILE_CONFIG}):`, err.message);
			throw err; // Rilancia per farlo gestire dall'handler HTTP
		}
	}
	
    /**
     * Handler ASINCRONO per tutte le richieste HTTP (GET, POST, ecc.).
     * **Responsabile di creare la SESSIONID e inviare il cookie sicuro.**
     */
	async handleHttpRequest(request, response) {
		let jdata = { request, response };
		try {
			const context = await this.loadContext(request);
			const { impostazioni, queryParams, parsedUrl } = context;
			const pathname = parsedUrl.pathname;
			
			// ⚠️ CORS HEADERS - PRIMA DI TUTTO (prima di qualsiasi altra operazione)
			const origin = request.headers.origin;
			const allowedOrigins = [
				'https://ilpiccolobardo.it',
				'https://www.ilpiccolobardo.it',
				// aggiungi altri domini permessi
			];
			
			if (origin && allowedOrigins.includes(origin)) {
				response.setHeader('Access-Control-Allow-Origin', origin);
				response.setHeader('Access-Control-Allow-Credentials', 'true');
				response.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
				response.setHeader('Access-Control-Allow-Headers', 'Content-Type, Cookie');
				response.setHeader('Access-Control-Expose-Headers', 'Set-Cookie');
			}
			
			// Preflight - rispondere SUBITO
			if (request.method === 'OPTIONS') {
				response.writeHead(204);
				response.end();
				return;
			}
			
			// *** RECUPERA SESSION_ID da MULTIPLI SORGENTI (fallback chain) ***
			let session_id = null;
			let session_source = 'unknown';
			
			// 1. Prova dal cookie (metodo tradizionale)
			const cookies = tools.parseCookies(request.headers.cookie);
			if (cookies.SESSIONID && this.isValidSessionIdFormat(cookies.SESSIONID)) {
				session_id = cookies.SESSIONID;
				session_source = 'cookie';
			}
			
			// 2. Fallback: leggi da query parameter (per sessionStorage client-side)
			if (!session_id && queryParams.session_id && this.isValidSessionIdFormat(queryParams.session_id)) {
				session_id = queryParams.session_id;
				session_source = 'query';
			}
			
			// 3. Fallback: leggi da header custom (alternativa più pulita)
			if (!session_id && request.headers['x-session-id'] && this.isValidSessionIdFormat(request.headers['x-session-id'])) {
				session_id = request.headers['x-session-id'];
				session_source = 'header';
			}
			
			// 1. Determina se serve una nuova sessione
			let isNewSession = false;
			if (!session_id || !this.isValidSessionIdFormat(session_id)) {
				session_id = Server.generateSessionId();
				session_source = 'generated';
				isNewSession = true;
				console.log(`✅ Nuova sessione generata: ${session_id}`);
			}

			// 2. Definisci la directory PRIMA di usarla
			const session_dir = path.join(process.env.SESSION_DIR, session_id);

			// 3. Se è nuova, crea la cartella e i file base
			if (isNewSession) {
				await fs.mkdir(session_dir, { recursive: true });
				await this.ensureUserFileExists(session_dir);
			}

			// 4. IMPORTANTE: Invia il cookie al browser per il prossimo refresh
			if (isNewSession || session_source !== 'cookie') {
				response.setHeader('Set-Cookie', [
					`SESSIONID=${session_id}; Path=/; HttpOnly; Secure; SameSite=Strict; Max-Age=${60 * 60 * 24 * 7}`
				]);
			}	
					
			// 3. PREPARA JDATA
			jdata = {
				...jdata,
				impostazioni,
				url: request.url,
				pagina: pathname,
				query: queryParams,
				dir: path.dirname(pathname),
				doc: path.basename(pathname),
				header: request.headers,
				SESSION: session_id,
				session_dir: session_dir,
			};
			

		
			// 4. GESTISCI RICHIESTA
			switch (request.method) {
				case "POST": {
					// Gestisci POST
					break;
				}
				case "GET": {
					// Gestisci GET
					break;
				}
			}
			
			await this.http_command( jdata);
			
		} catch (error) {
			console.error(`❌ Errore in handleHttpRequest: ${error.message}`);
			console.error(error.stack);
			
			if (!response.headersSent && !response.finished) {
				response.writeHead(500, { 'Content-Type': 'application/json' });
				response.end(JSON.stringify({ 
					error: 'Internal Server Error',
					message: process.env.NODE_ENV === 'development' ? error.message : undefined
				}));
			}
		}
	}

	// Helper per leggere il body
	async parseBody(request) {
		return new Promise((resolve, reject) => {
			let body = '';
			request.on('data', chunk => body += chunk);
			request.on('end', () => {
				try {
					resolve(JSON.parse(body));
				} catch (e) {
					resolve({});
				}
			});
			request.on('error', reject);
		});
	}

    /**
     * Handler ASINCRONO per la connessione WebSocket.
     * **Recupera il SESSIONID dal cookie impostato da HTTPS.**
     */

	async handleWsConnection(ws, request) {
		
		let jdata = { request, ws };
		try {
		
			const context = await this.loadContext(request);
			const { impostazioni, queryParams, parsedUrl } = context;
			const pathname = parsedUrl.pathname;
			
			const parsed_pathname = path.parse(pathname);
			const sec_websocket_key = request.headers['sec-websocket-key'];
			const client_ip = request.socket.remoteAddress;
			
			// *** RECUPERA SESSION_ID da MULTIPLI SORGENTI (fallback chain) ***
			let session_id = null;
			let session_source = 'unknown';
			
			// 1. Prova dal cookie (metodo tradizionale)
			const cookies = tools.parseCookies(request.headers.cookie);
			if (cookies.SESSIONID && this.isValidSessionIdFormat(cookies.SESSIONID)) {
				session_id = cookies.SESSIONID;
				session_source = 'cookie';
			}
			
			// 2. Fallback: leggi da query parameter (per sessionStorage client-side)
			if (!session_id && queryParams.session_id && this.isValidSessionIdFormat(queryParams.session_id)) {
				session_id = queryParams.session_id;
				session_source = 'query';
			}
			
			// 3. Fallback: leggi da header custom (alternativa più pulita)
			if (!session_id && request.headers['x-session-id'] && this.isValidSessionIdFormat(request.headers['x-session-id'])) {
				session_id = request.headers['x-session-id'];
				session_source = 'header';
			}
			
			// Se ANCORA non c'è sessione valida, RIFIUTA la connessione
			if (!session_id) {
				tools.setLog(`❌ WSS: Connessione rifiutata da ${client_ip} - nessuna sessione valida`);
				ws.close(1008, 'Unauthorized: No valid session');
				return;
			}
			
			tools.setLog(`✅ WSS: Sessione ${session_id} validata (source: ${session_source})`);
			
			// Verifica che la sessione esista nel filesystem
			const session_dir = path.join(process.env.SESSION_DIR, session_id);
			
			try {
				await fs.access(session_dir);
				tools.setLog(`✓ WSS: Directory sessione trovata: ${session_dir}`);
			} catch {
				tools.setLog(`⚠️ WSS: Sessione ${session_id} non trovata. Tentativo di ripristino automatico.`);
				
				try {
					// 1. Crea la directory
					await fs.mkdir(session_dir, { recursive: true });
					
					// 2. Crea i file minimi necessari
					await this.ensureUserFileExists(session_dir);
					await this.updateClientIps(client_ip, session_dir);
					
					tools.setLog(`✅ WSS: Sessione ${session_id} rigenerata con successo.`);
				} catch (createError) {
					tools.setLog(`❌ WSS: Errore fatale rigenerazione sessione: ${createError.message}`);
					ws.close(1011, 'Session restoration failed');
					return;
				}
			}
			
			// Prepara jdata
			jdata = {
				...jdata,
				impostazioni,
				url: request.url,
				pagina: pathname,
				query: queryParams,
				dir: path.dirname(pathname),
				doc: path.basename(pathname),
				header: request.headers,
				client_ip,
				SESSION: session_id,
				session_dir: session_dir,
				session_source: session_source, // utile per debug
			};
			
			// Configura ping/pong per keepalive
			ws.isAlive = true;
			ws.on('pong', Server.pongHandler.bind(ws));
			
			// Aggiorna metadata sessione
			await this.updateClientIps(client_ip, session_dir);
			await this.ensureUserFileExists(session_dir);
			
			// Registra il client nella mappa globale
			this.CLIENTS[session_id] = ws;
			
			tools.setLog(`🔌 WSS: Connesso da ${client_ip}. Sessione: ${session_id}. Totale clients: ${Object.keys(this.CLIENTS).length}`);
			
			// Configurazione degli handler
			ws.on('message', (data, isBinary) => this.handleWsMessage(ws, data, isBinary, jdata));
			ws.on('close', () => this.handleWsClose(ws, request, session_id));
			ws.on('error', (error) => console.error(`❌ WS Error for ${sec_websocket_key}:`, error.message));
			
			// Hook personalizzato
			await this.on_my_ws_connection(this, request, ws, jdata);
			
		} catch (error) {
			console.error(`❌ Errore durante la connessione WebSocket: ${error.message}`);
			console.error(error.stack);
			ws.terminate();
		}
	}

    /**
     * Aggiorna il file degli IP dei client connessi
     */
    async updateClientIps(client_ip, session_dir) {
        try {
            const ip_file = path.join(session_dir, 'client_ips.json');
            let ips = [];
            
            try {
                const data = await fs.readFile(ip_file, 'utf8');
                ips = JSON.parse(data);
            } catch (err) {
                // File non esiste, crea array vuoto
            }
            
            if (!ips.includes(client_ip)) {
                ips.push(client_ip);
                await fs.writeFile(ip_file, JSON.stringify(ips, null, 2));
            }
        } catch (error) {
            console.error('Errore updateClientIps:', error.message);
        }
    }

    /**
     * Assicura che il file utente esista
     */
    async ensureUserFileExists(session_dir) {
        try {
            const user_file = path.join(session_dir, 'user.json');
            
            try {
                await fs.access(user_file);
            } catch {
                // File non esiste, crealo
                const defaultUser = {
                    id: Server.generateSessionId(),
                    created: new Date().toISOString(),
                    data: {}
                };
                await fs.writeFile(user_file, JSON.stringify(defaultUser, null, 2));
            }
        } catch (error) {
            console.error('Errore ensureUserFileExists:', error.message);
        }
    }

    /**
     * Gestisce i messaggi WebSocket
     */
    async handleWsMessage(ws, data, isBinary, jdata) {
        try {
            const message = isBinary ? data : data.toString();
            
            try {
                const parsed = JSON.parse(message);
                jdata.message = parsed;
                await this.wss_command(this, ws, jdata);
            } catch {
                // Non è JSON, gestisci come testo
                jdata.message = { type: 'text', data: message };
                await this.wss_command(this, ws, jdata);
            }
        } catch (error) {
          console.error('Errore handleWsMessage:\n', error.stack);
        }
    }

    /**
     * Gestisce la chiusura della connessione WebSocket
     */
    handleWsClose(ws, request, session_id) {
        const sec_websocket_key = request.headers['sec-websocket-key'];
        

    if (session_id && this.CLIENTS[sec_websocket_key]) {
        delete this.CLIENTS[session_id];
        tools.setLog(`🔌 WSS: Disconnesso session ${session_id}. Clients rimanenti: ${Object.keys(this.CLIENTS).length}`);
    }
        
        this.on_my_ws_close(this, ws, request);
    }

    /**
     * Scrive log locale per il client
     */
    async setLocalLog(ws, jdata, message) {
        try {
            const log_file = path.join(jdata.session_dir, 'ws_errors.log');
            const timestamp = new Date().toISOString();
            const log_entry = `${timestamp}: ${message}\n`;
            await fs.appendFile(log_file, log_entry);
        } catch (error) {
            console.error('Errore setLocalLog:', error.message);
        }
    }

    /**
     * Hook per la connessione WebSocket (override in classi derivate)
     */
    async on_my_ws_connection(server, ws, jdata) {
        // Override questo metodo nelle classi derivate
        console.log('[WSS] Nuova connessione WebSocket');
    }

    /**
     * Hook per la chiusura WebSocket (override in classi derivate)
     */
    async on_my_ws_close(server, ws, request) {
        // Override questo metodo nelle classi derivate
        console.log('[WSS] Connessione WebSocket chiusa');
    }
        
    listen() {
        if (!this.srv_https) {
            console.error("Il server non è inizializzato. Chiamare prima il metodo .init()");
            return;
        }
        const h_port = this.port;
        const h_port_int = 2+parseInt(this.port);

        const d = new Date();
        console.log(`${d.toISOString().split('T')[0]} ${d.toTimeString().split(' ')[0]} : process ID ${process.pid}`);
        
        this.srv_https.listen(h_port, this.host, () => {
            console.log(`Listening on https://${this.host}:${h_port} and wss://${this.host}:${h_port}`);
            this.startHeartbeat(); 
        });
        
		// HTTP interno Docker — non aggiungere ai ports in docker-compose
		this.srv_http_internal.listen(h_port_int, '0.0.0.0', () => {
            console.log(`Listening on http://${'0.0.0.0'}:${h_port_int}`);
		});
        
    }

    async http_command( jdata) {
        // Ora puoi accedere all'ID di sessione sicuro tramite jdata.SESSION
        console.log(`[HTTPS] Sessione attiva: ${jdata.SESSION}`); 
        if (!jdata.response.finished) {
             jdata.response.writeHead(200, { 'Content-Type': 'text/plain' });
             jdata.response.end(`Richiesta gestita: ${jdata.pagina} (Sessione: ${jdata.SESSION})`);
        }
    }

    async wss_command(server, ws, jdata) {
        // Ora puoi accedere all'ID di sessione sicuro anche qui
        console.log(`[WSS] Comando ricevuto (${jdata.query.command || 'N/A'}). Sessione: ${jdata.SESSION}`);
    }
    
   
}


// Esempio di utilizzo (ora come Top-Level await)
// Nota: in un file .mjs (o con "type": "module") puoi usare Top-Level await
/*
(async () => {
    const server = new Server(8080);
    await server.init(); // Chiamata al metodo ASINCRONO di inizializzazione
    server.listen();
})();
*/
