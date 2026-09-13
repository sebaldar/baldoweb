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
        //
        // NOTA sulla fase lunare: "elongation_deg" dal motore C++ è la
        // differenza grezza di ascensione retta (luna - sole), NON
        // normalizzata in [0°,360°) e trattata a lungo come se fosse
        // un'elongazione assoluta 0-180°. Due difetti, corretti qui:
        // 1) senza normalizzazione, per circa metà delle configurazioni
        //    reali il valore esce dal range 0-180 atteso (può essere molto
        //    negativo o >180), e la fase non veniva assegnata affatto
        //    (restava "non visibile" per il fallback, anche a luna piena).
        // 2) un valore assoluto non distingue mai crescente da calante:
        //    normalizzando in [0°,360°) invece, il segno si conserva —
        //    0°=nuova, 90°=primo quarto, 180°=piena, 270°=ultimo quarto,
        //    esattamente la convenzione standard di "fase lunare".
        const elongazioneNormalizzata = (deg) => ((deg % 360) + 360) % 360;

        const FASE_LUNA_NUOVA_SOGLIA = 10; // gradi di elongazione entro cui la luna è troppo vicina al sole per essere vista

        // Frazione illuminata dall'angolo di fase (approssimazione standard,
        // errore trascurabile per la Luna: il Sole è ~390 volte più lontano
        // della Luna). 0°=nuova→0%, 90°=primo quarto→50%, 180°=piena→100%,
        // esattamente la stessa parametrizzazione già usata per "fase" sopra.
        // Perché conta oltre alla fase testuale: a 15° di elongazione
        // (già fuori dalla soglia di "nuova invisibile") l'illuminazione è
        // solo ~1,7%, un filo sottilissimo — a 70° (ancora "Crescente") è
        // già il 41%, una mezzaluna ben visibile. "Crescente" da solo non
        // distingue i due casi.
        const illuminazionePercento = (elonDeg) =>
            Math.round((1 - Math.cos(elonDeg * Math.PI / 180)) / 2 * 100);

        const simplified_bodies = (raw_json.bodies || [])
            .map(b => {
                let elonNorm = null;
                let fase = null;
                let eNuova = false;
                let illuminazione = null;
                if (b.name === "luna") {
                    elonNorm = elongazioneNormalizzata(b.elongation_deg);
                    eNuova = elonNorm < FASE_LUNA_NUOVA_SOGLIA || elonNorm > 360 - FASE_LUNA_NUOVA_SOGLIA;
                    illuminazione = illuminazionePercento(elonNorm);
                    if (eNuova) fase = "Nuova (invisibile)";
                    else if (elonNorm < 80) fase = "Crescente";
                    else if (elonNorm < 100) fase = "Primo Quarto";
                    else if (elonNorm < 170) fase = "Gibbosa Crescente";
                    else if (elonNorm <= 190) fase = "Piena";
                    else if (elonNorm < 260) fase = "Gibbosa Calante";
                    else if (elonNorm < 280) fase = "Ultimo Quarto";
                    else fase = "Calante";
                }
                return { body: b, fase, eNuova, illuminazione };
            })
            // Una luna nuova non si vede a occhio nudo (troppo vicina al
            // sole nel cielo) anche se geometricamente sopra l'orizzonte —
            // il flag "visible" del motore C++ guarda solo l'altezza.
            .filter(({ body, eNuova }) => body.visible === true && !eNuova)
            .map(({ body: b, fase, illuminazione }) => {
                const alt = b.altitude_deg;
                let posizione = "all'orizzonte";
                if (alt > 20) posizione = "nel cielo";
                if (alt > 50) posizione = "alto sopra la testa";

                return {
                    nome: b.name.charAt(0).toUpperCase() + b.name.slice(1),
                    posizione_testuale: posizione,
                    altezza_deg: Math.round(alt),
                    costellazione: b.constellation,
                    ...(fase ? { fase } : {}),
                    ...(illuminazione !== null ? { illuminazione_percento: illuminazione } : {}),
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
