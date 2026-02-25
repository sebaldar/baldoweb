import 'dotenv/config';
import mysql from 'mysql2/promise';

class MySQL2Service {
  constructor(config = {}) {
    this.config = {
		host: process.env.DB_HOST || 'bardo-mariadb',
		  port: process.env.DB_PORT || 3306,
		  user: process.env.DB_ROOT_USER,
		  password: process.env.DB_ROOT_PASSWORD,
		  database: process.env.DB_NAME,
		  
		  waitForConnections: true,
		  connectionLimit: 10,
		  queueLimit: 0,
		  waitForConnections: true,
		  connectionLimit: config.connectionLimit || 10,
		  queueLimit: 0,
		  enableKeepAlive: true,
		  keepAliveInitialDelay: 0,
		  
		  ...config
    };

    this.pool = null;
    this.connectPromise = null;
    
    // Opzioni di retry e reconnection
    this.maxRetries = config.maxRetries || 3;
    this.retryDelay = config.retryDelay || 2000;
    this.autoReconnect = config.autoReconnect !== false;
    this.reconnectInterval = config.reconnectInterval || 5000;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = config.maxReconnectAttempts || 10;
    this.isReconnecting = false;
  }

  async connect() {
    if (this.pool) {
      return this.pool;
    }

    if (this.connectPromise) {
      return this.connectPromise;
    }

    this.connectPromise = this._connectWithRetry();
    
    try {
      await this.connectPromise;
      return this.pool;
    } finally {
      this.connectPromise = null;
    }
  }

  async _connectWithRetry(attempt = 1) {
    try {
      await this._performConnect();
      this.reconnectAttempts = 0;
      return this.pool;
    } catch (error) {
      if (attempt < this.maxRetries) {
        console.warn(`⚠️ Tentativo ${attempt} fallito, riprovo tra ${this.retryDelay}ms...`);
        await this.sleep(this.retryDelay);
        return this._connectWithRetry(attempt + 1);
      } else {
        console.error(`❌ Connessione fallita dopo ${this.maxRetries} tentativi`);
        throw error;
      }
    }
  }

  sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  async _performConnect() {
    try {
      this.pool = mysql.createPool(this.config);
      
      // Verifica che la connessione sia effettiva
      const conn = await this.pool.getConnection();
      await conn.ping();
      conn.release();
      
      console.log('✅ Connesso a MySQL/MariaDB');

      // Gestione eventi del pool
      this.pool.on('acquire', (connection) => {
        // console.log('🔗 Connessione acquisita');
      });

      this.pool.on('release', (connection) => {
        // console.log('🔓 Connessione rilasciata');
      });

      this.pool.on('connection', (connection) => {
        // console.log('🆕 Nuova connessione creata');
      });

    } catch (error) {
      this.pool = null;
      console.error('❌ Errore connessione MySQL:', error.message);
      throw error;
    }
  }

