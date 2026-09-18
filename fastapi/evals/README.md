# Valutazione narrativa ripetibile

Eseguire dalla directory `fastapi`, nell'ambiente con le dipendenze del servizio.

```bash
python -m evals.run
python -m evals.run --live --providers anthropic deepseek --repeats 2 --output /tmp/revisori.json
python -m evals.run --live --providers anthropic deepseek --cases sciarpa_maglia destinazione_coerente --refine-cases sciarpa_maglia destinazione_coerente --refiner deepseek --judge anthropic --output /tmp/rifinitura.json
```

Il primo comando valida il corpus senza chiamate API. Gli altri usano le credenziali dell'ambiente e consumano token. Non modificano il routing dell'app né i racconti salvati. Il fallback verso altri provider è disabilitato per non contaminare il confronto; il report registra i modelli effettivi, i prompt, le risposte, i token, i tempi e gli hash del codice e dei casi. Nessun file di risultato viene sovrascritto.

Gli otto casi sono coppie corretto/difettoso. Le etichette servono al calcolo delle metriche, non vengono inviate ai modelli. Precisione e richiamo misurano la presenza di una segnalazione; non dimostrano che il revisore abbia individuato il difetto atteso o che una correzione sia valida. Leggere il motivo atteso e quello restituito. Gli errori di formato o servizio sono distinti, con copertura esplicita: non considerarli silenziosamente successi. Due ripetizioni sono un primo controllo della variabilità, non una misura statistica robusta.

Il confronto `--refine-cases` mantiene fissa la bozza: un ramo usa il testo identico, l'altro chiama `rifinisci`. Entrambi vengono valutati dallo stesso provider, senza mostrare al revisore quale ramo sta leggendo. L'esperimento confronta i controlli sul testo ricevuto, non il testo dopo eventuali correzioni del revisore. Il diff e le tracce permettono di giudicare se il passaggio preserva voce, dettagli e azioni. La quota di testo cambiata è descrittiva: non è un voto di qualità. Le impostazioni di generazione sono quelle dell'app, quindi gli esiti non sono deterministici.

## Versioni nei report delle nuove storie

`versioni_racconto` conserva i testi di `genera_draft`, ogni `correggi_draft`, `rifinisci`, l'eventuale `limita_reduplicazioni`, `verifica_coerenza_domanda` e `verifica_testo_finale`. Include fase, modelli, hash, eventuale tentativo/bypass, numero di parole e diff dalla versione precedente. Le versioni identiche restano registrate per rendere visibile un passaggio che non cambia il testo. Le proposte finali scartate restano in `revisione_finale`; la versione finale contiene solo il testo effettivamente consegnato.

Le storie precedenti non possono recuperare retroattivamente le bozze mancanti. Conservare più versioni aumenta lo spazio occupato dai report, senza aggiungere chiamate LLM alla generazione.
