import fs from 'fs/promises';     // ← per scrivere file
import path from 'path';          // ← per gestire percorsi
import { fileURLToPath } from 'url'; // ← necessario per __dirname in ESM

// Per avere __dirname in un modulo ES (come il tuo)
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

import neo4j from 'neo4j-driver';
import { OpenAI } from 'openai'; 
import neo4jService, { int } from '../../services/service.neo4j.js';

/**
 * Il Piccolo Bardo - Neo4j Story Database Manager
 * Gestisce una collezione di storie per bambini in età prescolare
 * Database dedicato: storybook
 */
class PiccoloBardoManager {
  constructor(uri, username, password, database = 'storybook') {
    this.driver = neo4j.driver(
      uri,
      neo4j.auth.basic(username, password)
    );
    this.database = database;

    // Inizializza OpenAI
    this.openai = new OpenAI({
      apiKey: process.env.OPENAI_API_KEY
    });
  }

  getSession() {
    return this.driver.session({ database: this.database });
  }

  async close() {
    await this.driver.close();
  }
  
  
  /**
   * Crea il database storybook se non esiste
   */
  async createDatabase() {
    const systemSession = this.driver.session({ database: 'system' });
    try {
      // Verifica se il database esiste
      const result = await systemSession.run(
        'SHOW DATABASES WHERE name = $name',
        { name: this.database }
      );

      if (result.records.length === 0) {
        // Crea il database
        await systemSession.run(`CREATE DATABASE ${this.database} IF NOT EXISTS`);
        console.log(`✓ Database "${this.database}" creato con successo`);
        
        // Aspetta che il database sia online
        await this.waitForDatabase();
      } else {
        console.log(`✓ Database "${this.database}" già esistente`);
      }
    } catch (error) {
      console.error('Errore durante la creazione del database:', error);
      throw error;
    } finally {
      await systemSession.close();
    }
  }

  /**
   * Aspetta che il database sia online
   */
  async waitForDatabase(maxAttempts = 10, delayMs = 1000) {
    const systemSession = this.driver.session({ database: 'system' });
    try {
      for (let i = 0; i < maxAttempts; i++) {
        const result = await systemSession.run(
          'SHOW DATABASES WHERE name = $name',
          { name: this.database }
        );
        
        if (result.records.length > 0) {
          const status = result.records[0].get('currentStatus');
          if (status === 'online') {
            console.log(`✓ Database "${this.database}" online`);
            return true;
          }
        }
        
        await new Promise(resolve => setTimeout(resolve, delayMs));
      }
      
      throw new Error(`Database "${this.database}" non è diventato online`);
    } finally {
      await systemSession.close();
    }
  }

/**
   * Inizializza il database con constraints, indici e VECTOR INDEX
   */
  async initializeDatabase() {
    await this.createDatabase();
    const session = this.getSession();
    try {
      console.log('Inizializzazione constraints e indici...');

      // Constraints per unicità
      await session.run('CREATE CONSTRAINT story_id IF NOT EXISTS FOR (s:Story) REQUIRE s.id IS UNIQUE');
      await session.run('CREATE CONSTRAINT plot_id IF NOT EXISTS FOR (p:PlotFragment) REQUIRE p.id IS UNIQUE');
      await session.run('CREATE CONSTRAINT char_id IF NOT EXISTS FOR (c:Character) REQUIRE c.id IS UNIQUE');
      await session.run('CREATE CONSTRAINT setting_id IF NOT EXISTS FOR (s:Setting) REQUIRE s.id IS UNIQUE');
      await session.run('CREATE CONSTRAINT emotion_name IF NOT EXISTS FOR (e:Emotion) REQUIRE e.name IS UNIQUE');
      await session.run('CREATE CONSTRAINT theme_name IF NOT EXISTS FOR (t:Theme) REQUIRE t.name IS UNIQUE');

      // Indici classici
      await session.run('CREATE INDEX plot_age IF NOT EXISTS FOR (p:PlotFragment) ON (p.ageMin, p.ageMax)');
      await session.run('CREATE INDEX plot_complexity IF NOT EXISTS FOR (p:PlotFragment) ON (p.complexity)');
      await session.run('CREATE INDEX story_age IF NOT EXISTS FOR (s:Story) ON (s.ageMin, s.ageMax)');

      // VECTOR INDEX per RAG semantico
      await session.run(`
        CREATE VECTOR INDEX fragment_embeddings IF NOT EXISTS
        FOR (f:PlotFragment) ON (f.embedding)
        OPTIONS { indexConfig: { 
          \`vector.dimensions\`: 1536,
          \`vector.similarity_function\`: 'cosine'
        }}
      `);

      console.log('✓ Database storybook inizializzato con successo (incluso vector index)');
    } catch (error) {
      console.error('Errore durante l\'inizializzazione:', error);
      throw error;
    } finally {
      await session.close();
    }
  }
  /**
   * Cancella tutti i dati dal database storybook (non il database stesso)
   */
  async clearAllData() {
    const session = this.getSession();
    try {
      await session.run('MATCH (n) DETACH DELETE n');
      console.log('✓ Tutti i dati cancellati dal database storybook');
    } catch (error) {
      console.error('Errore durante la cancellazione:', error);
      throw error;
    } finally {
      await session.close();
    }
  }

