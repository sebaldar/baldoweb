"""Ground narrative observations in the actual engine output, including missing data."""
import json

ASTRONOMY_CRITERIA = (
    "Il tool fornisce fatti geometrici verificati per l'istante indicato: il "
    "racconto non può aumentare il grado di certezza oltre quello che il tool "
    "garantisce. Evita 'di sicuro', 'sicuramente', 'senza dubbio' su "
    "un'osservazione futura anche se la posizione geometrica attuale è nota — "
    "es. non 'la troverai di sicuro' ma 'puoi provare a cercarla'. "
    "Per le osservazioni reali usa esclusivamente i dati astronomici forniti. "
    "Un corpo elencato è geometricamente disponibile: nuvole, ostacoli e luminosità "
    "possono impedirne l'osservazione. Usa la direzione_cardinale del corpo se presente; "
    "se manca non inventare una direzione. L'altezza indica quanto è sopra l'orizzonte, "
    "non il punto cardinale. Non usare la Luna come riferimento vicino a un pianeta "
    "senza dati che ne dimostrino la vicinanza; preferisci la direzione esplicita del pianeta. "
    "Non spostare un corpo basso sull'orizzonte sopra la testa. Rispetta fase e illuminazione "
    "lunare. Non garantire che posizione o visibilità restino uguali domani o fra un anno: "
    "i dati valgono per l'istante indicato. Il meteo disponibile non è automaticamente "
    "una previsione per l'ora futura richiesta. Se manca la previsione evita certezze. "
    "Un foro fra le dita può restringere il campo ma non identifica da solo un pianeta: "
    "serve un riferimento concreto, per esempio la direzione e una mappa consultata insieme. "
    "Restare apparentemente fermo non distingue un pianeta da una stella e non ne prova l’identità. Una mappa aiuta a cercare, ma non prova che si veda attraverso nuvole o edifici. "
    "Quando la richiesta chiede di riconoscere un corpo, scegline uno dai dati "
    "e nominalo nella scena, anche se le nuvole ne impediscono la vista. Una mappa può essere consultata subito: non rinviare "
    "all'indomani l'identificazione richiesta se i dati la permettono. "
    "Non attribuire a un cannocchiale giocattolo dettagli che non può mostrare. "
    "Distingui il gioco immaginario dalle osservazioni e dalle spiegazioni reali. "
    "Se status non è reale o i dati mancano, non inventare un cielo verificato: "
    "il personaggio può consultare una mappa o rimandare l'osservazione. "
    "Segnala affermazioni astronomiche incompatibili o non supportate tra i problemi "
    "di continuità, con correzioni locali; non cambiare una fantasia dichiarata in una lezione."
)


def sky_context(engine_data, **observer):
    return {'osservatore': observer, 'dati_motore': engine_data or {'status': 'non_disponibile', 'corpi': []},
            'meteo_ha_previsione_oraria': False}


def sky_prompt(data):
    return ('\nOSSERVAZIONE ASTRONOMICA: ' + ASTRONOMY_CRITERIA + '\n'
            + json.dumps(sky_context(data), ensure_ascii=False) + '\n')
