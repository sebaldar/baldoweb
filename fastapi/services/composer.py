"""
services/composer.py
====================
Si occupa di assemblare il prompt finale per l'LLM, integrando:
1. Analisi del prompt (personaggi, emozioni)
2. Frammenti recuperati da Neo4j
3. Contesto Fisico (Meteo, Coordinate, Astronomia)
"""

import logging

logger = logging.getLogger(__name__)

class StoryComposer:
    def __init__(self):
        self.base_instruction = (
            "Sei Baldo, un anziano astrologo e contastorie gentile che vive in una torre antica. "
            "Usi un linguaggio magico, rassicurante e adatto a bambini. "
            "La tua caratteristica unica è che intrecci sempre la realtà fisica (meteo e stelle) "
            "con la fantasia dei frammenti di storia."
        )

    def componi(self, prompt_originale: str, analisi: dict, frammenti: list, **kwargs) -> str:
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

        # --- COSTRUZIONE SEZIONE FRAMMENTI ---
        testo_frammenti = ""
        if frammenti:
            testo_frammenti = "\n".join([f"- {f.get('testo')}" for f in frammenti])
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
        # Il frammento non spiega la tecnica, la applica: Baldo la deduce dal
        # nome e dall'esempio di testo già fornito sopra tra i frammenti.
        tecniche = sorted({
            f.get("tecnica_narrativa") for f in (frammenti or []) if f.get("tecnica_narrativa")
        })
        sezione_tecnica = ""
        if tecniche:
            sezione_tecnica = (
                f"\nTECNICA NARRATIVA DA APPLICARE: {', '.join(tecniche)}. "
                "Osserva come il frammento sopra la mette in pratica e usa lo stesso "
                "dispositivo narrativo per costruire la tua storia.\n"
            )

        # --- COSTRUZIONE SEZIONE DOMANDA FINALE ---
        # Prende la prima disponibile: i frammenti arrivano già ordinati per
        # rilevanza da cerca_frammenti, quindi è quella del frammento migliore.
        domanda_finale = next(
            (f.get("domanda") for f in (frammenti or []) if f.get("domanda")), None
        )
        sezione_domanda = ""
        if domanda_finale:
            sezione_domanda = (
                f"\nDOMANDA FINALE: Concludi il racconto rivolgendo al bambino "
                f"questa domanda (adattala al contesto se serve): \"{domanda_finale}\"\n"
            )

        # --- COSTRUZIONE SEZIONE RITORNELLO ---
        # Stessa logica di domanda_finale: prende quello del frammento più
        # rilevante. Va ripetuto più volte (non solo citato), è il suo scopo.
        ritornello = next(
            (f.get("ritornello") for f in (frammenti or []) if f.get("ritornello")), None
        )
        sezione_ritornello = ""
        if ritornello:
            sezione_ritornello = (
                f"\nRITORNELLO: Ripeti questa frase 2-3 volte durante il racconto, "
                f"nei momenti chiave, sempre uguale — è pensata per essere riconosciuta "
                f"e ripetuta ad alta voce dal bambino: \"{ritornello}\"\n"
            )

        # --- COSTRUZIONE SEZIONE CIELO ---
        descrizione_cielo = f"La luna è in fase {fase_luna}."
        if corpi_celesti:
            nomi_corpi = [c.get("nome") for c in corpi_celesti]
            descrizione_cielo += f" In cielo sono visibili: {', '.join(nomi_corpi)}."

        # --- TEMPLATE FINALE ---
        prompt_finale = f"""
{self.base_instruction}

CONTESTO REALE (Usa questi dettagli per l'ambientazione):
- Luogo: {luogo}
- Data e Ora: {data} alle {ora}
- Meteo attuale: {meteo}
- Cielo: {descrizione_cielo}

ELEMENTI DELLA STORIA RICHIESTI:
- Prompt Utente: "{prompt_originale}"
- Personaggi identificati: {', '.join(analisi.get('personaggi', []))}
- Emozioni da evocare: {', '.join(analisi.get('emozioni', []))}
{sezione_bio}
FRAMMENTI DI TRAMA DAL DATABASE (Integrali nella narrazione):
{testo_frammenti}
{sezione_tecnica}{sezione_ritornello}{sezione_domanda}
REGOLE DI GENERAZIONE:
1. Rivolgiti al bambino con dolcezza.
2. Inizia menzionando il meteo o le stelle che Baldo vede dalla sua torre a {luogo}.
3. La lunghezza deve essere {kwargs.get('lunghezza', 'media')}.
4. Rispondi esclusivamente in lingua: {kwargs.get('lingua', 'Italiano')}.
5. Età del bambino: {kwargs.get('eta_bambino', 4)} anni (usa un vocabolario appropriato).

GENERA IL RACCONTO:
"""
        return prompt_finale.strip()
