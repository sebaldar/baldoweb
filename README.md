# BALDOWEB

Repository di **Il Piccolo Bardo** e **Planetarium**.

Il Piccolo Bardo genera racconti per bambini, illustrazioni e audio; usa Neo4j
per frammenti e memoria narrativa. Planetarium visualizza il cielo in WebGL,
calcola le posizioni tramite un modulo C++ e offre una chat con ricerca Qdrant.

## Componenti

| Directory | Ruolo |
| --- | --- |
| `ilpiccolobardo/` | Frontend pubblico, pannelli `admin/` e importazione `loader/` |
| `planetarium/` | Frontend WebGL |
| `server/` | Node.js, HTTP/WebSocket, proxy verso Python e binding nativo |
| `cpp/` | Motore astronomico e librerie native |
| `fastapi/` | Pipeline narrativa LangGraph, meteo, astronomia, Neo4j |
| `fastapi-planetarium/` | Chat del planetario, embedding e ricerca Qdrant |
| `docker-compose.yml` | Servizi applicativi, MariaDB, Neo4j, MongoDB e Qdrant |

Il browser raggiunge Node attraverso Apache; Node chiama FastAPI sulla rete
Docker. I file statici sono serviti da Apache. La configurazione Apache e i dati
astronomici/database necessari al sito non sono distribuiti in questo repository.

## Configurazione

1. Copiare `.env.example` in `.env` e
   `fastapi-planetarium/.env.example` in `fastapi-planetarium/.env`.
2. Impostare credenziali database, chiavi dei provider e percorsi Docker.
   `WSS_PORT=3000`, `SESSION_DIR=/app/server/session`,
   `NEO4J_URI=bolt://neo4j:7687`, `DB_HOST=db`.
3. Configurare certificati TLS in `ssl/` e le variabili `SSL_DIR`, `SSL_CERT`,
   `SSL_KEY`, `SSL_CA`. Preparare gli asset in `data/planetarium_assets/`.
4. Impostare `ADMIN_TOKEN` nel `.env` principale con una chiave casuale di almeno
   32 caratteri; per generarla: `openssl rand -hex 32`.

Node e il servizio narrativo ricevono lo stesso `ADMIN_TOKEN` tramite Compose.
I pannelli amministrativi chiedono la chiave e la mantengono soltanto in memoria
nella pagina: non viene salvata in localStorage, sessionStorage o URL.
Le sessioni anonime non concedono privilegi amministrativi. Senza una chiave
configurata, le operazioni amministrative sono disabilitate. Dopo una rotazione
ricreare entrambi i servizi e ricaricare i pannelli. Questo accesso con chiave
condivisa non fornisce account individuali né un registro per singolo operatore.

Il file `server/config.json` è opzionale; `SERVER_CONFIG_FILE` consente di
indicare un percorso lato server (montare il file nel container se necessario).
Il file locale non viene incorporato nell’immagine. Il parametro HTTP `config_file` non determina
più il file letto. Non aggiungere segreti al codice o alle immagini Docker.

## Build e avvio

Richiesti Docker Compose e un reverse proxy configurato. La build Node usa
Node 24; il servizio narrativo usa Python 3.12, il planetario Python 3.11.

```bash
docker compose config --quiet
docker compose build
docker compose up -d
```

Le porte pubblicate dal Compose sono limitate a `127.0.0.1`. Apache deve
inoltrare `/api` verso HTTPS `127.0.0.1:3000` e `/ws` verso
WSS `127.0.0.1:3000`. La porta HTTP `3002` di Node è usata dalle chiamate
interne Docker ed è mappata sulla porta locale `3001`.

Per applicare modifiche a Node e alla pipeline narrativa in un ambiente già
configurato, ricostruire e ricreare insieme i due servizi:

```bash
docker compose up -d --build app fastapi
```

L'avvio dipende anche dai database, dai certificati e dagli asset locali.
`depends_on` da solo non garantisce che i database abbiano terminato l'avvio.

## Dipendenze e test

Node usa `package-lock.json` e `npm ci`. Ogni servizio Python installa
`requirements.txt` con i vincoli esatti di `requirements.lock.txt`, acquisiti
dalle versioni dei container esistenti. Per aggiornarli, verificare le nuove
versioni in un ambiente isolato e rigenerare i vincoli: non modificare i numeri
senza eseguire i test. I vincoli del planetario riflettono l'ambiente Linux,
comprese le dipendenze native di PyTorch.

```bash
npm ci --prefix server
npm test --prefix server
```

Per Python, in un ambiente virtuale Python 3.12:

```bash
pip install -r fastapi/requirements.txt -c fastapi/requirements.lock.txt
cd fastapi
NEO4J_PASSWORD=test-only OPENWEATHER_API_KEY=test-only python -B -m unittest discover -s tests -v
```

I test coprono autorizzazioni HTTP/WebSocket, inoltro delle credenziali interne,
rendering di contenuti ostili, chiusura delle connessioni, validazione degli input
e isolamento/rilascio dei checkpoint delle storie. Non chiamano provider LLM né
database reali. GitHub Actions esegue le suite Node e Python.

## Generazione e revisione dei racconti

La pipeline recupera il contesto e i frammenti narrativi, genera una bozza
(`genera_draft`) e la valuta (`valuta_draft`). Se necessario esegue al massimo
due correzioni (`correggi_draft`). Seguono la rifinitura opzionale, i controlli
su domanda e ritornello e la revisione finale (`verifica_testo_finale`).

### Rifinitura opzionale