  /**
   * Elimina completamente il database storybook
   */
  async dropDatabase() {
    const systemSession = this.driver.session({ database: 'system' });
    try {
      await systemSession.run(`DROP DATABASE ${this.database} IF EXISTS`);
      console.log(`✓ Database "${this.database}" eliminato`);
    } catch (error) {
      console.error('Errore durante l\'eliminazione del database:', error);
      throw error;
    } finally {
      await systemSession.close();
    }
  }

  /**
   * Crea una nuova storia
   */
  async createStory(storyData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `CREATE (s:Story {
          id: $id,
          title: $title,
          description: $description,
          ageMin: $ageMin,
          ageMax: $ageMax,
          createdAt: datetime()
        })
        RETURN s`,
        storyData
      );
      console.log(`✓ Storia creata: ${storyData.title}`);
      return result.records[0].get('s').properties;
    } finally {
      await session.close();
    }
  }

/**
 * Crea un nuovo frammento di trama
 */
async createPlotFragment(fragmentData) {
  const session = this.getSession();
  try {
    // Mappatura sicura dei parametri: accetta varianti di nomi e fornisce fallback
    const params = {
      id: fragmentData.id,
      text: fragmentData.text || fragmentData.testo || "",
      ageMin: fragmentData.ageMin || fragmentData.etaMin || fragmentData.eta_minima || 4,
      ageMax: fragmentData.ageMax || fragmentData.etaMax || fragmentData.eta_massima || 10,
      complexity: fragmentData.complexity || fragmentData.complessita || fragmentData.complessità || 1,
      tone: fragmentData.tone || fragmentData.tono || "neutro",
      embedding: fragmentData.embedding || null  // importante: se non c'è, metti null (non undefined!)
    };

    const result = await session.run(
      `CREATE (p:PlotFragment {
        id: $id,
        text: $text,
        ageMin: $ageMin,
        ageMax: $ageMax,
        complexity: $complexity,
        tone: $tone,
        embedding: $embedding
      })
      RETURN p`,
      params  // ← passiamo l'oggetto mappato, non fragmentData grezzo
    );

    const createdFragment = result.records[0].get('p').properties;
    console.log(`✓ Frammento creato: ${createdFragment.id} - "${createdFragment.text.substring(0, 50)}..."`);

    return createdFragment;

  } catch (error) {
    console.error('Errore nella creazione del frammento:', error);
    throw error; // rilancia per gestione superiore
  } finally {
    await session.close();
  }
}

  /**
   * Crea un personaggio
   */
  async createCharacter(charData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `CREATE (c:Character {
          id: $id,
          name: $name,
          traits: $traits,
          description: $description
        })
        RETURN c`,
        charData
      );
      console.log(`✓ Personaggio creato: ${charData.name}`);
      return result.records[0].get('c').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Crea un'ambientazione
   */
  async createSetting(settingData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `CREATE (s:Setting {
          id: $id,
          name: $name,
          type: $type,
          description: $description
        })
        RETURN s`,
        settingData
      );
      console.log(`✓ Ambientazione creata: ${settingData.name}`);
      return result.records[0].get('s').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Crea o recupera un'emozione
   */
  async createOrGetEmotion(emotionName) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MERGE (e:Emotion {name: $name})
        RETURN e`,
        { name: emotionName }
      );
      return result.records[0].get('e').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Crea o recupera un tema
   */
  async createOrGetTheme(themeName) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MERGE (t:Theme {name: $name})
        RETURN t`,
        { name: themeName }
      );
      return result.records[0].get('t').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Collega un frammento a una storia
   */
  async linkFragmentToStory(storyId, fragmentId, order = 0) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (s:Story {id: $storyId})
        MATCH (p:PlotFragment {id: $fragmentId})
        MERGE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
        { storyId, fragmentId, order }
      );
      console.log(`✓ Frammento ${fragmentId} collegato alla storia ${storyId}`);
    } finally {
      await session.close();
    }
  }

  /**
   * Collega un frammento a un'emozione
   */
  async linkFragmentToEmotion(fragmentId, emotionName) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (p:PlotFragment {id: $fragmentId})
        MERGE (e:Emotion {name: $emotionName})
        MERGE (p)-[:EVOKES]->(e)`,
        { fragmentId, emotionName }
      );
      console.log(`✓ Frammento collegato all'emozione: ${emotionName}`);
    } finally {
      await session.close();
    }
  }

  /**
   * Collega un frammento a un tema
   */
  async linkFragmentToTheme(fragmentId, themeName) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (p:PlotFragment {id: $fragmentId})
        MERGE (t:Theme {name: $themeName})
        MERGE (p)-[:FITS_THEME]->(t)`,
        { fragmentId, themeName }
      );
      console.log(`✓ Frammento collegato al tema: ${themeName}`);
    } finally {
      await session.close();
    }
  }

  /**
   * Collega due frammenti in sequenza
   */
  async linkFragmentSequence(fromFragmentId, toFragmentId, weight = 1.0) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (p1:PlotFragment {id: $fromId})
        MATCH (p2:PlotFragment {id: $toId})
        MERGE (p1)-[:CAN_FOLLOW {weight: $weight}]->(p2)`,
        { fromId: fromFragmentId, toId: toFragmentId, weight }
      );
      console.log(`✓ Sequenza creata: ${fromFragmentId} -> ${toFragmentId}`);
    } finally {
      await session.close();
    }
  }

  /**
   * Collega un personaggio a una storia
   */
  async linkCharacterToStory(storyId, characterId, role = 'protagonista') {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (s:Story {id: $storyId})
        MATCH (c:Character {id: $characterId})
        MERGE (s)-[:HAS_CHARACTER {role: $role}]->(c)`,
        { storyId, characterId, role }
      );
      console.log(`✓ Personaggio collegato alla storia`);
    } finally {
      await session.close();
    }
  }

  /**
   * Collega un'ambientazione a una storia
   */
  async linkSettingToStory(storyId, settingId, order = 0) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (s:Story {id: $storyId})
        MATCH (set:Setting {id: $settingId})
        MERGE (s)-[:SET_IN {order: $order}]->(set)`,
        { storyId, settingId, order }
      );
      console.log(`✓ Ambientazione collegata alla storia (ordine: ${order})`);
    } finally {
      await session.close();
    }
  }

  /**
   * Collega un'ambientazione a un frammento specifico
   */
  async linkFragmentToSetting(fragmentId, settingId) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (p:PlotFragment {id: $fragmentId})
        MATCH (set:Setting {id: $settingId})
        MERGE (p)-[:TAKES_PLACE_IN]->(set)`,
        { fragmentId, settingId }
      );
      console.log(`✓ Frammento ${fragmentId} collegato all'ambientazione ${settingId}`);
    } finally {
      await session.close();
    }
  }

  /**
   * Costruisci una storia completa con tutti i collegamenti in una transazione
   */
