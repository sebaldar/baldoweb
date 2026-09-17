import 'dotenv/config';
import { canRunAction } from '../../services/admin-auth.js';
import fs from 'fs';
import neo4j from 'neo4j-driver';
import PiccoloBardoManager from './PiccoloBardoManager.js';

// Il servizio FastAPI non è esposto pubblicamente: raggiungibile solo
// dalla rete Docker interna con l'hostname del servizio (docker-compose.yml).
const FASTAPI_BASE = process.env.FASTAPI_URL || 'http://fastapi:8000';

async function callFastApi(path, options = {}) {
    const res = await fetch(`${FASTAPI_BASE}${path}`, {
        ...options,
        headers: { 'Content-Type': 'application/json', ...options.headers, Authorization: `Bearer ${process.env.ADMIN_TOKEN || ''}` },
    });
    const body = await res.json().catch(() => ({}));
    if (!res.ok) {
        throw new Error(body.detail || `FastAPI HTTP ${res.status}`);
    }
    return body;
}

async function manage_fastapi(server, ws, message) {
    const data = message.data;
    // ── Payload base verso FastAPI ────────────────────────────────────────
    const fastapiPayload = {
        prompt:      data.userPrompt,
        eta_bambino: data.age || 5,
        lingua:      'it',
        lunghezza:   'media',
        session_id:  data.session_id || null,
        lat:         data.geo?.lat  ?? null,
        lon:         data.geo?.lon  ?? null,
        source_geo:  data.geo ? 'device' : 'ip',
        dati_astronomici: null,   // verrà popolato se serve
        // Campi di personalizzazione: solo per il report YAML amministrativo
        // della storia, il testo del prompt li contiene già intrecciati.
        nome:              data.nome || null,
        colore_preferito:  data.coloreP || null,
        animale_preferito: data.animaleP || null,
    };

    // ── Helper: invia evento al browser ──────────────────────────────────
    const send = (payload) => {
        if (ws && ws.readyState === ws.OPEN) {
            ws.send(JSON.stringify(payload));
        }
    };

    // ── Helper: leggi stream SSE e gestisci ogni evento ──────────────────
    async function consumaSSE(payload) {
        const fastapiResponse = await fetch('http://fastapi:8000/racconto/stream', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(payload),
        });

        if (!fastapiResponse.ok) {
            const err = await fastapiResponse.json().catch(() => ({}));
            throw new Error(err.detail || `FastAPI HTTP ${fastapiResponse.status}`);
        }

        const decoder = new TextDecoder();
        let buffer = '';

        for await (const chunk of fastapiResponse.body) {
            buffer += decoder.decode(chunk, { stream: true });
            const lines = buffer.split('\n');
            buffer = lines.pop();   // ultima riga potenzialmente incompleta

            for (const line of lines) {
                if (!line.startsWith('data: ')) continue;
                const jsonStr = line.slice(6).trim();
                if (!jsonStr) continue;

                let evento;
                try { evento = JSON.parse(jsonStr); }
                catch { continue; }

                // ── Evento speciale: FastAPI chiede i dati astronomici ──
                if (evento.tipo === 'richiesta_astronomia') {
                    // Non ritrasmettere al browser, gestiscilo qui
                    const datiAstro = await chiamaPlanetarium(
                        evento.lat,
                        evento.lon,
                        evento.data_storia,
                    );

                    if (datiAstro) {
                        // Comunica al browser che stiamo guardando il cielo
                        send({ tipo: 'nodo_end', nodo: 'analizza_prompt',
                               stato: { usa_astronomia: true } });
                        send({ tipo: 'nodo_start', nodo: 'recupera_astronomia' });

                        // Rilancia FastAPI con i dati astronomici già pronti
                        // Interrompe questo stream e ne apre uno nuovo
                        payload.dati_astronomici = datiAstro;
                        return { rilancia: true, payload };
                    }
                    // Se PLANETARIUM fallisce continua senza astronomia
                    continue;
                }

                // ── Tutti gli altri eventi vanno al browser ─────────────
                send(evento);

                if (evento.tipo === 'fine' || evento.tipo === 'errore') {
                    return { rilancia: false };
                }
            }
        }
        return { rilancia: false };
    }

    // ── Helper: chiama il motore astronomico Node ─────────────────────────
    async function chiamaPlanetarium(lat, lon, dataStoria) {
        // Usa coordinate del dispositivo/IP se il prompt non ha un luogo
        const coordLat = lat ?? data.geo?.lat;
        const coordLon = lon ?? data.geo?.lon;
        if (!coordLat || !coordLon) return null;

        try {
            const params = new URLSearchParams({
                lat:  coordLat,
                lon:  coordLon,
                data: dataStoria || new Date().toISOString().split('T')[0],
            });

            // Chiamata interna HTTPS al motore astronomico su Node stesso
            const res = await fetch(`https://www.ilpiccolobardo.it/api/PLANETARIUM?${params}`, {
                headers: { 
                    'x-internal-call': 'baldo-ws',
                    'Cookie': `SESSIONID=${data.session_id}`,
            }
});
            if (!res.ok) {
                console.warn(`PLANETARIUM HTTP ${res.status}`);
                return null;
            }
            return await res.json();
        } catch (err) {
            console.warn('PLANETARIUM non disponibile:', err.message);
            return null;   // degrada gracefully, la storia si fa senza astronomia
        }
    }

    // ── Flusso principale ─────────────────────────────────────────────────
    try {
        send({ tipo: 'nodo_start', nodo: 'avvio' });

        let risultato = await consumaSSE(fastapiPayload);

        // Se FastAPI ha chiesto l'astronomia, rilancia con i dati iniettati
        if (risultato.rilancia) {
            risultato = await consumaSSE(risultato.payload);
        }

    } catch (err) {
        console.error('Errore manage_fastapi:', err.message);
        send({
            tipo:     'errore',
            messaggio: 'Il contastorie non è disponibile al momento. Riprova tra poco.',
        });
    }
}
/*```

---

## Come funziona il flusso completo
```
Browser ──WS──► Node: generate_story
                  │
                  ├─ POST /racconto/stream → FastAPI
                  │    LangGraph: nodo_start analizza_prompt ──────────► Browser
                  │    LangGraph: analizza_prompt estrae lat/lon
                  │    LangGraph: emette evento richiesta_astronomia
                  │                  │
                  │    Node intercetta├─ GET /api/PLANETARIUM?lat=&lon=&data=
                  │                  │    ← { pianeti, luna, costellazioni }
                  │                  │
                  ├─ Rilancia POST /racconto/stream con dati_astronomici già pronti
                  │    LangGraph: salta nodo recupera_astronomia (astronomia_pronta=True)
                  │    LangGraph: nodo_start recupera_frammenti ────────► Browser
                  │    LangGraph: nodo_start genera_racconto ───────────► Browser
                  │    LangGraph: token, token, token ──────────────────► Browser
                  │    LangGraph: fine ────────────────────────────────► Browser

*/
async function manage_node(server, ws, message, manager) {

    const data = message.data;
	const genResult = await manager.generateStoryFromPrompt(data.userPrompt, data.age || 5);
				 
	const response = {
		action: 'generated_story',
		data: genResult
	};

        // Invia la risposta al client WebSocket (se esiste)
        if (ws && ws.readyState === ws.OPEN) {
            ws.send(JSON.stringify(response));
        }

}


