import 'dotenv/config';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const binary = new URL('../../native/build/Release/solar.node', import.meta.url);

export function validateSolarEnvironment(env = process.env) {
    // Native static initializers construct strings from getenv(). Validate before
    // dlopen, otherwise missing values abort the process outside JS try/catch.
    const required = ['VSOP_DIR', 'DB_HOST', 'DB_USER', 'DB_PASS', 'DB_NAME'];
    const missing = required.filter(name => typeof env[name] !== 'string' || !env[name].trim());
    if (missing.length) throw new Error(`Configurazione Solar mancante: ${missing.join(', ')}`);
}

export function loadSolarModule() {
    validateSolarEnvironment();
    return require(binary.pathname);
}
