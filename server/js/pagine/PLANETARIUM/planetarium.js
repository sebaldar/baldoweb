import 'dotenv/config';
import { loadSolarModule } from '../../services/native-loader.js';
import { handleChat } from '../../services/planetarium-chat.js';

const solar = loadSolarModule();
const escapeXml = value => String(value).replace(/&/g, '&amp;').replace(/"/g, '&quot;')
    .replace(/</g, '&lt;').replace(/>/g, '&gt;');

export default {
    async exe(server, ws, message) {
        const doc = message.doc || message.action;
        switch (doc) {
            case 'command': {
                if (!ws.clientData || ws.readyState !== ws.OPEN) return;
                const command = '<data azione="command" data="' + escapeXml(message.data) + '" />';
                try {
                    const responseXml = solar.handleClient(ws.clientData.id, command);
                    ws.send(JSON.stringify({ tipo: 'command', xml: responseXml }));
                } catch (error) {
                    console.error('[PLANETARIUM] Comando:', error.message);
                }
                break;
            }
            case 'init':
                if (ws.readyState === ws.OPEN) ws.send(JSON.stringify({ tipo: 'init' }));
                break;
            case 'CHATBOT':
                await handleChat(solar, ws, message);
                break;
        }
    }
};