async function exe(server, ws, message) {
    try {
        const { action } = message;
        if (!canRunAction(action, message.admin_token)) {
            const denied = { action: 'error', code: 'ADMIN_REQUIRED', message: 'Accesso amministratore richiesto' };
            if (ws?.readyState === ws.OPEN) ws.send(JSON.stringify(denied));
            return denied;
        }
        let response = null; // Inizializziamo response

        const NEO4J_USER = process.env.NEO4J_USER || 'neo4j';
        const NEO4J_PASSWORD = process.env.NEO4J_PASSWORD || 'password';

        // Il costruttore si aspetta un oggetto (vedi PiccoloBardoManager.js), non
        // argomenti posizionali: passati così venivano tutti ignorati e this.uri
        // ripiegava sul default 'bolt://127.0.0.1:7687' (sbagliato in Docker).
        // Mascherato finora perché il driver Neo4j è un singleton già connesso
        // correttamente all'avvio del server con l'URI giusta.
        const manager = new PiccoloBardoManager({
            database: 'neo4j',             // Database di default (Community Edition)
            uri: 'bolt://neo4j:7687',
            user: NEO4J_USER,
            password: NEO4J_PASSWORD
        });

        await manager.initNeo4j();               // ← OBBLIGATORIO
        
        switch (action) {
            case 'init_database':
                await manager.initializeDatabase();
                response = { action: 'init_database', message: 'Database inizializzato con successo' };
                break;

            case 'clear_database':
                await manager.clearAllData();
                response = { action: 'clear_database', message: 'I dati del database cancellati con successo' };
                break;

            case 'create_story': {
                const data = message.data;
                
                
                const story = await manager.createStory({
                    id: data.id,
                    title: data.title,
                    description: data.description,
                    ageMin: data.ageMin,
                    ageMax: data.ageMax
                });

                response = {
                    action: 'create_story',
                    data: story
                };
              }
                break;
            case 'update_story': {
                const data = message.data;
                const story = await manager.updateStory({
                    id: data.id,
                    title: data.title,
                    description: data.description,
                    ageMin: data.ageMin,
                    ageMax: data.ageMax
                });

                response = {
                    action: 'update_story',
                    data: story
                };
              }
                break;
            // Frammenti: allineati allo schema realmente usato dal retrieval
            // (fastapi/services/neo4j_client.py::cerca_frammenti), lo stesso
            // del pannello /loader/. "Salva" = upsert via /admin/frammenti/bulk.
            case 'create_fragment':
            case 'update_fragment': {
                const data = message.data;
                const fragmentPayload = {
                    id: data.id,
                    text: data.text,
                    setting: data.setting,
                    characters: data.characters || [],
                    emotions: data.emotions || [],
                    tema: data.tema || null,
                    archetipo: data.archetipo || null,
                    tecnica_narrativa: data.tecnica_narrativa || null,
                    fascia_eta: data.fascia_eta || null,
                    ritornello: data.ritornello || null,
                    domanda: data.domanda || null,
                };

                const result = await callFastApi('/admin/frammenti/bulk', {
                    method: 'POST',
                    body: JSON.stringify({ frammenti: [fragmentPayload], sovrascrivi: true }),
                });

                if (result.errori && result.errori.length > 0) {
                    response = { action: 'error', message: result.errori.join('; ') };
                } else {
                    response = { action: 'fragment_saved', data: fragmentPayload };
                }
              }
                break;
            case 'create_character': {
                const data = message.data;
                const character = await manager.createCharacter(data);

                response = {
                    action: 'character_saved',
                    data: character
                };
              }
                break;
             case 'update_character': {
                const data = message.data;
                const character = await manager.updateCharacter(data);

                response = {
                    action: 'character_saved',
                    data: character
                };
              }
                break;
           case 'create_setting': {
                const data = message.data;
                const setting = await manager.createSetting(data);

                response = {
                    action: 'create_setting',
                    data: setting
                };
              }
                break;
           case 'updateSetting': {
                const data = message.data;
                const setting = await manager.updateSetting(data);

                response = {
                    action: 'update_setting',
                    data: setting
                };
              }
                break;
            case 'delete_story': {
                const id = message.data.id;
                await manager.deleteStory(id);

                response = {
                    action: 'delete_story',
                    id: id
                };
              }
                break;
            case 'delete_fragment': {
                const id = message.data.id;
                await manager.deletePlotFragment(id);

                response = {
                    action: 'fragment_deleted',
                    id: id
                };
              }
                break;
            case 'delete_character': {
                // I Character non hanno un id: la chiave è il nome (vedi PiccoloBardoManager.js)
                const name = message.data.name;
                await manager.deleteCharacter(name);

                response = {
                    action: 'character_deleted',
                    name: name
                };
              }
                break;
            case 'delete_setting': {
                const id = message.data.id;
                await manager.deleteSetting(id);

                response = {
                    action: 'delete_setting',
                    id: id
                };
              }
                break;
           case 'build_story': {
                const data = message.data;
                const story = await manager.buildCompleteStory({
                  storyId: data.storyId,
                  fragments: data.fragments,
                  characters: data.characters,
                  settingId: data.settingId,
                  emotions: data.emotions,
                  themes: data.themes
                });
                response = {
                    action: 'build_story',
                    data: story
                };
              }
                break;
           case 'update_story_details': {
                const data = message.data;
                const story = await manager.updateStoryDetails({
                  storyId: data.storyId,
                  fragments: data.fragments,
                  characters: data.characters,
                  settingId: data.settingId,
                  emotions: data.emotions,
                  themes: data.themes
                });
                response = {
                    action: 'update_story_details',
                    data: story
                };

              }
                break;

            case 'list_stories':
                const stories = await manager.listStories();
                response = {
                    action: 'list_stories',
                    data: stories
                };
                break;

            case 'get_story_complete':
                const completeStory = await manager.getStoryComplete(message.data.storyId) 
                response = {
                    action: 'story_complete',
                    data: completeStory
                };
                break;
            case 'list_stories':
                const listStories = await manager.listStories() 
                response = {
                    action: 'list_stories',
                    data: listStories
                };
                break;
            case 'list_settings':
                const listSettings = await manager.listSettings() 
                response = {
                    action: 'list_settings',
                    data: listSettings
                };
                break;
            case 'list_characters':
                const listCharacters = await manager.listCharacters() 
                response = {
                    action: 'list_characters',
                    data: listCharacters
                };
                break;
            case 'list_fragments': {
                // Lista dal vero schema (con setting/tema/archetipo/characters/emotions),
                // non da manager.listFragments() che legge solo le proprietà piatte
                // del vecchio schema e non include le relazioni CONTAINS/EVOKES.
                const result = await callFastApi('/admin/frammenti');
                response = {
                    action: 'list_fragments',
                    data: result.frammenti || []
                };
              }
                break;

            case 'generate_story': {
              const data = message.data;
              
              switch ( data.endpoint ) {
				case "fastapi" :
					return manage_fastapi(server, ws, message);
				break;
				case "node" :
					return manage_node(server, ws, message, manager);
				break;
			  }
             
            
            }
              break;
            case 'synthesize_speech': {
              const text = message.data.text;
              const speech_generated = await manager.synthesizeSpeech(text);
              response = {
                action: 'speech_generated',
                audioUrl: speech_generated
              };
            }
              break;
            case 'generate_title': {
              const text = message.data.text;
              const title = await manager.generateTitle(text);
              response = {
                action: 'title_generated',
                title: title
              };
            }
              break;
             case 'generate_illustration': {
              const text = message.data.text;
              const image_generated = await manager.generateImageFromStory(text);
              response = {
                action: 'image_generated',
                img: image_generated
              };
            }
              break;

            case 'list_model_routing': {
                const result = await callFastApi('/admin/modelli');
                response = {
                    action: 'model_routing_list',
                    data: result
                };
              }
                break;

            case 'set_model_routing': {
                const { fase, provider } = message.data;
                const result = await callFastApi(`/admin/modelli/${encodeURIComponent(fase)}`, {
                    method: 'PUT',
                    body: JSON.stringify({ provider }),
                });
                response = {
                    action: 'model_routing_updated',
                    data: result
                };
              }
                break;

              default:
                response = {
                    action: 'error',
                    message: `Azione non riconosciuta: ${action}`
                };
        }

        // Invia la risposta al client WebSocket (se esiste)
        if (ws && ws.readyState === ws.OPEN) {
            ws.send(JSON.stringify(response));
        }

        console.log('Risposta inviata:', response);
        return response;

    } catch (error) {
        console.error('Errore in main.exe:', error);

        const errorResponse = {
            action: 'error',
            message: error.message || 'Errore sconosciuto',
            stack: process.env.NODE_ENV === 'development' ? error.stack : undefined
        };

        if (ws && ws.readyState === ws.OPEN) {
            ws.send(JSON.stringify(errorResponse));
        }

        return errorResponse;
    }
}

export default { exe };