async buildCompleteStory(buildData) {
  const session = this.getSession();
  
  // Valori di default sicuri
  const {
    storyId,
    fragments = [],        // ← default array vuoto
    characters = [],       // ← default array vuoto
    settings = [],         // ← default array vuoto
    emotions = [],         // ← default array vuoto
    themes = []            // ← default array vuoto
  } = buildData;

  // Controllo obbligatorio: storyId deve esistere
  if (!storyId) {
    throw new Error('storyId è obbligatorio per buildCompleteStory');
  }

  try {
    const txc = session.beginTransaction();

    // 1. Collega i frammenti (se presenti)
    for (const frag of fragments) {
      await txc.run(
        `MATCH (s:Story {id: $storyId})
         MATCH (p:PlotFragment {id: $fragmentId})
         MERGE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
        { storyId, fragmentId: frag.fragmentId, order: frag.order ?? 0 }
      );

      if (frag.settingId) {
        await txc.run(
          `MATCH (p:PlotFragment {id: $fragmentId})
           MATCH (set:Setting {id: $settingId})
           MERGE (p)-[:TAKES_PLACE_IN]->(set)`,
          { fragmentId: frag.fragmentId, settingId: frag.settingId }
        );
      }
    }

    // 2. Emozioni globali su tutti i frammenti
    if (emotions.length > 0) {
      for (const frag of fragments) {
        for (const emotion of emotions) {
          await txc.run(
            `MATCH (p:PlotFragment {id: $fragmentId})
             MERGE (e:Emotion {name: $emotion})
             MERGE (p)-[:EVOKES]->(e)`,
            { fragmentId: frag.fragmentId, emotion }
          );
        }
      }
    }

    // 3. Temi globali su tutti i frammenti
    if (themes.length > 0) {
      for (const frag of fragments) {
        for (const theme of themes) {
          await txc.run(
            `MATCH (p:PlotFragment {id: $fragmentId})
             MERGE (t:Theme {name: $theme})
             MERGE (p)-[:FITS_THEME]->(t)`,
            { fragmentId: frag.fragmentId, theme }
          );
        }
      }
    }

    // 4. Personaggi
    for (const char of characters) {
      await txc.run(
        `MATCH (s:Story {id: $storyId})
         MATCH (c:Character {id: $characterId})
         MERGE (s)-[:HAS_CHARACTER {role: $role}]->(c)`,
        { storyId, characterId: char.characterId, role: char.role ?? 'secondario' }
      );
    }

    // 5. Ambientazioni generali
    for (const setting of settings) {
      await txc.run(
        `MATCH (s:Story {id: $storyId})
         MATCH (set:Setting {id: $settingId})
         MERGE (s)-[:SET_IN {order: $order}]->(set)`,
        { storyId, settingId: setting.settingId, order: setting.order ?? 0 }
      );
    }

    await txc.commit();
    console.log(`✓ Storia completa "${storyId}" costruita con successo`);
  } catch (error) {
    console.error('Errore in buildCompleteStory:', error);
    throw error;
  } finally {
    await session.close();
  }
}
  /**
   * Lista tutte le storie
   */
  async listStories() {
    const session = this.getSession();
    try {
      const result = await session.run(
        'MATCH (s:Story) RETURN s ORDER BY s.createdAt DESC'
      );
      return result.records.map(record => record.get('s').properties);
    } finally {
      await session.close();
    }
  }

  /**
   * Lista tutti i frammenti
   */
  async listFragments() {
    const session = this.getSession();
    try {
      const result = await session.run(
        'MATCH (p:PlotFragment) RETURN p ORDER BY p.id'
      );
      return result.records.map(record => record.get('p').properties);
    } finally {
      await session.close();
    }
  }

  /**
   * Lista tutti i personaggi
   */
  async listCharacters() {
    const session = this.getSession();
    try {
      const result = await session.run(
        'MATCH (c:Character) RETURN c ORDER BY c.name'
      );
      return result.records.map(record => record.get('c').properties);
    } finally {
      await session.close();
    }
  }

  /**
   * Lista tutte le ambientazioni
   */
  async listSettings() {
    const session = this.getSession();
    try {
      const result = await session.run(
        'MATCH (s:Setting) RETURN s ORDER BY s.name'
      );
      return result.records.map(record => record.get('s').properties);
    } finally {
      await session.close();
    }
  }

  /**
   * Recupera frammenti adatti per età e tema
   */
async getFragmentsByAgeAndTheme(ageMin, ageMax, themeName, limit = 10) {
  const session = this.getSession();
  try {
    // Forza tutti i parametri come interi
    const intAgeMin = int(Math.floor(ageMin));
    const intAgeMax = int(Math.floor(ageMax));
    const intLimit = int(Math.floor(limit));

    const result = await session.run(
      `MATCH (p:PlotFragment)-[:FITS_THEME]->(t:Theme {name: $theme})
       WHERE p.ageMin <= $ageMax AND p.ageMax >= $ageMin
       RETURN p
       LIMIT $limit`,
      { 
        theme: themeName, 
        ageMin: intAgeMin, 
        ageMax: intAgeMax, 
        limit: intLimit 
      }
    );
    return result.records.map(record => record.get('p').properties);
  } finally {
    await session.close();
  }
}

  /**
   * Recupera frammenti per emozione
   */
  async getFragmentsByEmotion(emotionName, ageMin, ageMax, limit = 10) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (p:PlotFragment)-[:EVOKES]->(e:Emotion {name: $emotion})
        WHERE p.ageMin <= $ageMax AND p.ageMax >= $ageMin
        RETURN p
        LIMIT $limit`,
        { emotion: emotionName, ageMin, ageMax, limit }
      );
      return result.records.map(record => record.get('p').properties);
    } finally {
      await session.close();
    }
  }

  /**
   * Recupera possibili frammenti successivi
   */
  async getNextFragments(fragmentId, limit = 5) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (p1:PlotFragment {id: $fragmentId})-[r:CAN_FOLLOW]->(p2:PlotFragment)
        RETURN p2, r.weight as weight
        ORDER BY weight DESC
        LIMIT $limit`,
        { fragmentId, limit }
      );
      return result.records.map(record => ({
        fragment: record.get('p2').properties,
        weight: record.get('weight')
      }));
    } finally {
      await session.close();
    }
  }
  
/**
   * Genera embedding con OpenAI e lo salva nel frammento
   */
  async generateAndSetEmbedding(fragmentId) {
    const session = this.getSession();
    try {
      // Recupera il testo del frammento
      const result = await session.run(
        `MATCH (p:PlotFragment {id: $id}) RETURN p.text AS text`,
        { id: fragmentId }
      );

      if (result.records.length === 0) throw new Error(`Frammento ${fragmentId} non trovato`);

      const text = result.records[0].get('text');
      if (!text || text.trim() === '') return;

      // Genera embedding con OpenAI
      const response = await this.openai.embeddings.create({
        model: 'text-embedding-3-small',
        input: text
      });

      const embedding = response.data[0].embedding;

      // Salva nel DB
      await session.run(
        `MATCH (p:PlotFragment {id: $id})
         SET p.embedding = $embedding`,
        { id: fragmentId, embedding }
      );

      console.log(`✓ Embedding generato e salvato per frammento: ${fragmentId}`);
    } catch (error) {
      console.error('Errore generazione embedding:', error);
      throw error;
    } finally {
      await session.close();
    }
  }

  /**
   * Popola TUTTI gli embedding per i frammenti esistenti
   */
  async populateAllEmbeddings() {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (p:PlotFragment) 
         WHERE p.embedding IS NULL OR size(p.embedding) = 0
         RETURN p.id AS id`
      );

      const fragmentIds = result.records.map(r => r.get('id'));
      console.log(`Trovati ${fragmentIds.length} frammenti senza embedding. Generazione in corso...`);

      for (const id of fragmentIds) {
        await this.generateAndSetEmbedding(id);
        await new Promise(r => setTimeout(r, 100)); // Rate limit gentile
      }

      console.log('✓ Tutti gli embedding sono stati popolati!');
    } finally {
      await session.close();
    }
  }

  /**
   * Vector search semantico
   */
  async vectorSearch(queryText, limit = 10) {
    const session = this.getSession();
    try {
      // Genera embedding per la query
      const response = await this.openai.embeddings.create({
        model: 'text-embedding-3-small',
        input: queryText
      });
      const queryEmbedding = response.data[0].embedding;

      // Cerca nel vector index
      const result = await session.run(
        `CALL db.index.vector.queryNodes('fragment_embeddings', $limit, $queryEmbedding)
         YIELD node, score
         RETURN node.id AS id, node.text AS text, score
         ORDER BY score DESC`,
        { limit, queryEmbedding }
      );

      return result.records.map(r => ({
        id: r.get('id'),
        text: r.get('text'),
        score: r.get('score')
      }));
    } finally {
      await session.close();
    }
  }

  /**
   * Genera una storia con RAG basata su un prompt utente
   */