Nella configurazione operativa adottata il 17 settembre 2026, `rifinisci` usa
il valore `nessuno`: il nodo conserva la bozza senza chiamare un modello.
È una scelta di routing persistita in `/app/config_data/model_routing.json`,
non il valore predefinito del codice. Una nuova installazione deve configurarla
esplicitamente nel pannello di routing dei modelli.

Nei test osservati la rifinitura restituiva spesso una bozza identica, aggiungendo
latenza senza correggere i difetti di contenuto. Il bypass permette di valutare
il generatore e conservare la sua voce; non disabilita le verifiche successive.
La rifinitura può essere riattivata se un confronto sulle stesse bozze dimostra
un beneficio. Il numero di similitudini è una metrica descrittiva: non attiva
più una riscrittura automatica dell'intero racconto.

### Controlli e correzioni locali

Il revisore finale verifica l'azione decisiva, la continuità narrativa e il
rispetto della richiesta. Per ciascun requisito individuato riporta una
citazione della richiesta, l'esito e, quando soddisfatto, un'evidenza testuale.
Un requisito segnalato come mancante impedisce l'esito `ok`; un controllo
obbligatorio assente o malformato produce `non_verificato` se non recuperabile.
Per azione decisiva, continuità e requisiti è consentito un solo tentativo complessivo di riparazione
delle evidenze per revisione, senza riscrivere il racconto. Differenze nei soli
spazi e a-capo sono ricondotte al passaggio originale senza chiamate LLM;
questa tolleranza non si applica alle sostituzioni. I controlli sono validati indipendentemente e quelli validi vengono conservati; `errori_evidenze` registra il problema
e `dettaglio_chiamate_llm` rende visibile la chiamata `.ripara_evidenze`.

Se il primo controllo rileva requisiti mancanti ma non produce modifiche,
un singolo passaggio `.correggi_requisiti` propone sostituzioni locali da
sottoporre alla conferma. I suggerimenti liberi non vengono applicati direttamente.

Sono ammesse al massimo cinque sostituzioni locali, con citazioni originali
univoche e senza sovrapposizioni. Una seconda chiamata verifica il candidato
prima di accettarlo. Se la conferma fallisce, viene conservato il testo originale
con l'esito diagnostico: il racconto può quindi essere restituito anche con
problemi irrisolti. `ok` è una valutazione del modello, non una garanzia di
correttezza; le citazioni sono validate dal codice, mentre completezza e
pertinenza del controllo richiedono anche prove e revisione umana.

### Meteo indicato nel prompt

Se il prompt impone condizioni atmosferiche, la pipeline usa quelle e salta la
chiamata al servizio meteo. L'analisi estrae una citazione della richiesta, con
un riconoscimento locale di condizioni esplicite comuni come «inverno rigido»
e «tormenta». Il report distingue `fonte_meteo: prompt`, `tool` e `non_disponibile`.
Un'ipotesi come «se piove» non impone il tempo; l'astronomia resta indipendente.

### Dati astronomici

Una semplice ambientazione invernale o notturna non attiva l’astronomia:
servono riferimenti celesti o una richiesta pertinente di osservazione.
Quando l'astronomia è attiva, generatore e revisore ricevono i dati del motore,
compresi altezza, azimut e direzione cardinale quando disponibili. L'orario
locale viene convertito per il motore; il report conserva `ora_utc_motore`.
L'altezza non determina il punto cardinale e la disponibilità geometrica di un
corpo non garantisce che sia osservabile attraverso nuvole o ostacoli.

Il racconto deve nominare il corpo scelto anche quando le nuvole ne impediscono
la vista, distinguendo osservazione e immaginazione. In assenza di dati reali,
il fallback non inventa corpi celesti. Il meteo disponibile non equivale
necessariamente a una previsione per l'ora futura richiesta.

### Report e prove ripetibili

I report delle nuove storie includono:

- `diagnosi_draft`: esiti delle valutazioni e correzioni della bozza;
- `revisione_finale`: proposte, esito e controlli iniziali/finali, inclusi
  `richiesta_iniziale` e `richiesta_finale`;
- `versioni_racconto`: testi per fase, hash, differenze e indicazione del bypass;
- `dettaglio_chiamate_llm`: modelli effettivi, token e tempi;
- `dati_astronomici`: dati restituiti dal motore, quando utilizzato.

Le versioni non vengono ricostruite retroattivamente per i racconti precedenti.
Per verificare il bypass, controllare `bypass: true` e `modelli: []` nella
versione `rifinisci`, oltre all'assenza della relativa chiamata LLM.

Dalla directory `fastapi`, nell'ambiente del servizio:

```bash
# Validazione del corpus senza chiamate ai provider
python -m evals.run

# Prove reali: richiedono credenziali e consumano token
python -m evals.run --live --providers anthropic deepseek --repeats 2 --output /tmp/revisori.json
python -m evals.check_request_live
python -m evals.check_blizzard_live
```

La prova `check_request_live` confronta un racconto che omette il nome del pianeta con uno che
lo nomina già, verificando correzione e conservazione del testo corretto.
Le prove live non modificano il routing dell'app. Per metodo, limiti e confronto
della rifinitura consultare [la guida alle valutazioni](fastapi/evals/README.md).

## Dati locali

`.env`, certificati, sessioni, ambienti virtuali, cache, log e media generati
restano fuori da Git. I `.dockerignore` dei singoli servizi escludono questi
file dalle immagini. Effettuare backup separati dei volumi `data/` e degli
allegati da conservare. I checkpoint dei racconti durano una singola richiesta;
la memoria conversazionale del planetario è ancora in RAM e richiede una
politica di conservazione dedicata per utilizzi prolungati.

## Licenza

Progetto proprietario — tutti i diritti riservati.
Autore: Sergio Baldaro.
