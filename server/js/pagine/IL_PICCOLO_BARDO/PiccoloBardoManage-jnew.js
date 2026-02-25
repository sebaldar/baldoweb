import fs from 'fs/promises';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

import { OpenAI } from 'openai';
import neo4jService, { int } from '../../services/service.neo4j.js';

/**
 * Il Piccolo Bardo - Neo4j Story Database Manager
 * Gestisce una collezione di storie per bambini in età prescolare
 * Database dedicato: storybook
 */
class PiccoloBardoManager {
  
  constructor(database = 'storybook') {
    this.database = database;

    // Inizializza OpenAI
    this.openai = new OpenAI({
      apiKey: process.env.OPENAI_API_KEY,
    });
  }

  // === METODI DI CONVENIENZA PER SESSIONI ===
  getSession() {
    return neo4jService.getSession(this.database);
  }

  getReadSession() {
    return neo4jService.getReadSession(this.database);
  }

  // === CREAZIONE E GESTIONE DATABASE ===
  async createDatabase() {
    // Usa il database 'system' per operazioni amministrative
    const systemSession = neo4jService.getSession('system');
    try {
      const result = await systemSession.run(
        'SHOW DATABASES WHERE name = $name',
        { name: this.database }
      );

      if (result.records.length === 0) {
        await systemSession.run(`CREATE DATABASE ${this.database} IF NOT EXISTS`);
        console.log(`✓ Database "${this.database}" creato con successo`);
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

  async waitForDatabase(maxAttempts = 10, delayMs = 1000) {
    const systemSession = neo4jService.getSession('system');
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

  async initializeDatabase() {
    await this.createDatabase();

    await neo4jService.withTransaction(async (tx) => {
      console.log('Inizializzazione constraints e indici...');

      await tx.run('CREATE CONSTRAINT story_id IF NOT EXISTS FOR (s:Story) REQUIRE s.id IS UNIQUE');
      await tx.run('CREATE CONSTRAINT plot_id IF NOT EXISTS FOR (p:PlotFragment) REQUIRE p.id IS UNIQUE');
      await tx.run('CREATE CONSTRAINT char_id IF NOT EXISTS FOR (c:Character) REQUIRE c.id IS UNIQUE');
      await tx.run('CREATE CONSTRAINT setting_id IF NOT EXISTS FOR (s:Setting) REQUIRE s.id IS UNIQUE');
      await tx.run('CREATE CONSTRAINT emotion_name IF NOT EXISTS FOR (e:Emotion) REQUIRE e.name IS UNIQUE');
      await tx.run('CREATE CONSTRAINT theme_name IF NOT EXISTS FOR (t:Theme) REQUIRE t.name IS UNIQUE');

      await tx.run('CREATE INDEX plot_age IF NOT EXISTS FOR (p:PlotFragment) ON (p.ageMin, p.ageMax)');
      await tx.run('CREATE INDEX plot_complexity IF NOT EXISTS FOR (p:PlotFragment) ON (p.complexity)');
      await tx.run('CREATE INDEX story_age IF NOT EXISTS FOR (s:Story) ON (s.ageMin, s.ageMax)');

      await tx.run(`
        CREATE VECTOR INDEX fragment_embeddings IF NOT EXISTS
        FOR (f:PlotFragment) ON (f.embedding)
        OPTIONS { 
          indexConfig: { 
            \`vector.dimensions\`: 1536,
            \`vector.similarity_function\`: 'cosine'
          }
        }
      `);
    }, this.database);

    console.log('✓ Database storybook inizializzato con successo (incluso vector index)');
  }

  async clearAllData() {
    await neo4jService.run('MATCH (n) DETACH DELETE n', {}, this.database);
    console.log('✓ Tutti i dati cancellati dal database storybook');
  }

  async dropDatabase() {
    const systemSession = neo4jService.getSession('system');
    try {
      await systemSession.run(`DROP DATABASE ${this.database} IF EXISTS`);
      console.log(`✓ Database "${this.database}" eliminato`);
    } finally {
      await systemSession.close();
    }
  }

  // === OPERAZIONI CRUD BASE ===
  /**
   * Crea una nuova storia
   */
  async createStory(storyData) {
    const result = await neo4jService.run(
      `CREATE (s:Story {
        id: $id,
        title: $title,
        description: $description,
        ageMin: $ageMin,
        ageMax: $ageMax,
        createdAt: datetime()
      }) RETURN s`,
      storyData,
      this.database
    );
    const props = result[0]?.s.properties;
    console.log(`✓ Storia creata: ${storyData.title}`);
    return props;
  }

/**
 * Crea un nuovo frammento di trama
 */
  async createPlotFragment(fragmentData) {
    const params = {
      id: fragmentData.id,
      text: fragmentData.text || fragmentData.testo || "",
      ageMin: fragmentData.ageMin || fragmentData.etaMin || fragmentData.eta_minima || 4,
      ageMax: fragmentData.ageMax || fragmentData.etaMax || fragmentData.eta_massima || 10,
      complexity: fragmentData.complexity || fragmentData.complessita || fragmentData.complessità || 1,
      tone: fragmentData.tone || fragmentData.tono || "neutro",
      embedding: fragmentData.embedding || null
    };

    const result = await neo4jService.run(
      `CREATE (p:PlotFragment $props) RETURN p`,
      { props: params },
      this.database
    );

    const created = result[0]?.p.properties;
    console.log(`✓ Frammento creato: ${created.id} - "${created.text.substring(0, 50)}..."`);
    return created;
  }


  /**
   * Crea un personaggio
   */
  async createCharacter(charData) {
    const result = await neo4jService.run(
      `CREATE (c:Character $props) RETURN c`,
      { props: charData },
      this.database
    );
    console.log(`✓ Personaggio creato: ${charData.name}`);
    return result[0]?.c.properties;
  }


  /**
   * Crea un'ambientazione
   */
  async createSetting(settingData) {
    const result = await neo4jService.run(
      `CREATE (s:Setting $props) RETURN s`,
      { props: settingData },
      this.database
    );
    console.log(`✓ Ambientazione creata: ${settingData.name}`);
    return result[0]?.s.properties;
  }

  /**
   * Crea o recupera un'emozione
   */
  async createOrGetEmotion(emotionName) {
    return await neo4jService.run(
      `MERGE (e:Emotion {name: $name}) RETURN e`,
      { name: emotionName },
      this.database
    );
  }

  /**
   * Crea o recupera un tema
   */
  async createOrGetTheme(themeName) {
    return await neo4jService.run(
      `MERGE (t:Theme {name: $name}) RETURN t`,
      { name: themeName },
      this.database
    );
  }

  // === RELAZIONI ===
  /**
   * Collega un frammento a una storia
   */
  async linkFragmentToStory(storyId, fragmentId, order = 0) {
    await neo4jService.run(
      `MATCH (s:Story {id: $storyId})
       MATCH (p:PlotFragment {id: $fragmentId})
       MERGE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
      { storyId, fragmentId, order },
      this.database
    );
  }

  /**
   * Collega un frammento a un'emozione
   */
  async linkFragmentToEmotion(fragmentId, emotionName) {
    await neo4jService.run(
      `MATCH (p:PlotFragment {id: $fragmentId})
       MERGE (e:Emotion {name: $emotionName})
       MERGE (p)-[:EVOKES]->(e)`,
      { fragmentId, emotionName },
      this.database
    );
  }

  /**
   * Collega un frammento a un tema
   */
  async linkFragmentToTheme(fragmentId, themeName) {
    await neo4jService.run(
      `MATCH (p:PlotFragment {id: $fragmentId})
       MERGE (t:Theme {name: $themeName})
       MERGE (p)-[:FITS_THEME]->(t)`,
      { fragmentId, themeName },
      this.database
    );
  }

  /**
   * Collega due frammenti in sequenza
   */
  async linkFragmentSequence(fromFragmentId, toFragmentId, weight = 1.0) {
    await neo4jService.run(
      `MATCH (p1:PlotFragment {id: $fromId})
       MATCH (p2:PlotFragment {id: $toId})
       MERGE (p1)-[:CAN_FOLLOW {weight: $weight}]->(p2)`,
      { fromId: fromFragmentId, toId: toFragmentId, weight },
      this.database
    );
  }

  /**
   * Collega un personaggio a una storia
   */
  async linkCharacterToStory(storyId, characterId, role = 'protagonista') {
    await neo4jService.run(
      `MATCH (s:Story {id: $storyId})
       MATCH (c:Character {id: $characterId})
       MERGE (s)-[:HAS_CHARACTER {role: $role}]->(c)`,
      { storyId, characterId, role },
      this.database
    );
  }

  /**
   * Collega un'ambientazione a una storia
   */
  async linkSettingToStory(storyId, settingId, order = 0) {
    await neo4jService.run(
      `MATCH (s:Story {id: $storyId})
       MATCH (set:Setting {id: $settingId})
       MERGE (s)-[:SET_IN {order: $order}]->(set)`,
      { storyId, settingId, order },
      this.database
    );
  }

  /**
   * Collega un'ambientazione a un frammento specifico
   */
  async linkFragmentToSetting(fragmentId, settingId) {
    await neo4jService.run(
      `MATCH (p:PlotFragment {id: $fragmentId})
       MATCH (set:Setting {id: $settingId})
       MERGE (p)-[:TAKES_PLACE_IN]->(set)`,
      { fragmentId, settingId },
      this.database
    );
  }

  // === ALTRE OPERAZIONI (list, search, stats, ecc.) ===
  async listStories() {
    return await neo4jService.runRead(
      'MATCH (s:Story) RETURN s ORDER BY s.createdAt DESC',
      {},
      this.database
    );
  }

  async listFragments() {
    return await neo4jService.runRead(
      'MATCH (p:PlotFragment) RETURN p ORDER BY p.id',
      {},
      this.database
    );
  }

  async listCharacters() {
    return await neo4jService.runRead(
      'MATCH (c:Character) RETURN c ORDER BY c.name',
      {},
      this.database
    );
  }

  async listSettings() {
    return await neo4jService.runRead(
      'MATCH (s:Setting) RETURN s ORDER BY s.name',
      {},
      this.database
    );
  }

  async getFragmentsByAgeAndTheme(ageMin, ageMax, themeName, limit = 10) {
    return await neo4jService.runRead(
      `MATCH (p:PlotFragment)-[:FITS_THEME]->(t:Theme {name: $theme})
       WHERE p.ageMin <= $ageMax AND p.ageMax >= $ageMin
       RETURN p
       LIMIT $limit`,
      { theme: themeName, ageMin: int(ageMin), ageMax: int(ageMax), limit: int(limit) },
      this.database
    );
  }

  async getNextFragments(fragmentId, limit = 5) {
    return await neo4jService.runRead(
      `MATCH (p1:PlotFragment {id: $fragmentId})-[r:CAN_FOLLOW]->(p2:PlotFragment)
       RETURN p2, r.weight as weight
       ORDER BY weight DESC
       LIMIT $limit`,
      { fragmentId, limit: int(limit) },
      this.database
    );
  }

  async generateAndSetEmbedding(fragmentId) {
    const [fragment] = await neo4jService.runRead(
      `MATCH (p:PlotFragment {id: $id}) RETURN p.text AS text`,
      { id: fragmentId },
      this.database
    );

    if (!fragment || !fragment.text?.trim()) return;

    const response = await this.openai.embeddings.create({
      model: 'text-embedding-3-small',
      input: fragment.text
    });

    const embedding = response.data[0].embedding;

    await neo4jService.run(
      `MATCH (p:PlotFragment {id: $id}) SET p.embedding = $embedding`,
      { id: fragmentId, embedding },
      this.database
    );

    console.log(`✓ Embedding generato e salvato per frammento: ${fragmentId}`);
  }

  async populateAllEmbeddings() {
    const fragments = await neo4jService.runRead(
      `MATCH (p:PlotFragment) 
       WHERE p.embedding IS NULL OR size(p.embedding) = 0
       RETURN p.id AS id`,
      {},
      this.database
    );

    const ids = fragments.map(r => r.id);
    console.log(`Trovati ${ids.length} frammenti senza embedding.`);

    for (const id of ids) {
      await this.generateAndSetEmbedding(id);
      await new Promise(r => setTimeout(r, 100));
    }
    console.log('✓ Tutti gli embedding popolati!');
  }

  async vectorSearch(queryText, limit = 10) {
    const response = await this.openai.embeddings.create({
      model: 'text-embedding-3-small',
      input: queryText
    });
    const queryEmbedding = response.data[0].embedding;

    return await neo4jService.runRead(
      `CALL db.index.vector.queryNodes('fragment_embeddings', $limit, $queryEmbedding)
       YIELD node, score
       RETURN node.id AS id, node.text AS text, score
       ORDER BY score DESC`,
      { limit: int(limit), queryEmbedding },
      this.database
    );
  }

  /**
   * Genera una storia con RAG basata su un prompt utente
   */
  async generateStoryFromPrompt(userPrompt, age = 5) {
    await neo4jService.connect(); // garantisce connessione (idempotente)
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

  async getStoryComplete(storyId) {
    const result = await neo4jService.runRead(
      `MATCH (s:Story {id: $storyId})

       OPTIONAL MATCH (s)-[hf:HAS_FRAGMENT]->(p:PlotFragment)
       OPTIONAL MATCH (p)-[:EVOKES]->(e:Emotion)
       OPTIONAL MATCH (p)-[:FITS_THEME]->(t:Theme)
       OPTIONAL MATCH (p)-[:TAKES_PLACE_IN]->(setFrag:Setting)

       OPTIONAL MATCH (s)-[hc:HAS_CHARACTER]->(c:Character)
       OPTIONAL MATCH (s)-[hs:SET_IN]->(setStory:Setting)

       WITH s, p, hf, c, hc, setStory, hs,
            collect(DISTINCT e.name) AS fragEmotions,
            collect(DISTINCT t.name) AS fragThemes,
            collect(DISTINCT properties(setFrag)) AS fragSettings

       WITH s,
            collect({
              fragment: properties(p),
              order: coalesce(hf.order, 0),
              emotions: fragEmotions,
              themes: fragThemes,
              settings: fragSettings
            }) AS fragments,
            collect(CASE WHEN c IS NOT NULL THEN {character: properties(c), role: hc.role} END) AS characters,
            collect(CASE WHEN setStory IS NOT NULL THEN {setting: properties(setStory), order: hs.order} END) AS settings

       RETURN
         properties(s) AS story,
         fragments,
         characters,
         settings`,
      { storyId },
      this.database
    );

    if (result.length === 0) return null;

    const record = result[0];

    return {
      story: record.story,
      fragments: record.fragments
        .filter(f => f.fragment?.id)
        .sort((a, b) => a.order - b.order)
        .map(f => ({
          id: f.fragment.id,
          text: f.fragment.text || '(testo mancante)',
          ageMin: f.fragment.ageMin,
          ageMax: f.fragment.ageMax,
          complexity: f.fragment.complexity || 1,
          tone: f.fragment.tone || 'neutro',
          order: f.order,
          emotions: f.emotions || [],
          themes: f.themes || [],
          settings: f.settings || []
        })),
      characters: record.characters
        .filter(c => c !== null)
        .map(c => ({
          id: c.character.id,
          name: c.character.name,
          description: c.character.description || '',
          traits: c.character.traits || [],
          role: c.role || 'secondario'
        })),
      settings: record.settings
        .filter(s => s !== null)
        .sort((a, b) => (a.order ?? 0) - (b.order ?? 0))
        .map(s => s.setting)
    };
  }

  async saveGeneratedStory(generatedStory, age, userPrompt) {
    const storyId = `gen_${Date.now()}`;
    const storyData = {
      id: storyId,
      title: `Storia dal Bardo: ${userPrompt.substring(0, 50)}${userPrompt.length > 50 ? '...' : ''}`,
      description: userPrompt,
      ageMin: Math.max(3, age - 1),
      ageMax: Math.min(10, age + 1)
    };

    // Crea la storia
    await this.createStory(storyData);

    // Estrai frammenti con LLM
    const fragments = await this.extractFragmentsFromStory(generatedStory, age);

    // Crea frammenti, collegamenti e embedding in transazione
    await neo4jService.withTransaction(async (tx) => {
      for (let i = 0; i < fragments.length; i++) {
        const fragId = `${storyId}_frag_${i}`;
        const fragData = {
          id: fragId,
          text: fragments[i].text.trim(),
          ageMin: storyData.ageMin,
          ageMax: storyData.ageMax,
          complexity: fragments[i].complexity || 'media',
          tone: fragments[i].tone || 'positivo',
          embedding: null  // sarà popolato dopo
        };

        await tx.run(
          `CREATE (p:PlotFragment $props)`,
          { props: fragData }
        );

        await tx.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (p:PlotFragment {id: $fragId})
           CREATE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
          { storyId, fragId, order: i }
        );
      }
    }, this.database);

    // Genera embedding per ogni frammento (fuori transazione per non bloccare troppo a lungo)
    for (let i = 0; i < fragments.length; i++) {
      const fragId = `${storyId}_frag_${i}`;
      await this.generateAndSetEmbedding(fragId);
      await new Promise(r => setTimeout(r, 50)); // gentile con rate limit
    }

    console.log(`✓ Storia generata salvata come "${storyId}" con ${fragments.length} frammenti e embedding`);
    return { storyId, ...storyData };
  }

  async updateStoryDetails(data) {
    const {
      storyId,
      fragments = [],     // può essere: ['frag1', 'frag2'] oppure [{ fragmentId: 'frag1', settingId: 'set1' }, ...]
      characters = [],
      settings = [],
      emotions = [],
      themes = []
    } = data;

    if (!storyId) {
      throw new Error('storyId è obbligatorio per updateStoryDetails');
    }

    await neo4jService.withTransaction(async (tx) => {
      // 1. Rimuovi TUTTI i vecchi collegamenti relativi alla storia
      await tx.run(
        `MATCH (s:Story {id: $storyId})
         // Rimuovi relazioni dirette dalla storia
         OPTIONAL MATCH (s)-[r:HAS_FRAGMENT|HAS_CHARACTER|SET_IN]->() DELETE r
         // Rimuovi relazioni EVOKES e FITS_THEME dai suoi frammenti
         OPTIONAL MATCH (s)-[:HAS_FRAGMENT]->(p:PlotFragment)-[re:EVOKES|FITS_THEME]->() DELETE re
         // Rimuovi anche eventuali TAKES_PLACE_IN dai frammenti (se vuoi pulire tutto)
         OPTIONAL MATCH (s)-[:HAS_FRAGMENT]->(p:PlotFragment)-[rs:TAKES_PLACE_IN]->() DELETE rs`,
        { storyId }
      );

      // 2. Ricrea i collegamenti per i frammenti
      for (let i = 0; i < fragments.length; i++) {
        let fragmentId, settingId;

        if (typeof fragments[i] === 'string') {
          fragmentId = fragments[i];
        } else if (typeof fragments[i] === 'object' && fragments[i].fragmentId) {
          fragmentId = fragments[i].fragmentId;
          settingId = fragments[i].settingId || null;
        } else {
          console.warn(`Frammento non valido ignorato:`, fragments[i]);
          continue;
        }

        // Collega il frammento alla storia con ordine
        await tx.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (p:PlotFragment {id: $fragmentId})
           MERGE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
          { storyId, fragmentId, order: i }
        );

        // Se specificata un'ambientazione specifica per questo frammento
        if (settingId) {
          await tx.run(
            `MATCH (p:PlotFragment {id: $fragmentId})
             MATCH (set:Setting {id: $settingId})
             MERGE (p)-[:TAKES_PLACE_IN]->(set)`,
            { fragmentId, settingId }
          );
        }
      }

      // 3. Emozioni globali (applicate a tutti i frammenti della storia)
      for (const emotion of emotions) {
        await tx.run(
          `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)
           MERGE (e:Emotion {name: $emotion})
           MERGE (p)-[:EVOKES]->(e)`,
          { storyId, emotion }
        );
      }

      // 4. Temi globali
      for (const theme of themes) {
        await tx.run(
          `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)
           MERGE (t:Theme {name: $theme})
           MERGE (p)-[:FITS_THEME]->(t)`,
          { storyId, theme }
        );
      }

      // 5. Personaggi
      for (const char of characters) {
        const characterId = typeof char === 'string' ? char : char.characterId || char.id;
        const role = typeof char === 'object' ? (char.role || 'secondario') : 'secondario';

        await tx.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (c:Character {id: $characterId})
           MERGE (s)-[:HAS_CHARACTER {role: $role}]->(c)`,
          { storyId, characterId, role }
        );
      }

      // 6. Ambientazioni generali della storia
      for (const setting of settings) {
        let settingId, order = 0;

        if (typeof setting === 'string') {
          settingId = setting;
        } else if (typeof setting === 'object') {
          settingId = setting.settingId || setting.id;
          order = setting.order ?? 0;
        }

        if (settingId) {
          await tx.run(
            `MATCH (s:Story {id: $storyId})
             MATCH (set:Setting {id: $settingId})
             MERGE (s)-[:SET_IN {order: $order}]->(set)`,
            { storyId, settingId, order }
          );
        }
      }
    }, this.database);