async generateStoryFromPrompt(userPrompt, age = 5) {
  try {
    console.log(`Generazione storia per età ${age}: "${userPrompt}"`);

    const vectorFragments = await this.vectorSearch(userPrompt, 8);
    const themes = await this.extractThemesFromPrompt(userPrompt);
    const emotions = await this.extractEmotionsFromPrompt(userPrompt);

    const thematicFragments = [];
    for (const theme of themes) {
      const frags = await this.getFragmentsByAgeAndTheme(age - 2, age + 2, theme, 4);
      thematicFragments.push(...frags);
    }

    const allFragments = [
      ...vectorFragments.map(f => ({ ...f, source: 'vector' })),
      ...thematicFragments.map(f => ({ id: f.id, text: f.text, score: 0.7 }))
    ];

    const uniqueFragments = Array.from(new Map(allFragments.map(f => [f.id, f])).values())
      .sort((a, b) => (b.score || 0) - (a.score || 0))
      .slice(0, 10);

    const inspirationText = uniqueFragments
      .map((f, i) => `${i + 1}. "${f.text}"`)
      .join('\n');

    const systemPrompt = `
Sei "Il Piccolo Bardo", un narratore gentile di favole per bambini dai 3 ai 10 anni.
Il tuo stile è caldo, rassicurante, poetico, con frasi brevi e qualche ripetizione dolce.
Inizia sempre con "C'era una volta...".

Crea una storia NUOVA e ORIGINALE ispirata ai seguenti frammenti del tuo repertorio.
Non copiare frasi intere, ma usa temi, immagini, emozioni e tono per ispirarti.

Frammenti di ispirazione:
${inspirationText}

Età del bambino: ${age} anni.
Richiesta: ${userPrompt}

REGOLE FERREE:
- Protagonista: un bambino/a o un animale parlante
- Massimo 3 personaggi principali
- Piccolo problema gentile (niente paura intensa o violenza)
- Soluzione basata su gentilezza, coraggio semplice o collaborazione
- Ambientazione quotidiana o fantastica leggera
- Valori: amicizia, fiducia in sé, rispetto, aiuto reciproco
- Nessuna morale esplicita alla fine
- Tono sempre positivo e sereno, perfetto per la buonanotte

LUNGHEZZA:
${age <= 4 ? '200-350 parole (frasi molto brevi)' : age <= 6 ? '350-500 parole' : '450-650 parole'}

STRUTTURA NATURALE:
- Inizio con presentazione del protagonista e del suo mondo
- Piccolo problema o desiderio
- Incontro con 1-2 amici/aiutanti
- Soluzione gentile e felice
- Finale sereno

OUTPUT:
Titolo breve e magico
Seguito dal testo della storia in paragrafi naturali.
    `;

    const completion = await this.openai.chat.completions.create({
      model: 'gpt-4o-mini',
      messages: [
        { role: 'system', content: systemPrompt },
        { role: 'user', content: 'Raccontami la storia.' }
      ],
      temperature: 0.8,
      max_tokens: 1200
    });

    const generatedStory = completion.choices[0].message.content.trim();

    // Salva la storia generata nella memoria narrativa e semantica
    await this.saveGeneratedStory(generatedStory, age, userPrompt);

    return {
      story: generatedStory,
      inspiration: uniqueFragments.map(f => ({ id: f.id, text: f.text.substring(0, 100) + '...' })),
      model: 'gpt-4o-mini'
    };
  } catch (error) {
    console.error('Errore generazione storia:', error);
    throw error;
  }
}

  // Funzioni helper per estrarre temi/emozioni (usa LLM)
  async extractThemesFromPrompt(prompt) {
    const completion = await this.openai.chat.completions.create({
      model: 'gpt-4o-mini',
      messages: [{ role: 'user', content: `Estrai 2-3 temi principali da questa richiesta per una favola per bambini: "${prompt}". Rispondi solo con i temi separati da virgola.` }],
      temperature: 0
    });
    return completion.choices[0].message.content.split(',').map(t => t.trim().toLowerCase());
  }

  async extractEmotionsFromPrompt(prompt) {
    const completion = await this.openai.chat.completions.create({
      model: 'gpt-4o-mini',
      messages: [{ role: 'user', content: `Estrai 2-3 emozioni principali da questa richiesta: "${prompt}". Rispondi solo con le emozioni separate da virgola.` }],
      temperature: 0
    });
    return completion.choices[0].message.content.split(',').map(e => e.trim().toLowerCase());
  }  

