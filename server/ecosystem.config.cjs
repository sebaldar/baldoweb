const path = require('path');
// Usa __dirname per essere sicuro di puntare alla cartella dove risiede questo file
require('dotenv').config({ path: path.join(__dirname, '.env') });

module.exports = {
  apps: [{
    name: 'ilpiccolobardo',
    cwd: '/app/server',      // Percorso interno al container
    script: 'index.js',      // Script relativo alla cwd
    out_file: 'logs/out.log',
    error_file: 'logs/err.log',
    log_date_format: 'YYYY-MM-DD HH:mm:ss',
    merge_logs: true,        // Utile se usi più istanze
    autorestart: true,
    watch: false,            // In produzione Docker è meglio tenerlo off
    env: {
      NODE_ENV: 'production',
      VSOP_DIR: process.env.VSOP_DIR,
      DB_HOST: process.env.DB_HOST,
      DB_NAME: process.env.DB_NAME,
      DB_USER: process.env.DB_USER,
      DB_PASS: process.env.DB_PASS
    }
  }]
};
