import 'dotenv/config';
import { MongoClient } from 'mongodb';

class MongoService {
  constructor(uri, options = {}) {
    this.uri = uri || process.env.MONGODB_LOCAL_URI;
    this.client = null;
    this.connectPromise = null;
    
    // Opzioni di retry e reconnection
    this.maxRetries = options.maxRetries || 3;
    this.retryDelay = options.retryDelay || 2000;
    this.autoReconnect = options.autoReconnect !== false;
    this.reconnectInterval = options.reconnectInterval || 5000;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = options.maxReconnectAttempts || 10;
    this.isReconnecting = false;
  }

  async connect() {
    // Se c'è già una connessione attiva, ritornala
    if (this.client) {
      return this.client;
    }

    // Se c'è una connessione in corso, attendi quella
    if (this.connectPromise) {
      return this.connectPromise;
    }

    // Crea una nuova connessione con retry
    this.connectPromise = this._connectWithRetry();
    
    try {
      await this.connectPromise;
      return this.client;
    } finally {
      this.connectPromise = null;
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

  async _connectWithRetry(attempt = 1) {
    try {
      await this._performConnect();
      this.reconnectAttempts = 0; // Reset counter on successful connection
      return this.client;
    } catch (error) {
      if (attempt < this.maxRetries) {
        console.warn(`⚠️ Tentativo ${attempt} fallito, riprovo tra ${this.retryDelay}ms...`);
        await this._sleep(this.retryDelay);
        return this._connectWithRetry(attempt + 1);
      } else {
        console.error(`❌ Connessione fallita dopo ${this.maxRetries} tentativi`);
        throw error;
      }
    }
  }

  _sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  async _performConnect() {
    try {
      this.client = new MongoClient(this.uri, {
        maxPoolSize: 10,
        minPoolSize: 2,
        serverSelectionTimeoutMS: 5000,
        socketTimeoutMS: 45000,
        connectTimeoutMS: 10000,
      });

      await this.client.connect();
      
      // Verifica che la connessione sia effettiva
      await this.client.db('admin').command({ ping: 1 });
      
      console.log('✅ Connesso a MongoDB');

      // Gestione eventi
      this.client.on('close', () => {
        console.warn('⚠️ Connessione MongoDB chiusa');
        this.client = null;
        this._handleDisconnection();
      });

      this.client.on('error', (err) => {
        console.error('❌ Errore MongoDB:', err.message);
        this.client = null;
        this._handleDisconnection();
      });

      this.client.on('timeout', () => {
        console.warn('⚠️ Timeout connessione MongoDB');
      });

      this.client.on('serverHeartbeatFailed', (event) => {
        console.warn('⚠️ Heartbeat MongoDB fallito:', event);
      });

    } catch (error) {
      this.client = null;
      console.error('❌ Errore connessione MongoDB:', error.message);
      throw error;
    }
  }

  async db(dbName) {
    if (!this.client) {
      throw new Error('MongoDB non connesso. Chiama connect() prima di usare db()');
    }
    
    try {
      return this.client.db(dbName);
    } catch (error) {
      console.error('❌ Errore accesso database:', error.message);
      throw error;
    }
  }

  async getCollection(dbName, collectionName) {
    const database = await this.db(dbName);
    return database.collection(collectionName);
  }

  isConnected() {
    return this.client !== null;
  }

  async healthCheck() {
    if (!this.client) {
      return { connected: false, message: 'Client non inizializzato' };
    }

    try {
      await this.client.db('admin').command({ ping: 1 });
      return { connected: true, message: 'Connessione attiva' };
    } catch (error) {
      return { connected: false, message: error.message };
    }
  }

  async close() {
    if (this.client) {
      try {
        this.autoReconnect = false; // Disabilita auto-reconnect durante la chiusura
        await this.client.close();
        console.log('🛑 Connessione MongoDB chiusa');
      } catch (error) {
        console.error('❌ Errore chiusura connessione:', error.message);
      } finally {
        this.client = null;
        this.connectPromise = null;
        this.reconnectAttempts = 0;
        this.isReconnecting = false;
      }
    }
  }

  // ===== TRANSACTION SUPPORT =====

  async withTransaction(callback, options = {}) {
    if (!this.client) {
      throw new Error('MongoDB non connesso. Chiama connect() prima di usare withTransaction()');
    }

    const session = this.client.startSession();
    
    try {
      let result;
      
      await session.withTransaction(async () => {
        result = await callback(session);
      }, {
        readPreference: 'primary',
        readConcern: { level: 'local' },
        writeConcern: { w: 'majority' },
        ...options
      });

      return result;
    } catch (error) {
      console.error('❌ Errore durante la transazione:', error.message);
      throw error;
    } finally {
      await session.endSession();
    }
  }

  async startSession() {
    if (!this.client) {
      throw new Error('MongoDB non connesso');
    }
    return this.client.startSession();
  }

  // Metodo helper per operazioni multi-documento
  async executeInTransaction(operations, options = {}) {
    return this.withTransaction(async (session) => {
      const results = [];
      
      for (const operation of operations) {
        const { collection, method, args } = operation;
        const result = await collection[method](...args, { session });
        results.push(result);
      }
      
      return results;
    }, options);
  }
}

// Esporta sia la classe che un'istanza singleton
export default new MongoService();
export { MongoService };
