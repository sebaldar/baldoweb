# 🐉 BALDOWEB: Sistema Agentico Narrativo per Preadolescenti

**Baldo** è un assistente virtuale (un draghetto astronomo) che genera storie personalizzate in tempo reale, integrando **dati fisici reali** (meteo e astronomia) e **memoria narrativa a lungo termine** (Graph Database).

Il sistema non si limita a rispondere a un prompt, ma "ragiona" attraverso un grafo di stati per costruire un contesto narrativo coerente, emotivo e scientificamente accurato.

---

## 🏗️ Architettura del Sistema

Il sistema è basato su un'architettura a **microservizi** orchestrata tramite Docker:

1.  **FastAPI (Backend Agentico):** Il core in Python che gestisce il grafo decisionale (**LangGraph**).
2.  **Node.js (Server principale):** Espone il sito, il WebSocket (`/ws`) e fa da proxy verso FastAPI per le rotte admin (`/api/ILPICCOLOBARDO`); include anche il servizio di calcolo astronomico (Planetarium).
3.  **Neo4j (Graph Database):** Memoria associativa per frammenti di trama (Plot Fragments), personaggi ed emozioni.
4.  **LLM Router:** Sistema di astrazione per l'utilizzo dinamico di modelli (OpenAI/Anthropic) per analisi, validazione e generazione.
5.  **Apache (reverse proxy):** Serve i file statici del sito direttamente da disco (`DocumentRoot` su questa cartella) e inoltra `/api` e `/ws` al servizio Node in Docker.

---

## 🧠 Il Ciclo di Ragionamento (Graph Workflow)

Quando l'utente inserisce un prompt, l'agente attraversa un workflow a stati finiti:

1.  **Analisi & RAG Extractor:** L'LLM scompone il prompt estraendo personaggi, emozioni e località geografica specifica.
2.  **Context Fetching (Fisica):**
    * **Geo-Coding:** Converte il luogo in coordinate GPS precise.
    * **Weather API:** Recupera temperatura e condizioni meteo attuali del luogo scelto.
    * **Planetarium API:** Interroga il microservizio Node.js per sapere quali pianeti e corpi celesti sono visibili in quel preciso istante.
3.  **Memoria Associativa (Neo4j):** Interroga il database a grafo per recuperare i frammenti narrativi più rilevanti rispetto a personaggi, emozioni ed età del bambino (vedi [Knowledge Base](#-knowledge-base-i-frammenti-narrativi) sotto), oltre a eventuali storie precedenti con personaggi simili.
4.  **Composizione & Draft:** Un'istanza dell'LLM fonde i dati "freddi" (meteo/astro) con la magia del racconto, integrando anche la tecnica narrativa del frammento scelto, le eventuali "schede personaggio" (descrizione/tratti) e una domanda finale per coinvolgere il bambino.
5.  **Revisione Critica & Rifinitura:** Un nodo di controllo verifica la coerenza pedagogica e stilistica prima dell'invio.
6.  **Streaming Output:** La storia viene trasmessa al frontend tramite eventi WebSocket.

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
| `domanda` | opzionale | Domanda di chiusura proposta al bambino a fine storia |
| `tema` / `archetipo` | opzionale | Metadati descrittivi per organizzare l'archivio (non influenzano la generazione) |

I **personaggi** (`Character`) sono identificati per **nome** (non hanno un `id` separato) e possono avere `description`/`traits` compilati a mano: se presenti, Baldo li usa per restare coerente con la personalità del personaggio tra una storia e l'altra.

### Gestione della Knowledge Base

Due pannelli, entrambi protetti da HTTP Basic Auth via Apache:

- **`/loader/`** — "Archivio dei Frammenti": carica in blocco un file JSON (`{"sovrascrivi": bool, "frammenti": [...]}`), lista, elimina. È il modo più veloce per popolare o aggiornare molti frammenti insieme.
- **`/admin/manager.html`** — pannello con tab **Frammenti** (editor singolo frammento, stesso schema di `/loader/`) e **Personaggi** (descrizione/tratti).

Entrambi passano per lo stesso endpoint FastAPI `POST /admin/frammenti/bulk` (upsert per `id`, con `sovrascrivi` per decidere se un frammento esistente va aggiornato o saltato).

---

## 🛠️ Stack Tecnologico

| Componente | Tecnologia |
| :--- | :--- |
| **Orchestratore Agentico** | Python / LangGraph |
| **Backend API** | FastAPI |
| **Graph Database** | Neo4j (Cypher) |
| **Servizi Esterni** | OpenAI API, OpenWeatherMap |
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

2. **Configura le variabili d'ambiente** in `.env` (vedi le chiavi richieste da `docker-compose.yml`: credenziali Neo4j/MariaDB/Mongo, `OPENAI_API_KEY`, ecc.).

3. **Avvia i servizi:**
   ```bash
   docker compose build
   docker compose up -d
   ```

4. **Popola la Knowledge Base** caricando un JSON di frammenti dal pannello `/loader/` (credenziali create con `htpasswd`, vedi configurazione Apache).
