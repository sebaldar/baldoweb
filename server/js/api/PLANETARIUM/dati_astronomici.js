import 'dotenv/config';
import { createRequire } from 'module';
import { promises as fs } from 'fs';
import path from 'path';

const require = createRequire(import.meta.url);
const SOLAR_PATH = '/app/server/native/build/Release/solar.node';

let solar;
try {
    solar = require(SOLAR_PATH);
    console.log("✅ Modulo C++ Solar caricato correttamente");
} catch (err) {
    console.error("❌ Errore critico modulo Solar:", err.message);
}

export default {
    async exe(jdata) {
        
        const session_dir = jdata.session_dir;
        const user_file = path.join(session_dir, 'user.json');
        const raw_user = await fs.readFile(user_file, 'utf8');
        const user_data = JSON.parse(raw_user);
        
        
            
        const id = user_data?.data?.planetarium?.id || 100;

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

// 2. SEMPLIFICAZIONE NARRATIVA
        const simplified_bodies = (raw_json.bodies || [])
            .filter(b => b.visible === true)
            .map(b => {
                const alt = b.altitude_deg;
                let posizione = "all'orizzonte";
                if (alt > 20) posizione = "nel cielo";
                if (alt > 50) posizione = "alto sopra la testa";

                let info_extra = {};
                
                // CALCOLO FASE LUNARE basato su elongation_deg
                if (b.name === "luna") {
                    const elon = b.elongation_deg;
                    let fase = "";
                    if (elon < 10) fase = "Nuova (invisibile)";
                    else if (elon < 80) fase = "Crescente";
                    else if (elon < 100) fase = "Primo Quarto";
                    else if (elon < 170) fase = "Gibbosa Crescente";
                    else if (elon <= 180) fase = "Piena";
                    // Nota: per distinguere calante/crescente servirebbe sapere se l'elongazione aumenta o diminuisce, 
                    // ma per una fiaba "Crescente/Piena" è già un ottimo dettaglio.
                    info_extra.fase = fase;
                }

                return {
                    nome: b.name.charAt(0).toUpperCase() + b.name.slice(1),
                    posizione_testuale: posizione,
                    altezza_deg: Math.round(alt),
                    costellazione: b.constellation,
                    ...info_extra, // Aggiunge la fase solo se è la luna
                    stelle_vicine: (b.constellation_stars || []).slice(0, 2)
                };
            });

        // 3. Determina il contesto del cielo
        const luna_obj = simplified_bodies.find(b => b.nome === "Luna");
        const is_night = (raw_json.bodies || []).find(b => b.name === 'sole')?.altitude_deg < -12;

        const response = jdata.response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");

        response.end(JSON.stringify({ 
            status: "success",
            id: id,
            info_cielo: {
                fase_giorno: is_night ? "notte" : "giorno/crepuscolo",
                fase_luna: luna_obj?.fase || "non visibile", // Comodo per il StoryComposer
                data_osservazione: raw_json.date,
                oggetti_visibili: simplified_bodies
            }
        }));
    
    }
};
