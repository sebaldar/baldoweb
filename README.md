Funzionalità principali – Il Piccolo Bardo
**Gestione database Neo4j dedicato (storybook)**

	Creazione automatica del database Neo4j se non esiste

	Attesa dello stato online prima dell’uso

	Inizializzazione completa dello schema:

	Constraint di unicità (Story, PlotFragment, Character, Setting, Emotion, Theme)

	Indici classici su età e complessità

	Vector Index per embedding semantici (cosine similarity, 1536 dimensioni)

	Cancellazione dati o eliminazione completa del database

**Modellazione narrativa a grafo**
Entità gestite

	Story – storia completa

	PlotFragment – frammenti narrativi modulari

	Character – personaggi

	Setting – ambientazioni

	Emotion – emozioni evocate

	Theme – temi narrativi

Relazioni semantiche

	HAS_FRAGMENT (con ordine)

	CAN_FOLLOW (sequenze narrative con peso)

	HAS_CHARACTER (con ruolo)

	SET_IN

	TAKES_PLACE_IN

	EVOKES

	FITS_THEME

**Creazione e gestione contenuti**
CRUD completo

Creazione, aggiornamento ed eliminazione di:

	storie

	frammenti

	personaggi

	ambientazioni

	Collegamento dinamico tra entità

	Aggiornamento strutturato dei dettagli di una storia (reset + rebuild)

**Costruzione storie complesse (transaction-safe)**

Metodo buildCompleteStory():

	collega frammenti ordinati

	associa personaggi, ambientazioni

	applica emozioni e temi globali

	tutto in un’unica transazione Neo4j

**Ricerca e interrogazione**
Query strutturate

Frammenti per:

	età

	tema

	emozione

	Frammenti successivi possibili (CAN_FOLLOW)

	Ricerca storie per:

	età

	tema

	emozione

	personaggio

	Recupero storia completa con:

	frammenti ordinati

	emozioni e temi per frammento

	personaggi con ruolo

	ambientazioni

**Memoria semantica (RAG)**
	Embedding OpenAI

	Generazione embedding per frammenti narrativi

	Popolamento batch degli embedding mancanti

	Vector search su Neo4j:

	query semantiche

	ranking per similarità

	✨ Generazione storie con AI (RAG + LLM)
	Pipeline completa

	Prompt utente

	Vector search semantica

	Estrazione automatica di:

	temi

	emozioni

	Recupero frammenti coerenti per età

	Prompt di sistema narrativo dedicato (Il Piccolo Bardo)

	Generazione storia originale (non copia)

	Salvataggio automatico nel grafo come memoria narrativa

**Auto-apprendimento**

Le storie generate:

	vengono spezzate in frammenti

	indicizzate semanticamente

	riutilizzabili per storie future
	→ memoria narrativa incrementale

**Generazione immagini AI**

	Estrazione automatica di una scena illustrabile

	Prompt visivo strutturato (stile libro per bambini)

	Generazione immagine con OpenAI Image API

	Salvataggio su filesystem pubblico

	Restituzione URL frontend-ready

**Sintesi vocale (audiostorie)**

	Text-to-speech automatico

	Output MP3
	
___	

	Salvataggio in cartella pubblica

	URL pronto per riproduzione frontend

	Statistiche

	Conteggio globale di:

	storie

	frammenti

	personaggi

	ambientazioni

	emozioni

	temi

	Robustezza e sicurezza

	Uso estensivo di:

	transazioni

	fallback LLM

	parsing JSON difensivo

	mapping parametri tollerante

	Logging dettagliato per debugging
---

## Architettura

```
IlPiccoloBardo/

├── www/             # Document root (index.html, assets, css, js)
├── server/             # server websocket
└── README.md
```

### Componenti principali

* **Frontend**: HTML / CSS / JavaScript
* **Backend nodejs**: gestione contenuti, form, email
* **Node.js**: server WebSocket per upload o comunicazioni realtime
* **Apache2**: reverse proxy verso Node.js e servizio HTTP
* **neo4j**: database a struttura Grafo

---

## Requisiti

### Ambiente 

* Apache2

### Ambiente Node.js (WebSocket)

* Node.js >= 18
* npm 

---



## 🔌 WebSocket (Node.js)

Il server WebSocket è utilizzato per funzionalità realtime (es. upload file, notifiche, streaming dati).

Avvio in sviluppo:

```bash
cd server
./start

```

In produzione è consigliato l'uso di **PM2**:

```bash
pm2 start ecosystem.config.js
```

---

## 🌐 Apache2 – Reverse Proxy

Apache gestisce:

* traffico HTTP standard 
* proxy WebSocket verso Node.js

Esempio (semplificato):

```apache2
    ProxyPreserveHost On
    ProxyRequests Off

    # Proxy per le chiamate API dinamiche gestite da Node.js 
    ProxyPass "/api" "https://127.0.0.1:2601/api"
    ProxyPassReverse "/api" "https://127.0.0.1:2601/api"

    # Proxy per WebSocket (se il frontend si connette a /wss)
    ProxyPass "/ws" "wss://127.0.0.1:2601/ws"
    ProxyPassReverse "/ws" "wss://127.0.0.1:2601/ws"
```

Assicurarsi che i moduli siano abilitati:

```bash
a2enmod proxy proxy_http proxy_wstunnel
```

---

## Sicurezza

* Validazione input lato server
* Upload file controllati
* Configurazioni SMTP e credenziali esterne fuori dal repository
* Possibile uso di HTTPS tramite Let's Encrypt

---

## Sviluppo

* Ambiente locale con Apache
* Logging separato per Node.js
* Possibilità di debug WebSocket

---

## Licenza

Progetto proprietario – tutti i diritti riservati.

---

## sito

https://www.ilpiccolobardo.it

## Autore

**Sergio Baldaro**
Ingegnere / sviluppatore full‑stack

