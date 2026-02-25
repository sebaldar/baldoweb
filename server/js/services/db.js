// db.js
import { MySQL2Service } from './mysql2-service.js';
import { dbConfig } from './config/databases.js';

export const db = {
  main: new MySQL2Service(dbConfig.main),
  analytics: new MySQL2Service(dbConfig.analytics),
  logs: new MySQL2Service(dbConfig.logs)
};

// Funzione helper per inizializzare tutte le connessioni
export async function initAllDatabases() {
  await Promise.all([
    db.main.connect(),
    db.analytics.connect(),
    db.logs.connect()
  ]);
  console.log('✅ Tutte le connessioni attive');
}
