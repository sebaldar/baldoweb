import 'dotenv/config';

// Il servizio FastAPI non è esposto pubblicamente: è raggiungibile solo
// dalla rete Docker interna con l'hostname del servizio in docker-compose.yml.
const FASTAPI_BASE = process.env.FASTAPI_URL || 'http://fastapi:8000';

async function callFastApi(path, options = {}) {
    const res = await fetch(`${FASTAPI_BASE}${path}`, {
        headers: { 'Content-Type': 'application/json' },
        ...options,
    });
    const body = await res.json().catch(() => ({}));
    if (!res.ok) {
        throw new Error(body.detail || `FastAPI HTTP ${res.status}`);
    }
    return body;
}

export default {
    async exe(jdata) {
        const response = jdata.response;
        const query = jdata.query || {};
        const action = query.action;

        try {
            let result;

            switch (action) {
                case 'admin_bulk_load': {
                    // Il file caricato dall'utente (multipart/form-data, campo file_grimorio)
                    // arriva già parsato: server_base.js prova a fare JSON.parse del contenuto.
                    const fileEntry = (query.files || []).find(f => f.field === 'file_grimorio');
                    if (!fileEntry || typeof fileEntry.content !== 'object') {
                        throw new Error('File dei frammenti mancante o JSON non valido');
                    }

                    const sovrascrivi = query.sovrascrivi === true || query.sovrascrivi === 'true'
                        || Boolean(fileEntry.content.sovrascrivi);

                    result = await callFastApi('/admin/frammenti/bulk', {
                        method: 'POST',
                        body: JSON.stringify({
                            frammenti: fileEntry.content.frammenti || [],
                            sovrascrivi,
                        }),
                    });
                    break;
                }

                case 'admin_stats':
                    result = await callFastApi('/admin/stats');
                    break;

                case 'admin_list_fragments':
                    result = await callFastApi('/admin/frammenti');
                    break;

                case 'admin_delete_fragments': {
                    const data = typeof query.data === 'string' ? JSON.parse(query.data) : (query.data || {});
                    result = await callFastApi('/admin/frammenti', {
                        method: 'DELETE',
                        body: JSON.stringify({ ids: data.ids || [] }),
                    });
                    break;
                }

                default:
                    response.statusCode = 400;
                    response.setHeader('Content-Type', 'application/json');
                    response.end(JSON.stringify({ status: 'error', message: `Azione non riconosciuta: ${action}` }));
                    return;
            }

            response.statusCode = 200;
            response.setHeader('Content-Type', 'application/json');
            response.end(JSON.stringify(result));

        } catch (err) {
            console.error('[ILPICCOLOBARDO/loader] Errore:', err.message);
            response.statusCode = 502;
            response.setHeader('Content-Type', 'application/json');
            response.end(JSON.stringify({ status: 'error', message: err.message }));
        }
    }
};