/**
 * Recupera una storia completa con tutti i suoi elementi
 */
async getStoryComplete(storyId) {
  const session = this.getSession();
  try {
const result = await session.run(
  `MATCH (s:Story {id: $storyId})

  OPTIONAL MATCH (s)-[hf:HAS_FRAGMENT]->(p:PlotFragment)
  OPTIONAL MATCH (p)-[:EVOKES]->(e:Emotion)
  OPTIONAL MATCH (p)-[:FITS_THEME]->(t:Theme)

  WITH s, p, hf,
       collect(DISTINCT e.name) AS fragEmotions,
       collect(DISTINCT t.name) AS fragThemes

  WITH s,
       collect(DISTINCT {
         fragment: {
           id: p.id,
           text: p.text,
           ageMin: p.ageMin,
           ageMax: p.ageMax,
           complexity: p.complexity,
           tone: p.tone
         },
         order: coalesce(hf.order, 0),
         emotions: fragEmotions,
         themes: fragThemes
       }) AS fragments

  OPTIONAL MATCH (s)-[hc:HAS_CHARACTER]->(c:Character)
  WITH s, fragments, c, hc

  WITH s, fragments,
       collect(DISTINCT CASE WHEN c IS NOT NULL THEN {character: properties(c), role: hc.role} END) AS characters

  OPTIONAL MATCH (s)-[:SET_IN]->(set:Setting)
  WITH s, fragments, characters, set

  WITH s, fragments, characters,
       collect(DISTINCT CASE WHEN set IS NOT NULL THEN properties(set) END) AS settings

  RETURN
    properties(s) AS story,
    fragments,
    characters,
    settings`,
  { storyId }
);


    if (result.records.length === 0) return null;

    const record = result.records[0];

    return {
      story: record.get('story'),
      fragments: record.get('fragments')
        .filter(f => f.fragment && f.fragment.id)  // sicurezza
        .sort((a, b) => a.order - b.order)
        .map(f => ({
          id: f.fragment.id,
          text: f.fragment.text || '(testo non disponibile)',
          ageMin: f.fragment.ageMin,
          ageMax: f.fragment.ageMax,
          complexity: f.fragment.complexity || 'media',
          tone: f.fragment.tone || 'neutro',
          order: f.order,
          emotions: f.emotions || [],
          themes: f.themes || []
        })),
      characters: record.get('characters')
        .filter(c => c !== null)
        .map(c => ({
          id: c.character.id,
          name: c.character.name,
          description: c.character.description || '',
          traits: c.character.traits || [],
          role: c.role || 'secondario'
        })),
      settings: record.get('settings')
        .filter(s => s !== null)
    };

  } finally {
    await session.close();
  }
}

  /**
   * Cerca storie per criteri multipli
   */
  async searchStories(criteria = {}) {
    const session = this.getSession();
    try {
      const { ageMin, ageMax, theme, emotion, character } = criteria;
      
      let query = 'MATCH (s:Story) WHERE 1=1';
      const params = {};
      
      if (ageMin !== undefined && ageMax !== undefined) {
        query += ' AND s.ageMin <= $ageMax AND s.ageMax >= $ageMin';
        params.ageMin = ageMin;
        params.ageMax = ageMax;
      }
      
      if (theme) {
        query += ' AND EXISTS((s)-[:HAS_FRAGMENT]->(:PlotFragment)-[:FITS_THEME]->(:Theme {name: $theme}))';
        params.theme = theme;
      }
      
      if (emotion) {
        query += ' AND EXISTS((s)-[:HAS_FRAGMENT]->(:PlotFragment)-[:EVOKES]->(:Emotion {name: $emotion}))';
        params.emotion = emotion;
      }
      
      if (character) {
        query += ' AND EXISTS((s)-[:HAS_CHARACTER]->(:Character {name: $character}))';
        params.character = character;
      }
      
      query += ' RETURN s';
      
      const result = await session.run(query, params);
      return result.records.map(record => record.get('s').properties);
    } finally {
      await session.close();
    }
  }

  /**
   * Ottieni statistiche del database
   */
  async getStatistics() {
    const session = this.getSession();
    try {
      const result = await session.run(`
        MATCH (s:Story) WITH count(s) as stories
        MATCH (p:PlotFragment) WITH stories, count(p) as fragments
        MATCH (c:Character) WITH stories, fragments, count(c) as characters
        MATCH (set:Setting) WITH stories, fragments, characters, count(set) as settings
        MATCH (e:Emotion) WITH stories, fragments, characters, settings, count(e) as emotions
        MATCH (t:Theme) WITH stories, fragments, characters, settings, emotions, count(t) as themes
        RETURN stories, fragments, characters, settings, emotions, themes
      `);
      
      if (result.records.length === 0) {
        return { stories: 0, fragments: 0, characters: 0, settings: 0, emotions: 0, themes: 0 };
      }
      
      const record = result.records[0];
      return {
        stories: record.get('stories').toNumber(),
        fragments: record.get('fragments').toNumber(),
        characters: record.get('characters').toNumber(),
        settings: record.get('settings').toNumber(),
        emotions: record.get('emotions').toNumber(),
        themes: record.get('themes').toNumber()
      };
    } finally {
      await session.close();
    }
  }

  /**
   * Elimina un nodo per ID e tipo
   */
  async deleteNode(nodeType, nodeId) {
    const session = this.getSession();
    try {
      await session.run(
        `MATCH (n:${nodeType} {id: $id})
        DETACH DELETE n`,
        { id: nodeId }
      );
      console.log(`✓ Nodo ${nodeType} eliminato: ${nodeId}`);
    } finally {
      await session.close();
    }
  }

  /**
   * Aggiorna le proprietà di un nodo
   */
  async updateNode(nodeType, nodeId, properties) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (n:${nodeType} {id: $id})
        SET n += $properties
        RETURN n`,
        { id: nodeId, properties }
      );
      console.log(`✓ Nodo ${nodeType} aggiornato: ${nodeId}`);
      return result.records[0]?.get('n').properties;
    } finally {
      await session.close();
    }
  }
  /**
   * Aggiorna una storia esistente
   */
  async updateStory(storyData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (s:Story {id: $id})
         SET s.title = $title,
             s.description = $description,
             s.ageMin = $ageMin,
             s.ageMax = $ageMax
         RETURN s`,
        storyData
      );
      console.log(`✓ Storia aggiornata: ${storyData.title}`);
      return result.records[0].get('s').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Aggiorna un frammento esistente
   */
  async updatePlotFragment(fragmentData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (p:PlotFragment {id: $id})
         SET p.text = $text,
             p.ageMin = $ageMin,
             p.ageMax = $ageMax,
             p.complexity = $complexity,
             p.tone = $tone
         RETURN p`,
        fragmentData
      );
      console.log(`✓ Frammento aggiornato: ${fragmentData.id}`);
      return result.records[0].get('p').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Aggiorna un personaggio esistente
   */
  async updateCharacter(charData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (c:Character {id: $id})
         SET c.name = $name,
             c.description = $description,
             c.traits = $traits
         RETURN c`,
        charData
      );
      console.log(`✓ Personaggio aggiornato: ${charData.name}`);
      return result.records[0].get('c').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Aggiorna un'ambientazione esistente
   */
  async updateSetting(settingData) {
    const session = this.getSession();
    try {
      const result = await session.run(
        `MATCH (s:Setting {id: $id})
         SET s.name = $name,
             s.type = $type,
             s.description = $description
         RETURN s`,
        settingData
      );
      console.log(`✓ Ambientazione aggiornata: ${settingData.name}`);
      return result.records[0].get('s').properties;
    } finally {
      await session.close();
    }
  }

  /**
   * Aggiorna i dettagli di una storia (frammenti, personaggi, ambientazioni, emozioni, temi)
   */
  async updateStoryDetails(data) {
    const session = this.getSession();
    const {
      storyId,
      fragments = [],
      characters = [],
      settings = [],
      emotions = [],
      themes = []
    } = data;

    try {
      const txc = session.beginTransaction();

      // Rimuovi tutti i collegamenti esistenti
      await txc.run(
        `MATCH (s:Story {id: $storyId})-[r:HAS_FRAGMENT|HAS_CHARACTER|SET_IN]->()
         DELETE r`,
        { storyId }
      );

      await txc.run(
        `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)-[r:EVOKES|FITS_THEME]->()
         DELETE r`,
        { storyId }
      );

      // Ricrea i collegamenti frammenti
      for (const frag of fragments) {
        await txc.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (p:PlotFragment {id: $fragmentId})
           MERGE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
          { storyId, fragmentId: frag, order: 0 }
        );
      }

      // Personaggi
      for (const char of characters) {
        await txc.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (c:Character {id: $characterId})
           MERGE (s)-[:HAS_CHARACTER {role: $role}]->(c)`,
          { storyId, characterId: char.characterId, role: char.role || 'secondario' }
        );
      }

      // Ambientazioni
      for (const setting of settings) {
        await txc.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (set:Setting {id: $settingId})
           MERGE (s)-[:SET_IN {order: $order}]->(set)`,
          { storyId, settingId: setting, order: 0 }
        );
      }

      // Emozioni e temi sui frammenti
      for (const emotion of emotions) {
        await txc.run(
          `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)
           MERGE (e:Emotion {name: $emotion})
           MERGE (p)-[:EVOKES]->(e)`,
          { storyId, emotion }
        );
      }

      for (const theme of themes) {
        await txc.run(
          `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)
           MERGE (t:Theme {name: $theme})
           MERGE (p)-[:FITS_THEME]->(t)`,
          { storyId, theme }
        );
      }

      await txc.commit();
      console.log(`✓ Dettagli storia "${storyId}" aggiornati con successo`);
    } catch (error) {
      console.error('Errore in updateStoryDetails:', error);
      throw error;
    } finally {
      await session.close();
    }
  }

  /**
   * Elimina una storia
   */
  async deleteStory(storyId) {
    await this.deleteNode('Story', storyId);
  }

  /**
   * Elimina un frammento
   */
  async deletePlotFragment(fragmentId) {
    await this.deleteNode('PlotFragment', fragmentId);
  }

  /**
   * Elimina un personaggio
   */
  async deleteCharacter(characterId) {
    await this.deleteNode('Character', characterId);
  }

  /**
   * Elimina un'ambientazione
   */
  async deleteSetting(settingId) {
    await this.deleteNode('Setting', settingId);
  }
  
  /**
   * Sintetizza testo in audio MP3 con ElevenLabs
   * @param {string} text - Testo da leggere
   * @param {string} voiceId - ID voce (es. "EXAVITQu4vr4xnSDxMaL" per voce italiana femminile)
   * @returns {Promise<string>} URL pubblico del file audio
   */
  async synthesizeSpeech(text, voiceId = 'EXAVITQu4vr4xnSDxMaL') {
    const apiKey = process.env.ELEVENLABS_API_KEY;
    if (!apiKey) throw new Error('ELEVENLABS_API_KEY non configurata');

    const response = await fetch(`https://api.elevenlabs.io/v1/text-to-speech/${voiceId}/stream`, {
      method: 'POST',
      headers: {
        'Accept': 'audio/mpeg',
        'Content-Type': 'application/json',
        'xi-api-key': apiKey
      },
      body: JSON.stringify({
        text,
        model_id: 'eleven_multilingual_v2',
        voice_id: 'EXAVITQu4vr4xnSDxMaL',  // Bella - voce calda femminile
        voice_settings: {
          stability: 0.6,
          similarity_boost: 0.85
        }
      })
    });

    if (!response.ok) {
      const errorText = await response.text();
      throw new Error(`ElevenLabs error: ${response.status} - ${errorText}`);
    }

    const buffer = await response.arrayBuffer();

    // Crea cartella public/audio se non esiste
  //  const audioDir = path.join(__dirname, 'public', 'audio');
    const audioDir = "/home/dedalo/IL_PICCOLO_BARDO/www/public/audio";

    await fs.mkdir(audioDir, { recursive: true });

    const fileName = `story_${Date.now()}.mp3`;
    const filePath = path.join(audioDir, fileName);
    
    await fs.writeFile(filePath, Buffer.from(buffer));

    // Restituisci URL accessibile dal frontend
    return `/public/audio/${fileName}`;
  }
  