  _handleDisconnection() {
    if (this.autoReconnect && !this.isReconnecting && this.reconnectAttempts < this.maxReconnectAttempts) {
      this.isReconnecting = true;
      this.reconnectAttempts++;
      
      console.log(`🔄 Tentativo di riconnessione ${this.reconnectAttempts}/${this.maxReconnectAttempts}...`);
      
      setTimeout(async () => {
        try {
          await this.connect();
          console.log('✅ Riconnessione riuscita');
          this.isReconnecting = false;
        } catch (error) {
          console.error('❌ Riconnessione fallita:', error.message);
          this.isReconnecting = false;
        }
      }, this.reconnectInterval);
    } else if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.error('❌ Numero massimo di tentativi di riconnessione raggiunto');
    }
  }

  async getConnection() {
    if (!this.pool) {
      throw new Error('MySQL non connesso. Chiama connect() prima di usare getConnection()');
    }
    
    try {
      return await this.pool.getConnection();
    } catch (error) {
      console.error('❌ Errore acquisizione connessione:', error.message);
      this._handleDisconnection();
      throw error;
    }
  }

  async query(sql, params = []) {
    if (!this.pool) {
      throw new Error('MySQL non connesso. Chiama connect() prima di usare query()');
    }
    
    try {
      const [rows, fields] = await this.pool.execute(sql, params);
      return rows;
    } catch (error) {
      console.error('❌ Errore query:', error.message);
      throw error;
    }
  }

  async execute(sql, params = []) {
    return this.query(sql, params);
  }

  async batch(sql, paramsArray) {
    if (!this.pool) {
      throw new Error('MySQL non connesso');
    }
    
    const conn = await this.pool.getConnection();
    try {
      const results = [];
      for (const params of paramsArray) {
        const [result] = await conn.execute(sql, params);
        results.push(result);
      }
      return results;
    } catch (error) {
      console.error('❌ Errore batch:', error.message);
      throw error;
    } finally {
      conn.release();
    }
  }

  isConnected() {
    return this.pool !== null;
  }

  async healthCheck() {
    if (!this.pool) {
      return { connected: false, message: 'Pool non inizializzato' };
    }

    try {
      const conn = await this.pool.getConnection();
      await conn.ping();
      conn.release();
      
      return { 
        connected: true, 
        message: 'Connessione attiva',
        poolSize: this.config.connectionLimit
      };
    } catch (error) {
      return { connected: false, message: error.message };
    }
  }

  async close() {
    if (this.pool) {
      try {
        this.autoReconnect = false;
        await this.pool.end();
        console.log('🛑 Pool MySQL chiuso');
      } catch (error) {
        console.error('❌ Errore chiusura pool:', error.message);
      } finally {
        this.pool = null;
        this.connectPromise = null;
        this.reconnectAttempts = 0;
        this.isReconnecting = false;
      }
    }
  }

  // ===== TRANSACTION SUPPORT =====

  async withTransaction(callback, options = {}) {
    if (!this.pool) {
      throw new Error('MySQL non connesso. Chiama connect() prima di usare withTransaction()');
    }

    const conn = await this.pool.getConnection();
    try {
      await conn.beginTransaction();

      const result = await callback(conn);

      await conn.commit();
      return result;
    } catch (error) {
      try {
        await conn.rollback();
        console.log('🔙 Transazione annullata (rollback)');
      } catch (rollbackError) {
        console.error('❌ Errore durante rollback:', rollbackError.message);
      }
      console.error('❌ Errore durante la transazione:', error.message);
      throw error;
    } finally {
      conn.release();
    }
  }

  async executeInTransaction(operations) {
    return this.withTransaction(async (conn) => {
      const results = [];
      
      for (const operation of operations) {
        const { sql, params } = operation;
        const [result] = await conn.execute(sql, params);
        results.push(result);
      }
      
      return results;
    });
  }

  // ===== UTILITY METHODS =====

  async insert(table, data) {
    const columns = Object.keys(data).join(', ');
    const placeholders = Object.keys(data).map(() => '?').join(', ');
    const values = Object.values(data);
    
    const sql = `INSERT INTO ${table} (${columns}) VALUES (${placeholders})`;
    const result = await this.query(sql, values);
    return result;
  }

  async update(table, data, where, whereParams = []) {
    const setClause = Object.keys(data).map(key => `${key} = ?`).join(', ');
    const values = [...Object.values(data), ...whereParams];
    
    const sql = `UPDATE ${table} SET ${setClause} WHERE ${where}`;
    return this.query(sql, values);
  }

  async delete(table, where, whereParams = []) {
    const sql = `DELETE FROM ${table} WHERE ${where}`;
    return this.query(sql, whereParams);
  }

  async select(table, columns = '*', where = null, whereParams = []) {
    const cols = Array.isArray(columns) ? columns.join(', ') : columns;
    let sql = `SELECT ${cols} FROM ${table}`;
    
    if (where) {
      sql += ` WHERE ${where}`;
    }
    
    return this.query(sql, whereParams);
  }

  async findOne(table, where, whereParams = []) {
    const results = await this.select(table, '*', where, whereParams);
    return results.length > 0 ? results[0] : null;
  }

  async findAll(table, where = null, whereParams = []) {
    return this.select(table, '*', where, whereParams);
  }

  async count(table, where = null, whereParams = []) {
    let sql = `SELECT COUNT(*) as count FROM ${table}`;
    if (where) {
      sql += ` WHERE ${where}`;
    }
    const result = await this.query(sql, whereParams);
    return result[0].count;
  }

  async exists(table, where, whereParams = []) {
    const count = await this.count(table, where, whereParams);
    return count > 0;
  }

  // Prepared statements
  async prepare(sql) {
    if (!this.pool) {
      throw new Error('MySQL non connesso');
    }
    const conn = await this.pool.getConnection();
    return conn.prepare(sql);
  }
}

// Esporta sia la classe che un'istanza singleton
export default new MySQL2Service();
export { MySQL2Service };
