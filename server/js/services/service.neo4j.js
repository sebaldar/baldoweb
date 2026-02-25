// service.neo4j.js — PATCH COMPLETA PRODUZIONE

import neo4j from 'neo4j-driver';

class Neo4jService {
  driver = null;
  defaultDatabase = 'neo4j';
  connectPromise = null;

  config = {
    uri: process.env.NEO4J_URI || 'bolt://127.0.0.1:7687',
    username: process.env.NEO4J_USER || 'neo4j',
    password: process.env.NEO4J_PASSWORD,
    database: process.env.NEO4J_DEFAULT_DB || 'neo4j',
    maxRetries: Number(process.env.NEO4J_MAX_RETRIES || 5),
    retryDelay: Number(process.env.NEO4J_RETRY_DELAY || 2000),
    queryTimeout: Number(process.env.NEO4J_QUERY_TIMEOUT || 15000)
  };

  /* =========================
     CONNESSIONE
     ========================= */

  async connect(uri, user, pass, db) {
    if (this.driver) return this.driver;
    if (this.connectPromise) return this.connectPromise;

    if (uri) this.config.uri = uri;
    if (user) this.config.username = user;
    if (pass) this.config.password = pass;
    if (db) this.config.database = db;

    this.connectPromise = this.#connectWithRetry();

    try {
      await this.connectPromise;
      return this.driver;
    } finally {
      this.connectPromise = null;
    }
  }

  async #connectWithRetry() {
    const {
      uri, username, password, database,
      maxRetries, retryDelay
    } = this.config;

    if (!password) {
      throw new Error('NEO4J_PASSWORD mancante');
    }

    for (let i = 1; i <= maxRetries; i++) {
      try {
        console.log(`[Neo4j] Connessione ${i}/${maxRetries}`);

        const driver = neo4j.driver(
          uri,
          neo4j.auth.basic(username, password),
          {
            maxConnectionLifetime: 3 * 60 * 60 * 1000,
            maxConnectionPoolSize: 50,
            connectionAcquisitionTimeout: 120000,
            connectionTimeout: 30000
          }
        );

        await driver.verifyConnectivity();

        this.driver = driver;
        this.defaultDatabase = database;

        console.log(`[Neo4j] Connesso a ${uri} (db=${database})`);
        return driver;

      } catch (err) {
        console.error(`[Neo4j] Errore: ${err.message}`);

        // ERRORI FATALI → stop retry
        if (err.code === 'Neo.ClientError.Security.Unauthorized') {
          throw err;
        }

        if (i === maxRetries) throw err;

        await new Promise(r => setTimeout(r, retryDelay));
      }
    }
  }

  /* =========================
     SESSIONI
     ========================= */

  #ensureDriver() {
    if (!this.driver) {
      throw new Error('Neo4j non connesso (chiama connect())');
    }
  }

  getSession(db = null) {
    this.#ensureDriver();
    return this.driver.session({
      database: db || this.defaultDatabase,
      defaultAccessMode: neo4j.session.WRITE
    });
  }

  getReadSession(db = null) {
    this.#ensureDriver();
    return this.driver.session({
      database: db || this.defaultDatabase,
      defaultAccessMode: neo4j.session.READ
    });
  }

  /* =========================
     QUERY
     ========================= */

  async run(cypher, params = {}, db = null) {
    const session = this.getSession(db);
    try {
      const res = await session.run(cypher, params, {
        timeout: this.config.queryTimeout
      });
      return res.records.map(r => r.toObject());
    } finally {
      await session.close();
    }
  }

  async runRead(cypher, params = {}, db = null) {
    const session = this.getReadSession(db);
    try {
      const res = await session.run(cypher, params, {
        timeout: this.config.queryTimeout
      });
      return res.records.map(r => r.toObject());
    } finally {
      await session.close();
    }
  }

  /* =========================
     TRANSAZIONI
     ========================= */

  async withTransaction(work, db = null) {
    const session = this.getSession(db);
    try {
      return await session.executeWrite(work);
    } finally {
      await session.close();
    }
  }

  async withReadTransaction(work, db = null) {
    const session = this.getReadSession(db);
    try {
      return await session.executeRead(work);
    } finally {
      await session.close();
    }
  }

  /* =========================
     HEALTH & SHUTDOWN
     ========================= */

  getConnectionInfo() {
    return {
      connected: Boolean(this.driver),
      uri: this.config.uri,
      database: this.defaultDatabase,
      user: this.config.username
    };
  }

  async verifyConnectivity() {
    this.#ensureDriver();
    await this.driver.verifyConnectivity();
    return true;
  }

  async healthCheck() {
    try {
      await this.verifyConnectivity();
      const r = await this.runRead('RETURN 1 AS ok');
      return { healthy: r[0]?.ok === 1 };
    } catch (e) {
      return { healthy: false, error: e.message };
    }
  }

  async close() {
    if (this.driver) {
      await this.driver.close();
      this.driver = null;
      console.log('[Neo4j] Driver chiuso');
    }
  }
}

/* =========================
   EXPORT
   ========================= */

export const int = neo4j.int;
export const isInt = neo4j.integer.inSafeRange;
export default new Neo4jService();