/**
 * Salva una storia generata nel grafo come memoria narrativa
 * @param {string} generatedStory - Testo della storia generata
 * @param {number} age - Età target
 * @param {string} userPrompt - Prompt originale
 * @returns {Promise<object>} Dettagli della storia salvata
 */
async saveGeneratedStory(generatedStory, age, userPrompt) {
  const session = this.getSession();
  try {
    const storyId = `gen_${Date.now()}`;
    const storyData = {
      id: storyId,
      title: `Storia dal Bardo: ${userPrompt.substring(0, 50)}...`,
      description: userPrompt,
      ageMin: age - 1,
      ageMax: age + 1
    };

    // Crea la storia
    await this.createStory(storyData);

    // Estrai frammenti dalla storia generata (usa LLM per splittarla)
    const fragments = await this.extractFragmentsFromStory(generatedStory, age);

    // Salva frammenti e collegamenti
    for (let i = 0; i < fragments.length; i++) {
      const fragId = `${storyId}_frag_${i}`;
      const fragData = {
        id: fragId,
        text: fragments[i].text,
        ageMin: age - 1,
        ageMax: age + 1,
        complexity: fragments[i].complexity || 'media',
        tone: fragments[i].tone || 'positivo'
      };
      await this.createPlotFragment(fragData);
      await this.linkFragmentToStory(storyId, fragId, i);

      // Genera e salva embedding per memoria semantica
      await this.generateAndSetEmbedding(fragId);
    }

    console.log(`✓ Storia generata salvata come "${storyId}" con ${fragments.length} frammenti`);
    return storyData;
  } finally {
    await session.close();
  }
}

