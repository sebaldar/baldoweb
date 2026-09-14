"""
services/composer.py
====================
Si occupa di assemblare il prompt finale per l'LLM, integrando:
1. Analisi del prompt (personaggi, emozioni)
2. Frammenti recuperati da Neo4j
3. Contesto Fisico (Meteo, Coordinate, Astronomia)
"""

import logging
import re

logger = logging.getLogger(__name__)

class StoryComposer:
    def __init__(self):
        # "sempre" qui aveva causato un problema reale: una storia esplicitamente
        # ambientata in casa ("Dentro casa...", nessun riferimento a cielo/meteo)
        # ha comunque aperto con un intero paragrafo di sole e meteo — la Regola 2
        # più sotto (condizionale: apre con il cielo solo se pertinente) è arrivata
        # troppo tardi per contrastare un'affermazione assoluta piazzata qui,
        # all'inizio del prompt. Riformulato da "intrecci sempre" (obbligo) a "sai
        # intrecciare" (un talento che ha) + rimando esplicito alla Regola 2 per
        # decidere quando usarlo.
        self.base_instruction = (
            "Sei Baldo, un anziano astrologo e contastorie gentile che vive in una torre antica. "
            "Usi un linguaggio magico, rassicurante e adatto a bambini. "
            "La tua caratteristica unica è che SAI intrecciare la realtà fisica (meteo e stelle) "
            "con la fantasia dei frammenti di storia — è un tuo talento, non un obbligo da "
            "sfoderare in ogni racconto: se e quanto usarlo lo decide la Regola 2 più sotto, "
            "non questa frase."
        )

    @staticmethod
    def _descrizione_illuminazione_luna(percento):
        """
        La sola fase ("Crescente") non dice quanto sia vistosa la luna: a
        15° di elongazione (già fuori dalla soglia di "nuova invisibile")
        l'illuminazione è ~1,7%, un filo sottilissimo — a 70° (ancora
        "Crescente") è già il 41%, una mezzaluna ben visibile. Senza questa
        distinzione il prompt le descrive allo stesso modo, specialmente
        rilevante per la luna diurna: una falce quasi invisibile non
        merita lo stesso "che regalo!" di una mezzaluna vistosa.
        """
        if percento is None:
            return None
        if percento < 5:
            return "un filo di luce sottilissimo, quasi invisibile"
        if percento < 25:
            return "una sottile falce"
        if percento < 45:
            return "una falce ben visibile"
        if percento < 65:
            return "una mezzaluna"
        if percento < 90:
            return "quasi piena, molto luminosa"
        return "piena e luminosa"

    def componi(self, prompt_originale: str, analisi: dict, frammenti: list, **kwargs) -> tuple[str, dict]:
        """
        Assembla il prompt finale per la generazione del draft.
        """
        
        # --- ESTRAZIONE SICURA VARIABILI (Risolve NameError e NoneType) ---
        luogo = analisi.get("luogo") or kwargs.get("luogo") or "un luogo segreto"
        
        # Gestione meteo con fallback sicuro per .lower()
        meteo_raw = analisi.get("dati_meteo") or kwargs.get("dati_meteo") or "sereno"
        meteo = str(meteo_raw).lower()
        
        data = analisi.get("data_storia") or kwargs.get("data_storia") or "oggi"
        ora = analisi.get("ora_storia") or kwargs.get("ora_storia") or "adesso"
        
        # Dati Astronomici
        astro = analisi.get("dati_astronomici") or kwargs.get("dati_astronomici") or {}
        corpi_celesti = astro.get("corpi", [])
        fase_luna = astro.get("fase_luna", "sconosciuta")

        # Quanto è vistosa la luna adesso, non solo in che fase è (vedi
        # _descrizione_illuminazione_luna per il perché conta).
        luna_info = next((c for c in corpi_celesti if c.get("nome") == "Luna"), None)
        illuminazione_luna = luna_info.get("illuminazione_percento") if luna_info else None
        descrizione_illuminazione = self._descrizione_illuminazione_luna(illuminazione_luna)

        # --- FRAMMENTO PRIMARIO vs SECONDARI ---
        # Solo il frammento con punteggio più alto (il primo: cerca_frammenti
        # restituisce già in ordine di rilevanza) fornisce domanda/ritornello/
        # tecnica narrativa. Prendere questi campi dal primo disponibile in
        # tutta la lista (comportamento precedente) poteva far ereditare una
        # domanda pensata per un frammento marginale, scollegata dalla trama
        # che l'LLM finisce davvero per raccontare.
        frammenti = frammenti or []
        frammento_primario = frammenti[0] if frammenti else {}
        # Al massimo 2 secondari (3 testi completi in tutto): con tutti e 5 i
        # frammenti trovati nel contesto, il draft si è visto arrivare 4277
        # token in ingresso e il rischio si capovolge — nelle storie migliori
        # è il frammento a pesare poco sulla trama, non il contrario.
        frammenti_secondari = frammenti[1:3]

        # --- COSTRUZIONE SEZIONE FRAMMENTI ---
        personaggi_richiesti = analisi.get('personaggi', [])
        riga_personaggi_obbligatori = (
            f"4. I personaggi nominati dall'utente ({', '.join(personaggi_richiesti)}) sono "
            "OBBLIGATORI E ATTIVI: ognuno deve avere almeno un'azione con una sua "
            "intenzione e almeno una battuta di dialogo. Non ridurre un "
            "personaggio richiesto (es. un antagonista) a un semplice rumore o "
            "spavento di passaggio (\"Miao!\" e sparisce) — deve avere un obiettivo "
            "e delle parole sue.\n"
            if personaggi_richiesti else ""
        )
        if frammento_primario:
            archetipo = frammento_primario.get('archetipo')
            riga_archetipo = (
                f"ARCHETIPO NARRATIVO: {archetipo} (termine tecnico per te, l'autore — "
                "guida la struttura della trama, non deve mai comparire come parola "
                "dentro il racconto: un bambino non sa cosa significhi).\n\n"
                if archetipo else ""
            )
            testo_frammenti = (
                f"{riga_archetipo}"
                f"ESEMPIO DI STILE (NON di trama — leggi bene la regola 1 sotto):\n"
                f"- {frammento_primario.get('testo')}\n\n"
                "Come usarlo — regole non negoziabili:\n"
                "1. Il testo sopra NON è la trama da seguire o parafrasare: è solo "
                "un esempio di stile, ritmo e registro linguistico adatto "
                "all'età. Inventa una trama NUOVA — eventi, dettagli, svolte — "
                "basata sul PROMPT DELL'UTENTE e sull'archetipo narrativo "
                "indicato sopra, non sulla sequenza di eventi del testo "
                "d'esempio. Le storie migliori nascono quando il testo "
                "d'esempio influenza pochissimo gli eventi concreti — più ti "
                "allontani dalla sua trama letterale, meglio è, purché resti "
                "coerente con l'archetipo e con il prompt.\n"
                "2. Il PROMPT DELL'UTENTE vince sempre sui dettagli concreti "
                "(dove, cosa, chi, oggetti) — se qualcosa del testo d'esempio "
                "sopravvive per caso nella tua trama nuova e confligge con il "
                "prompt, usa quello che ha chiesto l'utente.\n"
                "3. Non riprendere battute o richiami del testo d'esempio che "
                "presuppongono una scena non raccontata (es. un personaggio che "
                "\"non ride più\" implica che prima ridesse: se quella risata non "
                "fa parte della tua storia, non scrivere il richiamo, oppure "
                "costruisci prima il momento che lo giustifica).\n"
                f"{riga_personaggi_obbligatori}"
                "5. Non introdurre personaggi antagonisti aggiuntivi che non sono "
                "nel prompt dell'utente, se il prompt ne specifica già uno.\n"
                "6. La tua trama nuova deve avere una motivazione chiara per cui "
                "il protagonista agisce — mai un'azione immotivata (es. mai \"un "
                "giorno decise di...\" senza dire perché, specie se il "
                "personaggio è timido: un timido ha bisogno di un motivo forte "
                "per agire).\n"
                "7. Se il testo d'esempio sopra è una favola con una morale "
                "esplicita tradizionale (es. una favola di Esopo, una storiella "
                "con un insegnamento dichiarato alla fine), prendine SOLO ritmo, "
                "lessico, struttura e tecnica narrativa — la sua eventuale morale "
                "esplicita non va mai trasferita nel tuo racconto: vale comunque "
                "la Regola 7 più sotto, che vieta di dichiarare la morale.\n"
            )
            if frammenti_secondari:
                testi_secondari = "\n".join(f"- {f.get('testo')}" for f in frammenti_secondari)
                testo_frammenti += (
                    "\nALTRI FRAMMENTI DISPONIBILI (spunti accessori: usali SOLO se si "
                    "integrano naturalmente nella trama del frammento principale, "
                    "altrimenti ignorali — meglio una storia coerente che una che li "
                    "cita tutti a forza):\n"
                    f"{testi_secondari}\n"
                )
        else:
            testo_frammenti = "Usa la tua fantasia, ma mantieni lo stile di Baldo."

        # --- COSTRUZIONE SEZIONE PROFILI PERSONAGGI ---
        # Solo i personaggi con descrizione/tratti compilati a mano nel
        # pannello Personaggi (identificati per nome, come ovunque nel grafo).
        personaggi_bio = kwargs.get("personaggi_bio") or {}
        righe_bio = []
        for nome, bio in personaggi_bio.items():
            dettagli = []
            if bio.get("description"):
                dettagli.append(bio["description"])
            if bio.get("traits"):
                dettagli.append("tratti: " + ", ".join(bio["traits"]))
            if dettagli:
                righe_bio.append(f"- {nome}: {'; '.join(dettagli)}")
        sezione_bio = ""
        if righe_bio:
            sezione_bio = (
                "\nPROFILI PERSONAGGI (mantienili coerenti con le storie precedenti):\n"
                + "\n".join(righe_bio) + "\n"
            )

        # --- COSTRUZIONE SEZIONE TECNICA NARRATIVA ---
        # Solo dal frammento primario (vedi nota sopra): il nome della tecnica
        # non spiega nulla di per sé, Baldo la deduce dal testo di quel
        # frammento specifico — prenderla da un frammento diverso da quello
        # che dà anche testo/domanda/ritornello creerebbe la stessa
        # incoerenza che vogliamo evitare.
        tecnica_narrativa = frammento_primario.get("tecnica_narrativa")
        sezione_tecnica = ""
        if tecnica_narrativa:
            sezione_tecnica = (
                f"\nTECNICA NARRATIVA DA APPLICARE: {tecnica_narrativa}. "
                "Osserva come il frammento principale la mette in pratica e usa lo "
                "stesso dispositivo narrativo per costruire la tua storia. Anche "
                "questo è un termine tecnico per te: non usarlo mai come parola "
                "dentro il racconto (mai scrivere frasi come \"ed ecco il "
                f"{tecnica_narrativa}\" — applica la tecnica, non nominarla).\n"
            )

        # --- COSTRUZIONE SEZIONE DOMANDA FINALE ---
        domanda_finale = frammento_primario.get("domanda")
        sezione_domanda = ""
        if domanda_finale:
            sezione_domanda = (
                f"\nDOMANDA FINALE: Concludi il racconto rivolgendo al bambino "
                f"questa domanda (adattala al contesto se serve): \"{domanda_finale}\"\n"
            )

        # --- COSTRUZIONE SEZIONE RITORNELLO ---
        # Sempre presente, anche quando il frammento non ne fornisce uno: se
        # dipende dal caso (solo quando il frammento vincitore ne ha uno), il
        # ritornello compare in modo incostante da una storia all'altra. Va
        # dichiarato come requisito PRIMA della scrittura, non lasciato
        # emergere se capita.
        ritornello = frammento_primario.get("ritornello")
        if ritornello:
            sezione_ritornello = (
                f"\nRITORNELLO: Il frammento suggerisce questo ritornello: \"{ritornello}\" — "
                "usalo come PATTERN RITMICO (suoni, struttura, lunghezza), non come "
                "stringa fissa da ripetere alla lettera. Se contiene un nome proprio "
                "specifico della fiaba di provenienza del frammento (es. un "
                "personaggio che la TUA storia non ha motivo di includere), "
                "SOSTITUISCILO SEMPRE con il personaggio o l'elemento equivalente "
                "della tua trama (es. \"il drago\", non il nome originale) — il "
                "bambino non deve mai sentire un nome che la tua versione non ha "
                "mai presentato. Una volta deciso l'adattamento, ripetilo 2-3 volte "
                "durante il racconto, nei momenti chiave, sempre IDENTICO a se "
                "stesso (non ri-adattarlo una seconda volta a metà racconto). "
                "NON annunciare mai, in nessuna forma, che sta per arrivare un "
                "ritornello — né alla lettera (\"il ritornello di oggi è...\") né "
                "con un giro di parole più elegante (es. \"fu proprio in quel "
                "momento che il ritornello le sbocciò sulle labbra\" — osservato "
                "in un caso reale: è comunque un annuncio, solo travestito). E "
                "soprattutto: se dici che un personaggio \"lo canta due volte\" o "
                "simili, quelle due volte devono comparire DAVVERO scritte nel "
                "testo, non solo essere raccontate — un bambino deve sentirlo "
                "ripetuto per davvero, non sapere che è stato ripetuto. "
                "ECCEZIONE deliberata alle regole di varietà lessicale che trovi "
                "altrove in questo prompt (sul Contesto Reale, sull'apertura di "
                "Baldo): quelle chiedono di non ripetere le STESSE frasi da una "
                "storia all'altra — qui vale l'esatto opposto, e di proposito: "
                "ripeti la stessa frase più volte DENTRO questa storia, non è un "
                "errore da evitare, è il punto del ritornello.\n"
                "Se il ritornello nomina più figure o elementi distinti (es. \"una "
                "strega, un uomo, un gigante, un giudice\"), la tua storia deve "
                "costruire PRIMA una scena concreta e visibile per OGNUNO di essi, "
                "così il bambino riconosce a chi si riferisce ciascuno quando la "
                "frase torna. Se per uno di questi elementi non riesci a inventare "
                "una scena credibile nella tua trama nuova, è meglio adattare la "
                "frase togliendolo (restando fedele allo spirito del ritornello) "
                "piuttosto che nominarlo comunque senza che sia mai comparso.\n"
            )
        else:
            sezione_ritornello = (
                "\nRITORNELLO: il frammento non ne fornisce uno — INVENTANE TU uno, "
                "breve e orecchiabile (un'onomatopea o una frasetta di poche "
                "parole), coerente con l'archetipo e la tecnica narrativa sopra. "
                "Decidilo PRIMA di scrivere la storia, non a metà: poi ripetilo "
                "2-3 volte, identico, nei momenti chiave — deve essere una frase "
                "che il bambino può dire insieme a te già dalla seconda volta. "
                "\"Decidilo prima\" è un'istruzione per te, non per il bambino: "
                "NON annunciare mai, in nessuna forma, che sta per arrivare un "
                "ritornello — né alla lettera (\"il ritornello di oggi è...\") né "
                "con un giro di parole più elegante (es. \"fu proprio in quel "
                "momento che il ritornello le sbocciò sulle labbra\" — osservato "
                "in un caso reale: è comunque un annuncio, solo travestito) — "
                "intreccialo nella narrazione come se ci fosse sempre stato. E "
                "soprattutto: se dici che un personaggio \"lo canta due volte\" o "
                "simili, quelle due volte devono comparire DAVVERO scritte nel "
                "testo, non solo essere raccontate — un bambino deve sentirlo "
                "ripetuto per davvero, non sapere che è stato ripetuto. "
                "ECCEZIONE deliberata alle regole di varietà lessicale che trovi "
                "altrove in questo prompt (sul Contesto Reale, sull'apertura di "
                "Baldo): quelle chiedono di non ripetere le STESSE frasi da una "
                "storia all'altra — qui vale l'esatto opposto, e di proposito: "
                "ripeti la stessa frase più volte DENTRO questa storia, non è un "
                "errore da evitare, è il punto del ritornello.\n"
            )

        # --- COSTRUZIONE SEZIONE CIELO (coerente con l'ora reale) ---
        # Senza questo controllo il modello, lasciato solo con "adesso" come
        # ora, tende a descrivere sempre un cielo notturno stellato — è il
        # framing stesso di Baldo ("scruta le stelle dalla torre") a
        # spingerlo in quella direzione, anche quando la storia è ambientata
        # di giorno. Le stelle non sono mai visibili in pieno giorno.
        ora_match = re.match(r"^(\d{1,2})", str(ora))
        ora_num = int(ora_match.group(1)) if ora_match else None
        è_notte = ora_num is not None and (ora_num >= 20 or ora_num < 6)

        suffisso_illuminazione = f" ({descrizione_illuminazione})" if descrizione_illuminazione else ""

        if ora_num is None:
            descrizione_cielo = f"La luna è in fase {fase_luna}{suffisso_illuminazione}."
            if corpi_celesti:
                nomi_corpi = [c.get("nome") for c in corpi_celesti]
                descrizione_cielo += f" In cielo sono visibili: {', '.join(nomi_corpi)}."
        elif è_notte:
            descrizione_cielo = f"È notte: il cielo è scuro, le stelle sono visibili. La luna è in fase {fase_luna}{suffisso_illuminazione}."
            if corpi_celesti:
                nomi_corpi = [c.get("nome") for c in corpi_celesti]
                descrizione_cielo += f" Sono visibili anche: {', '.join(nomi_corpi)}."
        else:
            descrizione_cielo = (
                "È giorno: il cielo è azzurro e luminoso. Le stelle NON sono "
                "visibili adesso (si vedono solo di notte) — se vuoi parlare "
                "del cielo, descrivi il sole, le nuvole o gli uccelli, non le "
                "stelle. Vale per TUTTA la storia, dall'inizio fino alla "
                "domanda finale compresa: non farle ricomparire in chiusura."
            )
            # Solo la Luna, non Venere: la Luna ha un ruolo fiabesco/culturale
            # per un bambino (fasi, storie, "buonanotte luna"), Venere è solo
            # "un puntino luminoso" senza identità propria — osservato che
            # menzionarle insieme ("si vede anche Luna e Venere... falla
            # scoprire come un regalo") spinge il narratore a introdurre
            # Venere anche quando il prompt chiedeva solo della luna, senza
            # che serva mai alla trama. Venere resta comunque nei dati grezzi
            # (dati_astronomici, per il report), solo non viene spinta qui.
            luna_diurna = any(c.get("nome") == "Luna" for c in corpi_celesti)
            if luna_diurna:
                if descrizione_illuminazione:
                    descrizione_aspetto = (
                        f"la Luna è {descrizione_illuminazione} nel blu del "
                        "cielo, non luminosa come di notte"
                    )
                else:
                    descrizione_aspetto = (
                        "appare pallida e sbiadita nel blu del cielo, non "
                        "luminosa come di notte"
                    )
                # Una falce al minimo di illuminazione è un dettaglio sottile,
                # non uno spettacolare: senza questa distinzione il prompt
                # tratterebbe un filo quasi invisibile come una mezzaluna
                # vistosa, con lo stesso "che regalo!".
                nota_vividezza = (
                    " Essendo quasi al minimo di illuminazione, è un dettaglio "
                    "sottile che un personaggio nota solo guardando con "
                    "attenzione — non descriverla come un evento eclatante."
                    if illuminazione_luna is not None and illuminazione_luna < 5
                    else ""
                )
                descrizione_cielo += (
                    f" DETTAGLIO REALE DA NON PERDERE: oggi si vede anche la "
                    f"Luna nonostante sia giorno — capita davvero, non è un "
                    f"errore: {descrizione_aspetto}.{nota_vividezza} Se il "
                    "prompt dell'utente chiede di guardare il cielo o cercare "
                    "la luna, questo è il posto giusto per usarlo: falla "
                    "scoprire al personaggio come un piccolo regalo inatteso, "
                    "non aggiungerla come dettaglio a caso se non c'entra con "
                    "la trama. Non introdurre altri pianeti o stelle solo "
                    "perché presenti nei dati — la Luna basta."
                )
            else:
                # Assenza esplicita, non silenzio: senza questa frase il
                # modello, davanti a un prompt che chiede di "guardare il
                # cielo e cercare la luna", risolve la tensione a favore del
                # prompt e ne inventa comunque una visibile (osservato in
                # una storia reale: "vide luccicare... era lei, la luna",
                # con fase_luna "non visibile" nei dati). Il vincolo riguarda
                # solo il cielo reale: se il prompt insiste, la luna può
                # comparire in altro modo, non come bugia sul cielo.
                #
                # Prima versione di questa regola offriva 3 alternative alla
                # pari (riflesso/ricordo/promessa per stasera): il modello ha
                # scelto quella più comoda da scrivere (promessa per dopo),
                # che risolve il conflitto astronomico ma abbandona
                # un'azione che il prompt chiedeva esplicitamente (in un
                # caso reale: "seguila e attraversa il lago" — con la
                # "promessa per stasera" quell'attraversamento non avviene
                # più). La priorità va quindi decisa in base a COSA chiede
                # il prompt, non fissata una volta per tutte sulla luna: se
                # l'utente vuole un'azione con l'oggetto assente, la
                # soluzione deve permettere che quell'azione avvenga
                # comunque, non solo evocarlo.
                descrizione_cielo += (
                    " La Luna oggi NON è visibile in cielo (è sotto "
                    "l'orizzonte o troppo vicina al sole): un personaggio "
                    "che guarda in alto non la trova, e la storia non deve "
                    "far finta che ci sia. Se il prompt dell'utente chiede "
                    "comunque di cercarla, non ignorare la richiesta e non "
                    "falsificare il cielo: trasformala narrativamente. Se il "
                    "prompt chiede solo di vederla o menzionarla, un ricordo "
                    "o la promessa che tornerà stasera bastano. Ma se il "
                    "prompt chiede un'AZIONE che coinvolge la luna — "
                    "seguirla, raggiungerla, orientarsi con lei, parlarle — "
                    "scegli una soluzione che permetta quell'azione di "
                    "avvenire comunque (es. una traccia luminosa, un "
                    "riflesso che si muove e si può seguire davvero): "
                    "rimandare l'incontro a un altro momento la farebbe "
                    "sparire dalla trama, e quell'azione richiesta non "
                    "accadrebbe più."
                )

        # --- NUMERO DI INGANNI/SVOLTE SCALATO SULL'ETÀ ---
        # Osservato: una storia con 3 inganni in sequenza, ognuno con il suo
        # apparato di oggetti e scena, funziona a 6 anni ma è troppo densa
        # (difficile da visualizzare) già a 4, e ancora di più a 3.
        eta_bambino = kwargs.get('eta_bambino', 4)
        try:
            eta_bambino = int(eta_bambino)
        except (TypeError, ValueError):
            eta_bambino = 4
        if eta_bambino <= 3:
            regola_numero_inganni = (
                f"Se la trama prevede inganni, travestimenti o sotterfugi in sequenza: "
                f"per un bambino di {eta_bambino} anni, massimo 1, semplice e diretto — "
                "niente sequenze di trucchi diversi da tenere a mente."
            )
        elif eta_bambino <= 5:
            regola_numero_inganni = (
                f"Se la trama prevede inganni, travestimenti o sotterfugi in sequenza: "
                f"per un bambino di {eta_bambino} anni, massimo 2 — ognuno in più allunga "
                "la storia e aggiunge un elemento nuovo da visualizzare."
            )
        else:
            regola_numero_inganni = (
                f"Se la trama prevede inganni, travestimenti o sotterfugi in sequenza: "
                f"per un bambino di {eta_bambino} anni puoi arrivare fino a 3, se la trama "
                "lo richiede davvero — ma meno è comunque meglio."
            )

        # --- TEMPLATE FINALE ---
        prompt_finale = f"""
{self.base_instruction}

ATTENZIONE PRIMA DI USARE QUESTO BLOCCO: l'uso di Cielo/Meteo nell'apertura del racconto è CONDIZIONALE, non automatico (dettagli alla Regola 2 più sotto) — se la storia è ambientata al chiuso, o il prompt non fa riferimento a cielo/meteo/un momento preciso della giornata, NON aprire descrivendo il cielo: vai dritto alla storia, anche se i dati qui sotto sono compilati (lo sono sempre, a prescindere da dove è ambientata la storia — la loro presenza qui non è un segnale che vadano usati). Osservato più volte: una storia ambientata "dentro casa"/"in soffitta", senza nessun accenno al cielo nel prompt, ha comunque aperto con un intero paragrafo di sole e meteo — è esattamente l'errore da evitare.

CONTESTO REALE (Usa questi dettagli per l'ambientazione — sono ispirazione per TE, l'autore: non copiare mai le frasi qui sotto alla lettera nel racconto, riformulale sempre con parole tue e nella voce di Baldo). In particolare, il campo Meteo è costruito con un piccolo numero di espressioni fisse che tendono a ricomparire identiche da una storia all'altra — non riprodurre MAI, nemmeno in parte, formule come "di quelli in cui si esce senza giacca", "di quelli da sciarpa e guanti", "giusta per una giacca leggera", "perfetta per giocare fuori", "di quelli da acqua fresca e ombra": sono etichette per te, non battute da mettere in bocca a Baldo. Caso osservato di elusione: una storia ha aggirato il divieto riformulando la STESSA idea con altre parole ("lasciare a casa la giacca" invece di "uscire senza giacca") — cambiare le parole non basta se il concetto resta lo stesso. Quindi, in aggiunta al divieto sulle frasi esatte: non descrivere MAI la temperatura tramite abiti da indossare o non indossare (giacca, sciarpa, guanti, maglione, felpa...), in nessuna forma. Usa solo l'informazione (che sensazione di temperatura c'è) per descriverla con parole nuove ogni volta, parlando della sensazione fisica direttamente — sulla pelle, nell'aria, nel respiro — non di cosa si indossa:
- Luogo: {luogo}
- Data e Ora: {data} alle {ora}
- Meteo attuale: {meteo}
- Cielo: {descrizione_cielo}

ELEMENTI DELLA STORIA RICHIESTI:
- Prompt Utente: "{prompt_originale}"
- Personaggi identificati: {', '.join(analisi.get('personaggi', []))}
- Emozioni da evocare: {', '.join(analisi.get('emozioni', []))}
{sezione_bio}
FRAMMENTI DI TRAMA DAL DATABASE:
{testo_frammenti}
{sezione_tecnica}{sezione_ritornello}{sezione_domanda}
REGOLE DI GENERAZIONE:
1. Rivolgiti al bambino con dolcezza.
2. L'apertura di Baldo resta breve in ogni caso — una o due frasi, mai un intero paragrafo di cielo prima che la storia cominci davvero. Se la storia è ambientata all'aperto, o il prompt fa in qualche modo riferimento al cielo, al meteo o a un momento preciso della giornata, la frase d'apertura può accennare a cosa Baldo vede davvero dalla sua torre a {luogo} in questo momento (leggi la sezione Cielo sopra: se è giorno, niente stelle) — UN dettaglio scelto, non un elenco di tutto quello che c'è in cielo. Se invece la storia si svolge interamente al chiuso o in un contesto dove il cielo non c'entra nulla (es. un oggetto animato in una stanza, un salone, una cameretta), un saluto naturale basta, con al massimo un accenno rapido al meteo se viene spontaneo. In entrambi i casi varia i dettagli concreti che scegli da una storia all'altra. In particolare, queste frasi sono già ricomparse troppe volte in storie precedenti — NON usarle mai, in nessuna forma o variante: "non c'è nemmeno una nuvola" (e varianti tipo "nemmeno una nuvoletta", "nemmeno una nuvola disturba il cielo"), "una caramella di zucchero". Se il cielo è sereno, inventa ogni volta un modo diverso di dirlo. In ogni caso, quello che il prompt dell'utente chiede esplicitamente viene sempre prima della cornice di Baldo, mai il contrario.
3. La lunghezza deve essere {kwargs.get('lunghezza', 'media')}.
4. Rispondi esclusivamente in lingua: {kwargs.get('lingua', 'Italiano')}.
5. Età del bambino: {kwargs.get('eta_bambino', 4)} anni (usa un vocabolario appropriato).
6. Massimo un'immagine poetica per paragrafo (una metafora, un paragone lirico): il resto della frase resta concreto. Non impilare più immagini liriche nella stessa frase o nel giro di poche righe.
7. Se c'è una DOMANDA FINALE, non far dichiarare la morale della storia — né a un personaggio né a te come narratore — prima di arrivarci. Vale ANCHE per la frase immediatamente precedente la domanda: è il punto più a rischio (osservato in un caso reale: un personaggio diceva "Abbiamo vinto qualcosa di più bello di una gara" un attimo prima della domanda finale, svuotandola del tutto). Evita in particolare aperture come "Abbiamo vinto qualcosa di più bello di...", "La cosa più importante era...", "Quello che conta davvero è...", "Avevano scoperto che..." — sono quasi sempre un annuncio di morale, in qualunque contesto compaiano. Costruzioni più generiche come "aveva capito che..." o "in fondo..." non sono vietate in sé (possono descrivere un fatto qualunque), ma diventano lo stesso problema se usate per chiudere la storia con un insegnamento esplicito: il segnale d'allarme è la POSIZIONE (proprio prima della domanda finale), non solo le parole. Il significato deve emergere da azioni, scelte e conseguenze dei personaggi, mai da una frase che lo riassume. La domanda deve restare una domanda vera, che il bambino può ancora pensare da solo, non la conferma di qualcosa già detto esplicitamente. Se una frase prima della domanda è già una chiusura emotiva soddisfacente, fermati lì: non serve aggiungere altro.
8. Ogni dettaglio sensoriale deve essere percepibile davvero da un bambino che ascolta: niente immagini che funzionano solo per un adulto che coglie il sottotesto (es. un personaggio che "arrossisce sotto il pelo nero" — un bambino non può vederlo). Se un'immagine ha senso solo a livello concettuale e non letterale, cambiala con qualcosa di concreto (un suono, un movimento, un'espressione visibile).
9. Se la storia costruisce più inganni, travestimenti o apparizioni pensati per essere poi elencati o nominati insieme (in un ritornello, in un riepilogo recitato da un personaggio, in una lista finale), OGNUNO di questi elementi deve avere prima una scena concreta e visibile che lo giustifichi — un bambino deve poter VEDERE la cosa nominata, non decodificarla come simbolo. Esempio di errore da evitare: l'elenco finale nomina "un giudice" ma nella storia è comparso solo un sasso lanciato contro un tronco — un bambino non collega le due cose, solo un adulto può leggerci "il martello di un giudice". Se vuoi quell'effetto, rendilo esplicito nella scena stessa (es. un tronco spezzato a forma di martelletto, un colpo netto "come quello di un giudice che grida: basta!").
10. Gli oggetti usati per un inganno o un travestimento devono essere cose che il personaggio ha già con sé o trova naturalmente sul posto (un mantello, un ramo, una borsa, un sasso) — evita di far comparire dal nulla un oggetto creato apposta per il trucco e mai menzionato prima (es. una gabbietta con lucciole tenuta pronta per l'occasione): se serve un oggetto specifico, mostralo prima o rendilo qualcosa che il personaggio troverebbe davvero lì.
11. {regola_numero_inganni}
12. Se attribuisci un genere grammaticale a un personaggio/creatura tramite l'articolo (es. "il T-Rex", "la strega"), mantieni lo stesso genere nei pronomi per tutta la storia — non alternare "lui" e "lei" per lo stesso personaggio.
13. Se un personaggio pone una condizione esplicita in un dialogo (es. "non uscirò finché non mi porti X", "ti aiuterò solo se..."), la trama deve poi affrontarla chiaramente: risolta com'è stata posta, sostituita da un'alternativa che il personaggio accetta esplicitamente, o lasciata cadere con un motivo raccontato — mai abbandonata in silenzio, con la storia che prosegue come se non fosse mai stata detta.
14. La domanda finale deve restare dentro l'esperienza concreta del protagonista, mai diventare una riflessione astratta su percezione, identità o cambiamento (evita domande come "è cambiato lui davvero, o è cambiato il modo in cui lo guardavi?" — un ragionamento di secondo livello troppo concettuale per {eta_bambino} anni). Preferisci una domanda che il bambino risponde pensando a cosa avrebbe fatto lui, o a cosa succede dopo, restando nei panni del protagonista (es. "Tu avresti avuto il coraggio di parlargli?", "Secondo te, potrebbero diventare amici?").
15. Se un antagonista o un ostacolo minaccioso si ammorbidisce, non farlo cedere dopo un solo scambio di battute (una richiesta gentile e subito "va bene, passa pure" è troppo rapido, indebolisce sia l'antagonista che il coraggio del protagonista): costruisci prima un piccolo momento di esitazione o resistenza — il protagonista ha paura, pensa al motivo per cui è lì, fa comunque un passo avanti — e solo dopo l'antagonista si ferma e ascolta davvero.
16. La TECNICA NARRATIVA indicata sopra deve arricchire la sequenza di eventi che il prompt richiede esplicitamente, non prenderne il posto. Se il prompt descrive un'azione o una sequenza precisa (es. "osserva X, seguilo, attraversa Y"), quella resta il nucleo della trama dall'inizio alla fine; la tecnica va applicata DENTRO quella sequenza (nel modo in cui viene raccontata, in una svolta, in un dettaglio), non usata per introdurre una deviazione che finisce per diventare il centro della storia al posto dell'azione richiesta.
17. La reduplicazione ritmica di una parola (es. "piccola piccola", "forte forte", "vicino vicino") è una tecnica legittima — non evitarla del tutto — ma sta diventando un tic se ricorre più di una volta nello stesso racconto: usala AL MASSIMO una volta in tutta la storia, nel punto che la merita di più. Per esprimere intensità o vicinanza altrove, trova un modo diverso ogni volta (un paragone, un dettaglio concreto, il ritmo della frase stessa) invece di ricadere sempre sullo stesso trucco.
18. Il protagonista (o chi aiuta gli altri nella storia) non deve risultare perfettamente paziente, saggio e infallibile con tutti i personaggi che incontra. Se aiuta più personaggi in sequenza, evita lo schema meccanico "personaggio ha un problema → il protagonista offre subito la soluzione giusta", ripetuto identico per ciascuno — un bambino non se ne accorge consapevolmente, ma il risultato suona costruito, non raccontato. Varia invece la reazione da un personaggio all'altro: un'esitazione, una battuta, un piccolo errore che poi corregge, un momento di stanchezza o di insofferenza vera (anche solo un attimo) prima di aiutare comunque. Non deve diventare un personaggio-guida infallibile: un'imperfezione piccola e umana lo rende più vero.

GENERA IL RACCONTO:
"""
        # ritornello_atteso NON viene più preso qui: il testo grezzo del
        # frammento può contenere un nome proprio specifico della fiaba
        # d'origine (es. "Mangiafuoco") che il draft, lasciato libero,
        # sostituisce già con i personaggi della sua trama — imporlo
        # verbatim a rifinisci reintroduceva quel nome estraneo. Il
        # ritornello da proteggere viene rilevato DOPO il draft, da quello
        # che il modello ha davvero scritto (vedi agent/nodes.py:genera_draft).
        # tecnica_narrativa/archetipo tornano invece qui, per il controllo
        # (economico, senza LLM) che il loro valore letterale — jargon per
        # l'autore, non per il bambino — non sia trapelato nel testo finale.
        metadati = {
            "tecnica_narrativa": tecnica_narrativa,
            "archetipo": frammento_primario.get("archetipo"),
        }
        return prompt_finale.strip(), metadati
