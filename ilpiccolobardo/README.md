# 🐉 BALDOWEB: Sistema Agentico Narrativo per Preadolescenti

**Baldo** è un assistente virtuale (un draghetto astronomo) che genera storie personalizzate in tempo reale, integrando **dati fisici reali** (meteo e astronomia) e **memoria narrativa a lungo termine** (Graph Database).

Il sistema non si limita a rispondere a un prompt, ma "ragiona" attraverso un grafo di stati per costruire un contesto narrativo coerente, emotivo e scientificamente accurato.

---

## 🏗️ Architettura del Sistema

Il sistema è basato su un'architettura a **microservizi** orchestrata tramite Docker:

1.  **FastAPI (Backend Agentico):** Il core in Python che gestisce il grafo decisionale (**LangGraph**).
2.  **Node.js (Server principale):** Espone il sito, il WebSocket (`/ws`) e fa da proxy verso FastAPI per le rotte admin (`/api/ILPICCOLOBARDO`); include anche il servizio di calcolo astronomico (Planetarium).
3.  **Neo4j (Graph Database):** Memoria associativa per frammenti di trama (Plot Fragments), personaggi ed emozioni.
4.  **LLM Router:** Sistema di astrazione per l'utilizzo dinamico di modelli. Provider primario **Anthropic Claude Sonnet 5**, con fallback automatico su **OpenAI** in caso di errore o risposta senza testo utilizzabile.
5.  **Apache (reverse proxy):** Serve i file statici del sito direttamente da disco (`DocumentRoot` su questa cartella) e inoltra `/api` e `/ws` al servizio Node in Docker.

---

## 🧠 Il Ciclo di Ragionamento (Graph Workflow)

Quando l'utente inserisce un prompt, l'agente attraversa un workflow a stati finiti:

1.  **Analisi & RAG Extractor:** L'LLM scompone il prompt estraendo personaggi, emozioni e località geografica specifica. Le espressioni temporali relative dell'italiano parlato ("stasera", "stanotte", "ieri"...) vengono risolte in data/ora assolute usando come riferimento l'ora corrente nel fuso dell'utenza (`Europe/Rome`), non quella del container — altrimenti, di sera, "stasera" rischia di risolversi con 1-2 ore di scarto (CET/CEST) rispetto all'ora reale in Italia.
2.  **Context Fetching (Fisica):**
    * **Geo-Coding:** Converte il luogo in coordinate GPS precise (con fallback sulle coordinate del dispositivo se il geocoding fallisce).
    * **Weather API:** Recupera temperatura e condizioni meteo del luogo scelto, tradotte in una sensazione descrittiva adatta a un bambino (es. "pomeriggio caldo caldo, di quelli in cui si esce senza giacca") invece del dato numerico in gradi.
    * **Planetarium API:** Interroga il microservizio Node.js per sapere quali pianeti e corpi celesti sono visibili in quel preciso istante, inclusa la fase lunare reale (vedi [Precisione Astronomica](#-precisione-astronomica) sotto). L'ora locale della storia viene convertita in UTC prima di essere inviata al motore astronomico, che si aspetta input in UTC.
3.  **Memoria Associativa (Neo4j):** Interroga il database a grafo per recuperare i frammenti narrativi più rilevanti rispetto a personaggi, emozioni ed età del bambino (vedi [Knowledge Base](#-knowledge-base-i-frammenti-narrativi) sotto), oltre a eventuali storie precedenti con personaggi simili e alle "schede personaggio" (descrizione/tratti) già compilate a mano.
4.  **Composizione & Draft:** Un'istanza dell'LLM fonde i dati "freddi" (meteo/astro, coerenti con giorno/notte) con la magia del racconto. Il frammento più rilevante viene usato **come riferimento di stile e ritmo, non come trama da parafrasare**: la trama concreta nasce dal prompt dell'utente e dall'archetipo narrativo del frammento, non dai suoi eventi letterali. Vengono inoltre integrati: la tecnica narrativa del frammento, un ritornello ripetuto 2-3 volte nei momenti chiave (se il frammento non ne ha uno, l'LLM è istruito a inventarne uno coerente *prima* di scrivere, senza annunciarlo al bambino), e una domanda finale per coinvolgerlo.
5.  **Revisione Critica & Rifinitura:** Regole esplicite impediscono derive comuni: nessuna morale dichiarata (né da un personaggio né dal narratore), nessun dettaglio sensoriale impercettibile per l'età, nessun finale da fiaba generico ("vissero felici e contenti"), nessun Markdown nel testo (ripulito anche a livello di codice come rete di sicurezza).
6.  **Verifica di Coerenza:** Un nodo dedicato controlla che la domanda finale parli davvero di personaggi/eventi presenti nel racconto generato; se non lo è, un secondo passaggio LLM la riscrive mantenendo intatto il resto della storia (con un controllo di lunghezza a protezione da riscritture che troncano il racconto).
7.  **Streaming Output:** La storia viene trasmessa al frontend tramite eventi WebSocket.

---

## 📚 Knowledge Base: i frammenti narrativi

Il cuore della memoria di Baldo è la collezione di **Plot Fragment** su Neo4j. Ogni frammento ha questa struttura:

| Campo | Obbligatorio | Usato per... |
| :--- | :--- | :--- |
| `id` | ✅ | Chiave univoca del frammento |
| `text` | ✅ | Il testo che finisce nella storia generata |
| `setting` | ✅ | Ambientazione — fallback di ricerca se nessun personaggio/emozione combacia |
| `characters` | opzionale | Personaggi coinvolti — determinano quali frammenti Baldo sceglie |
| `emotions` | opzionale | Emozioni evocate — stesso ruolo di `characters` nella scelta |
| `fascia_eta` | opzionale | Es. `"3-6"` — dà una leggera preferenza ai frammenti adatti all'età del bambino (non esclude gli altri) |
| `tecnica_narrativa` | opzionale | Tecnica narrativa applicata dal frammento (es. `binomio_fantastico`, tecniche alla Rodari, alla Grimm, o originali) — Baldo la riusa per costruire la storia |
| `ritornello` | opzionale | Frase breve e orecchiabile ripetuta 2-3 volte nella storia. Se assente sul frammento scelto, Baldo ne inventa uno coerente con l'archetipo |
| `domanda` | opzionale | Domanda di chiusura proposta al bambino a fine storia |
| `archetipo` | opzionale | Archetipo narrativo (es. `viaggio_e_ritorno`, `piccolo_supera_grande`) — usato per guidare la trama nuova, non solo come metadato |
| `tema` | opzionale | Metadato descrittivo per organizzare l'archivio (non influenza la generazione) |

Tutti questi campi sono presi **solo dal frammento con il punteggio più alto** restituito dalla ricerca su Neo4j (mai da "il primo non-nullo trovato nella lista"), per evitare di mescolare ritornello/tecnica/domanda di frammenti diversi in una stessa storia.

I **personaggi** (`Character`) sono identificati per **nome** (non hanno un `id` separato) e possono avere `description`/`traits` compilati a mano: se presenti, Baldo li usa per restare coerente con la personalità del personaggio tra una storia e l'altra.

### Gestione della Knowledge Base

Due pannelli, entrambi protetti da HTTP Basic Auth via Apache:

- **`/loader/`** — "Archivio dei Frammenti": carica in blocco un file JSON (`{"sovrascrivi": bool, "frammenti": [...]}`), lista, elimina. È il modo più veloce per popolare o aggiornare molti frammenti insieme.
- **`/admin/manager.html`** — pannello con tab **Frammenti** (editor singolo frammento, stesso schema di `/loader/`) e **Personaggi** (descrizione/tratti).

Entrambi passano per lo stesso endpoint FastAPI `POST /admin/frammenti/bulk` (upsert per `id`, con `sovrascrivi` per decidere se un frammento esistente va aggiornato o saltato).

---

## 🌙 Precisione Astronomica

Il cielo descritto nella storia riflette il cielo reale per il luogo, la data e l'ora indicati (o dedotti) dal prompt:

- **Fase lunare corretta:** calcolata normalizzando l'elongazione della Luna in un angolo di fase [0°, 360°) — 0°=nuova, 90°=primo quarto, 180°=piena, 270°=ultimo quarto — anziché usare la differenza grezza di ascensione retta, che non distingue crescente da calante e su circa metà delle configurazioni reali non ricadeva nel range atteso.
- **Luna nuova non "visibile":** una Luna nuova, anche se geometricamente sopra l'orizzonte, è troppo vicina al Sole nel cielo per essere vista a occhio nudo — viene quindi esclusa dagli oggetti visibili indipendentemente dalla sua altezza.
- **Conversione locale → UTC:** il motore astronomico C++ si aspetta un istante in UTC ma non lo converte da solo. Data e ora della storia (in ora locale `Europe/Rome`, con gestione automatica dell'ora legale) vengono convertite in UTC prima della chiamata — altrimenti il cielo calcolato risulterebbe sfasato di 1-2 ore rispetto a quello reale.
- **Giorno vs notte:** la descrizione del cielo distingue esplicitamente le due situazioni — niente stelle "che brillano nel cielo azzurro" di giorno, niente sole a mezzanotte — applicata in modo coerente per tutta la storia, domanda finale inclusa.

---

## 🛠️ Stack Tecnologico

| Componente | Tecnologia |
| :--- | :--- |
| **Orchestratore Agentico** | Python / LangGraph |
| **Backend API** | FastAPI |
| **Graph Database** | Neo4j (Cypher) |
| **LLM** | Anthropic Claude Sonnet 5 (primario), OpenAI (fallback) |
| **Servizi Esterni** | OpenWeatherMap |
| **Astronomia** | Node.js / Custom Engine (C++) |
| **Containerizzazione** | Docker & Docker Compose |
| **Reverse Proxy** | Apache (file statici + proxy verso Docker) |

---

## 🚦 Installazione e Avvio

1. **Clona il repository:**
   ```bash
   git clone https://github.com/sebaldar/baldoweb.git
   cd BALDOWEB
   ```

2. **Configura le variabili d'ambiente** in `.env` (vedi le chiavi richieste da `docker-compose.yml`: credenziali Neo4j/MariaDB/Mongo, `ANTHROPIC_API_KEY` (provider primario), `OPENAI_API_KEY` (fallback), ecc.).

3. **Avvia i servizi:**
   ```bash
   docker compose build
   docker compose up -d
   ```

4. **Popola la Knowledge Base** caricando un JSON di frammenti dal pannello `/loader/` (credenziali create con `htpasswd`, vedi configurazione Apache).