    console.log(`✓ Dettagli della storia "${storyId}" aggiornati con successo`);
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

  async buildCompleteStory(buildData) {
    const {
      storyId,
      fragments = [],
      characters = [],
      settings = [],
      emotions = [],
      themes = []
    } = buildData;

    if (!storyId) {
      throw new Error('storyId è obbligatorio per buildCompleteStory');
    }

    await neo4jService.withTransaction(async (tx) => {
      // 1. Collega i frammenti con ordine e eventuali ambientazioni specifiche
      for (const frag of fragments) {
        await tx.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (p:PlotFragment {id: $fragmentId})
           MERGE (s)-[:HAS_FRAGMENT {order: $order}]->(p)`,
          { storyId, fragmentId: frag.fragmentId, order: frag.order ?? 0 }
        );

        if (frag.settingId) {
          await tx.run(
            `MATCH (p:PlotFragment {id: $fragmentId})
             MATCH (set:Setting {id: $settingId})
             MERGE (p)-[:TAKES_PLACE_IN]->(set)`,
            { fragmentId: frag.fragmentId, settingId: frag.settingId }
          );
        }
      }

      // 2. Emozioni globali applicate a tutti i frammenti della storia
      if (emotions.length > 0) {
        for (const emotion of emotions) {
          await tx.run(
            `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)
             MERGE (e:Emotion {name: $emotion})
             MERGE (p)-[:EVOKES]->(e)`,
            { storyId, emotion }
          );
        }
      }

      // 3. Temi globali applicati a tutti i frammenti
      if (themes.length > 0) {
        for (const theme of themes) {
          await tx.run(
            `MATCH (s:Story {id: $storyId})-[:HAS_FRAGMENT]->(p:PlotFragment)
             MERGE (t:Theme {name: $theme})
             MERGE (p)-[:FITS_THEME]->(t)`,
            { storyId, theme }
          );
        }
      }

      // 4. Personaggi
      for (const char of characters) {
        await tx.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (c:Character {id: $characterId})
           MERGE (s)-[:HAS_CHARACTER {role: $role}]->(c)`,
          { storyId, characterId: char.characterId, role: char.role ?? 'secondario' }
        );
      }

      // 5. Ambientazioni generali della storia
      for (const setting of settings) {
        await tx.run(
          `MATCH (s:Story {id: $storyId})
           MATCH (set:Setting {id: $settingId})
           MERGE (s)-[:SET_IN {order: $order}]->(set)`,
          { storyId, settingId: setting.settingId || setting, order: setting.order ?? 0 }
        );
      }
    }, this.database);

    console.log(`✓ Storia completa "${storyId}" costruita con successo`);
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