/**
 * Estrae frammenti da una storia generata (usa LLM)
 * @param {string} storyText - Testo completo
 * @param {number} age - Età
 * @returns {Promise<array>} Array di frammenti
 */
async extractFragmentsFromStory(storyText, age) {
  try {
    const completion = await this.openai.chat.completions.create({
      model: 'gpt-4o-mini',
      messages: [{
        role: 'user',
content: `Dividi questa storia in 4-6 frammenti brevi adatti a bambini di ${age} anni.
Per ogni frammento restituisci esattamente questo formato JSON:
{
  "text": "testo del frammento (max 100 parole)",
  "complexity": "bassa" | "media" | "alta",
  "tone": "positivo" | "neutro" | "riflessivo"
}

Rispondi SOLO con un array JSON valido, senza markdown, senza blocchi di codice, senza testo extra o spiegazioni.`
      }],
      temperature: 0.3,
      max_tokens: 1000
    });

    let content = completion.choices[0].message.content.trim();

    // Rimuovi eventuali blocchi markdown 
    content = content.replace(/^```json\s*/i, '').replace(/\s*```$/i, '').trim();

    // Se fallisce ancora, prova a estrarre il JSON con regex (sicurezza extra)
    const jsonMatch = content.match(/\[[\s\S]*\]/);
    if (jsonMatch) {
      content = jsonMatch[0];
    }

    return JSON.parse(content);
  } catch (parseError) {
    console.error('Errore parsing frammenti:', parseError);
    console.log('Contenuto ricevuto:', completion.choices[0].message.content);

    // Fallback: crea frammenti manuali semplici
    const sentences = storyText.split('. ').filter(s => s.length > 20);
    return sentences.slice(0, 5).map((s, i) => ({
      text: s.trim() + (s.endsWith('.') ? '' : '.'),
      complexity: i < 2 ? 'bassa' : 'media',
      tone: 'positivo'
    }));
  }
}  
  
}

