# Esempi positivi — set di valutazione qualitativa

Storie generate dal motore (`agent/graph.py` → `composer.py` → LLM) usate come
riferimento per verificare che, dopo modifiche future al motore o alla
Knowledge Base, la qualità dell'output non regredisca. Non sono test
automatici (l'output è per natura non deterministico) — sono esempi da
rileggere a occhio dopo un cambiamento importante, confrontando con la
checklist qui sotto.

Contesto: questi esempi nascono da una serie di bug reali trovati testando
manualmente il motore (vedi cronologia del progetto) — ogni esempio annota
quali difetti erano presenti nelle versioni precedenti e cosa verificare.

---

## Esempio 1 — "Il topolino e il leone nei rovi"

**Prompt utente:** `Raccontami la storia di un topolino piccolo e timido che aiuta un grosso leone a liberarsi dai rovi`
**Età bambino:** 3 anni
**Frammento primario recuperato:** `leone_topo_002` (Esopo, "Il leone e il topolino")

### Checklist di coerenza (motivo per cui questo esempio esiste)

- [x] **Il prompt vince sui dettagli concreti del frammento**: l'utente chiede "rovi", il frammento `leone_topo_002` descrive una "rete" — il racconto usa i rovi, non la rete.
- [x] **Nessun rovesciamento senza primo tempo**: il frammento primario contiene la battuta "Ridevi / Adesso non rido più", che presuppone una scena di derisione mai raccontata in questa versione — il racconto non la riprende, usa un dialogo di gratitudine coerente con quanto narrato.
- [x] **Ritornello integrato naturalmente**: "Cric, cric, cric!" ripetuto più volte nel punto giusto della scena (mentre il topolino rosicchia), non incollato a caso.
- [x] **Domanda finale coerente e non anticipata**: "chi pensi sia il più forte, il leone o il topolino?" resta una domanda aperta — nessun personaggio dichiara la morale prima.
- [x] **Verificato dal nodo `verifica_coerenza_domanda`**: passato senza bisogno di correzioni (`corretto_da_verifica_coerenza: false`), a conferma che la domanda era già coerente.
- [~] **Densità di immagini poetiche**: regola "massimo un'immagine per paragrafo" rispettata nella maggior parte del testo, ma il paragrafo di apertura di Baldo ne impila tre ("stelle come lanterne", "la luna gioca a nascondino", "sussurrata dal vento") — miglioria attesa ma non garantita al 100%, essendo un'istruzione data a un LLM e non un vincolo strutturale.

### Testo generato

> Ciao piccolo amico! Io sono Baldo, e dalla mia torre nel bosco vedo le stelle brillare come piccole lanterne nel cielo sereno. Sai, stasera la luna gioca a nascondino. Ma ora voglio raccontarti una storia magica sussurrata dal vento tra gli alberi...
>
> C'era una volta un topolino timido con grandi orecchie come foglie di quercia e un cuore coraggioso. Viveva nel fitto bosco, tra le radici degli alberi, e ogni notte guardava le stelle danzare sopra di lui.
>
> Un giorno, mentre passeggiava, sentì un ruggito forte come il tuono. Era un leone, grande e maestoso, intrappolato in un groviglio di rovi spinosi. Il leone si dibatteva e ruggiva, ma i rovi si stringevano sempre di più.
>
> Il topolino si avvicinò piano piano, tremando un po', ma deciso ad aiutare. Con i suoi piccoli dentini, iniziò a rosicchiare le spine, una alla volta. Cric, cric, cric!
>
> Il leone, inizialmente scettico, osservava stupito il coraggio del piccolo amico. Cric, cric, cric: il topolino continuava, senza fermarsi.
>
> Finalmente, con un ultimo cric, il leone fu libero! Guardò il topolino con gratitudine. "Grazie, piccolo amico. Non dimenticherò mai il tuo aiuto."
>
> E così, il topolino e il leone diventarono grandi amici. Spesso si incontravano sotto il cielo stellato per ammirare la luna che, a volte, sembrava una fetta di formaggio d'argento.
>
> E tu, caro bambino, chi pensi sia il più forte, il leone o il topolino?

---

## Come si è arrivati a questo esempio (per chi lo rilegge in futuro)

Le prime due generazioni con lo stesso identico prompt avevano difetti reali,
poi corretti nel motore:

1. **Frammento sbagliato scelto come primario** (bug di scoring: un prodotto
   cartesiano tra personaggi ed emozioni gonfiava il punteggio dei frammenti
   con più personaggi collegati) → ritornello e domanda finale scollegati
   dalla storia raccontata (parlavano del "topo di campagna", mai narrato).
2. **Frammento giusto, ma dettagli e battute copiati alla lettera dal
   frammento invece che dal prompt** → "rete" invece di "rovi", e un
   rovesciamento comico ("ridevi / non rido più") senza il primo tempo che lo
   giustificasse.

Se rileggendo una nuova generazione con questo stesso prompt ricompaiono
questi pattern, è un segnale di regressione nel motore (`composer.py` o
`neo4j_client.py`), non nella Knowledge Base.