export default PiccoloBardoManager;


// Esempio di utilizzo
/*
const manager = new PiccoloBardoManager(
  'bolt://localhost:7687', 
  'neo4j', 
  'password',
  'storybook'  // Database dedicato
);

// Inizializza (crea database + struttura)
await manager.initializeDatabase();

// Crea una storia
const story = await manager.createStory({
  id: 'story_001',
  title: 'Il coraggio del cagnolino',
  description: 'Una storia sul superare le paure',
  ageMin: 3,
  ageMax: 5
});

// Costruisci una storia completa in un'unica operazione
await manager.buildCompleteStory({
  storyId: 'story_001',
  fragments: [
    { fragmentId: 'pf_001', order: 0 },
    { fragmentId: 'pf_002', order: 1 }
  ],
  characters: [
    { characterId: 'ch_dog', role: 'protagonista' }
  ],
  settingId: 'st_forest',
  emotions: ['coraggio', 'gioia'],
  themes: ['autostima', 'crescita']
});

// Ottieni statistiche
const stats = await manager.getStatistics();

// Lista elementi
const stories = await manager.listStories();
const fragments = await manager.listFragments();

// Cancella tutto
await manager.clearAllData();

// Elimina il database
await manager.dropDatabase();

await manager.close();
* 
* 
* 
* 
* 
const manager = new PiccoloBardoManager('bolt://localhost:7687', 'neo4j', 'password');

// 1. Inizializza
await manager.initializeDatabase();

// 2. Popola embeddings (una volta sola!)
await manager.populateAllEmbeddings();

// 3. Genera una storia!
const result = await manager.generateStoryFromPrompt(
  "raccontami una storia che parli di un principe che libera una principessa segregata da un drago",
  6
);

console.log(result.story);

 
*/
