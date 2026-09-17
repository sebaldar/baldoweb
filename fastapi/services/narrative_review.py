"""Final review with small, atomic replacements instead of a full rewrite."""
import json
import logging
import re

from services.story_versions import snapshot
from services.story_style import count_similitudes, detect_refrain
from services.sky_context import ASTRONOMY_CRITERIA, sky_context

logger = logging.getLogger(__name__)

ACTION_CRITERIA = (
    "Verifica esplicitamente l'azione che risolve il problema centrale: confronta "
    "posizione iniziale dell'oggetto, ostacolo, strumento, contatto o sostegno, "
    "gesto e risultato. Il testo rende possibile il passaggio dall'una all'altra? "
    "Per tirare un oggetto serve una presa o un aggancio; toccarlo con un bastone "
    "non basta. Per farlo rotolare fuori da una cavità serve un percorso di uscita. "
    "Non inventare uncini, pendenze, legami o poteri che il testo non offre. "
    "Non chiedere una lezione di fisica né dettagli ovvi di azioni quotidiane: "
    "se basta una normale inferenza, accetta la scena. In una magia dichiarata "
    "controlla la regola narrativa, non la fisica. Se il problema non comporta "
    "un'azione materiale, usa non_applicabile."
)

CONTINUITY_CRITERIA = (
    "Confronta la richiesta con l'intero racconto, soprattutto il finale. "
    "Verifica chi deve compiere ciascuna azione richiesta, con chi, verso quale "
    "destinazione e per quale destinatario, e se la compie davvero. Mantieni "
    "distinte casa propria, casa dell'accompagnatore e casa del destinatario. "
    "Controlla che parentele e riferimenti nei dialoghi non cambino senza motivo; "
    "una visita concordata non può diventare due visite separate se la richiesta "
    "prevede di arrivare insieme. Distinguere conoscere la strada da tenere "
    "compagnia: un accompagnamento è valido anche se entrambi conoscono il percorso. "
    "Leggi le frasi adiacenti per stabilire chi parla e a chi risponde. "
    "Segnala contraddizioni o azioni richieste omesse, non preferenze stilistiche. "
    "Controlla anche le premesse esplicite: il freddo da solo non dimostra scarsità di cibo. "
    "Conoscere un percorso non basta a localizzare un gruppo mobile: serve un indizio "
    "o una conoscenza della sua posizione. Se la visibilità è nulla, non si possono "
    "vedere figure lontane senza un cambiamento delle condizioni. "
    "Se un riferimento della richiesta è ambiguo, accetta una lettura plausibile "
    "purché il racconto la mantenga coerente e completi le azioni esplicite. "
    "Non imporre una scena aggiuntiva se una frase mostra già l'arrivo o la consegna. "
)


REQUEST_CRITERIA = (
    " Se è presente una richiesta, restituisci anche verifica_richiesta: una lista "
    "non vuota di oggetti requisito (citazione esatta dalla richiesta), esito "
    "(soddisfatto o mancante), evidenza (citazione esatta dal racconto se soddisfatto, "
    "stringa vuota se mancante). Controlla tutti i vincoli espliciti, incluse le "
    "condizioni: una richiesta alternativa non impone entrambe le alternative. "
    "Se si chiede di scegliere un pianeta reale, il racconto deve nominarlo, anche "
    "se le nuvole ne impediscono la vista: 'il pianeta' o i soli anelli non bastano. "
    "Non inventare vincoli stilistici non richiesti. Per ogni requisito mancante "
    "proponi una correzione locale, se possibile con i dati disponibili. In conferma "
    "ripeti il controllo sul candidato. Non dichiarare soddisfatto un requisito "
    "solo perché il racconto tratta lo stesso argomento."
)


def evidence_quote(quote, text):
    """Whitespace-only matching for evidence; edits still require exact strings."""
    if not isinstance(quote, str) or not quote.strip():
        return None
    if quote in text:
        return quote
    pattern = r'\s+'.join(re.escape(part) for part in quote.split())
    matches = list(re.finditer(pattern, text))
    return matches[0].group() if len(matches) == 1 else None


def validate_request(payload, text, request):
    if not request:
        return []
    checks = payload.get('verifica_richiesta')
    if not isinstance(checks, list) or not checks:
        raise ValueError('Verifica della richiesta assente')
    for check in checks:
        if not isinstance(check, dict):
            raise ValueError('Requisito non valido')
        requirement, verdict, evidence = (check.get(k) for k in ('requisito', 'esito', 'evidenza'))
        if not isinstance(requirement, str) or not requirement.strip() or requirement not in request:
            raise ValueError('Requisito non presente nella richiesta')
        if verdict not in ('soddisfatto', 'mancante') or not isinstance(evidence, str):
            raise ValueError('Esito requisito non valido')
        if verdict == 'soddisfatto' and not evidence_quote(evidence, text):
            raise ValueError(f'Evidenza requisito assente dal racconto: requisito={requirement!r}, evidenza={evidence!r}')
        if verdict == 'soddisfatto':
            check['evidenza'] = evidence_quote(evidence, text)
        if verdict == 'mancante' and evidence:
            raise ValueError('Requisito mancante con evidenza contraddittoria')
    return checks


def response_format(confirm=False):
    """Both calls receive a complete example, including the required action audit."""
    example = {
        'modifiche': [],
        'verifica_richiesta': [{'requisito': 'citazione esatta dalla richiesta', 'esito': 'soddisfatto', 'evidenza': 'citazione esatta dal racconto'}],
        'continuita_narrativa': {'esito': 'coerente', 'problemi': []},
        'azione_decisiva': {
            'passaggio': 'citazione esatta dal racconto ricevuto',
            'esito': 'coerente',
            'collegamento_mancante': '',
        },
    }
    edits = (
        'Se una correzione è invalida, inserisci in modifiche oggetti con il campo '
        'motivo; non proporre altre riscritture. Se tutte le correzioni sono valide, '
        'modifiche deve essere una lista vuota: non ricopiare le modifiche approvate '
        'e non inserire oggetti con motivo valida. '
        if confirm else
        'Per ogni correzione inserisci in modifiche un oggetto con originale '
        '(citazione esatta e univoca), sostituzione e motivo. Se l’azione è '
        'incompleta proponi una modifica locale realizzabile con le risorse presenti. '
    )
    return (
        ' Restituisci soltanto un oggetto JSON con i campi dei controlli obbligatori, '
        'anche quando non ci sono modifiche: ' + json.dumps(example, ensure_ascii=False) +
        '. ' + edits +
        'azione_decisiva descrive il racconto ricevuto in questo passaggio, '
        'prima delle eventuali modifiche proposte. Gli esiti ammessi sono coerente, '
        'incompleta, non_applicabile. Se incompleta, cita il gesto e specifica '
        'il collegamento_mancante; se coerente lascia collegamento_mancante vuoto. '
        'Se non_applicabile lascia passaggio e collegamento_mancante vuoti. '
        'continuita_narrativa deve contenere esito (coerente o incoerente) e problemi. '
        'Se coerente, problemi è vuoto. Se incoerente, ogni problema contiene passaggio '
        '(una citazione breve, esatta e continua, senza omissioni o puntini di sospensione, '
        'dal racconto che evidenzia la contraddizione o il finale '
        'incompleto) e motivo (quale azione, destinazione o relazione non torna). '
        'Proponi correzioni locali anche per le incoerenze, salvo che questa sia la '
        'conferma. Non omettere nessuno dei controlli nella conferma: valuta il candidato.'
    )


def validate_action(payload, text):
    action = payload.get('azione_decisiva')
    if not isinstance(action, dict):
        raise ValueError('Verifica dell’azione decisiva assente')
    verdict = action.get('esito')
    quote, missing = action.get('passaggio'), action.get('collegamento_mancante')
    if verdict not in ('coerente', 'incompleta', 'non_applicabile'):
        raise ValueError('Esito azione decisiva non valido')
    if not isinstance(quote, str) or not isinstance(missing, str):
        raise ValueError('Descrizione azione decisiva non valida')
    if verdict != 'non_applicabile':
        matched = evidence_quote(quote, text)
        if not matched:
            raise ValueError(f'Citazione azione decisiva assente dal testo: {quote!r}')
        action['passaggio'] = matched
    if verdict == 'incompleta' and not missing.strip():
        raise ValueError('Il passaggio mancante deve essere specificato')
    if verdict != 'incompleta' and missing.strip():
        raise ValueError('Esito in contrasto con il passaggio mancante')
    return action


def continuity_quote(quote, text):
    """Expand ellipses in diagnostic evidence only; never use this to apply edits."""
    if not isinstance(quote, str) or not quote.strip():
        raise ValueError('Citazione continuità assente dal testo')
    matched = evidence_quote(quote, text)
    if matched:
        return matched
    fragments = re.split(r'\s*(?:\.{3}|…)\s*', quote)
    if len(fragments) < 2 or any(len(part.strip()) < 12 for part in fragments):
        raise ValueError('Citazione continuità assente dal testo')
    start, cursor = None, 0
    for fragment in fragments:
        position = text.find(fragment, cursor)
        if position < 0:
            raise ValueError('Citazione continuità assente dal testo')
        if start is None:
            start = position
        cursor = position + len(fragment)
    return text[start:cursor]


def validate_continuity(payload, text):
    audit = payload.get('continuita_narrativa')
    if not isinstance(audit, dict) or audit.get('esito') not in ('coerente', 'incoerente'):
        raise ValueError('Verifica della continuità narrativa assente o non valida')
    problems = audit.get('problemi')
    if not isinstance(problems, list) or bool(problems) != (audit['esito'] == 'incoerente'):
        raise ValueError('Esito continuità in contrasto con i problemi')
    for problem in problems:
        if not isinstance(problem, dict):
            raise ValueError('Problema di continuità non valido')
        quote, reason = problem.get('passaggio'), problem.get('motivo')
        problem['passaggio'] = continuity_quote(quote, text)
        if not isinstance(reason, str) or not reason.strip():
            raise ValueError('Motivazione continuità assente')
    return audit


REVIEW_CRITERIA = (
    "Controlla errori concreti: anatomia e azioni compatibili con la creatura; "
    "grammatica; continuità di oggetti e luoghi; conoscenze dei personaggi motivate; "
    "nesso tra indizio e soluzione. Parlare e vestirsi sono convenzioni fiabesche "
    "lecite, ma non attribuiscono automaticamente un'anatomia diversa. "
    "Un rumore può condurre a un incontro, non dimostrare la direzione di casa; "
    "chi accompagna deve conoscere la destinazione. La conoscenza può essere "
    "dichiarata in un dialogo: aver visitato un luogo, visto passare una famiglia "
    "o percorso una strada è una motivazione sufficiente, salvo contraddizioni "
    "esplicite. Non esigere un flashback o una dimostrazione ulteriore. Prima "
    "di segnalare un’informazione immotivata, leggi tutta la scena e controlla "
    "se un personaggio la spiega o se l’azione richiesta avviene già dopo. "
    "Distingui 'conosco quel sentiero, passa vicino alla tua casa' (esperienza "
    "dichiarata valida) da 'tutti i sentieri in discesa portano a casa' "
    "(deduzione infondata). Non imporre un accompagnamento già presente o "
    "sostituirlo a indicazioni motivate solo per preferenza. Distingui metafore innocue "
    "da affermazioni sul mondo usate come spiegazioni vere: scorrere verso il basso "
    "non implica arrivare a una destinazione, e un animale non è un fenomeno "
    "atmosferico. Controlla anche le affermazioni dei dialoghi: la voce di un "
    "personaggio non rende vera una spiegazione falsa. Non trasformare una "
    "fantasia esplicita in realismo: verifica solo che le sue possibilità siano coerenti. "
    "Se un tentativo fallisce, il successivo deve usare una nuova azione o informazione. "
    "Non inventare problemi per giustificare una revisione. " + ACTION_CRITERIA
)


def apply_edits(text, edits):
    """Validate against the original, then apply non-overlapping edits atomically."""
    if not isinstance(edits, list) or len(edits) > 5:
        raise ValueError("Sono ammesse al massimo cinque modifiche locali")
    spans = []
    for edit in edits:
        if not isinstance(edit, dict):
            raise ValueError("Modifica non valida")
        old, new, reason = (edit.get(k) for k in ('originale', 'sostituzione', 'motivo'))
        if not isinstance(old, str) or not old or text.count(old) != 1:
            raise ValueError("Il passaggio originale deve comparire esattamente una volta")
        if not isinstance(new, str) or not isinstance(reason, str) or not reason.strip() or old == new:
            raise ValueError("Sostituzione o motivazione non valida")
        start = text.index(old)
        spans.append((start, start + len(old), new))
    spans.sort()
    if any(left[1] > right[0] for left, right in zip(spans, spans[1:])):
        raise ValueError("Modifiche sovrapposte")
    if sum(end - start for start, end, _ in spans) > len(text) * .4:
        raise ValueError("Revisione troppo estesa")
    result = text
    for start, end, replacement in reversed(spans):
        result = result[:start] + replacement + result[end:]
    if not .6 * len(text) <= len(result) <= 1.3 * len(text):
        raise ValueError("Lunghezza della revisione non valida")
    return result


async def verifica_testo_finale(state, llm):
    original = state.get('racconto_finale') or ''
    if not original:
        return {}
    usage, proposed = [], []
    candidate = original
    astronomy_active = bool(state.get('usa_astronomia') or state.get('dati_astronomici'))
    astronomy_instructions = ASTRONOMY_CRITERIA if astronomy_active else ''
    system = (
        "Rileggi il testo finale di una storia per bambini. " + REQUEST_CRITERIA + CONTINUITY_CRITERIA + REVIEW_CRITERIA + astronomy_instructions +
        " Intervieni sullo stile solo per una morale esplicitata o una spiegazione "
        "che ripete ciò che un gesto ha già mostrato, oppure per una coda che ripete "
        "la chiusura già compiuta senza aggiungere eventi. In quel caso elimina solo "
        "le frasi ridondanti, senza inventare una nuova conclusione. Non imporre un "
        "numero di similitudini e non cancellare un’immagine solo perché poetica. Conserva lessico personale, "
        "umorismo, ritmo e immagini riuscite. Non normalizzare la voce. "
        "Esamina tutte queste categorie prima di rispondere, senza fermarti al primo "
        "errore trovato. Proponi solo sostituzioni locali indispensabili, al massimo cinque, "
        "che insieme risolvano il difetto senza contraddire il resto. Conserva chi "
        "compie ciascuna azione e chi raggiunge chi: non invertire i ruoli nei "
        "dialoghi. Per togliere una falsa spiegazione preferisci eliminarla e "
        "usare un fatto già mostrato, senza inventare una spiegazione sostitutiva. Non riscrivere "
        "interi paragrafi per abbellirli. Non aggiungere personaggi, saluti, ritornelli "
        "o domande; conserva i nomi richiesti. " + response_format()
    )
    status, detail = 'non_verificato', None
    initial_action = final_action = None
    initial_continuity = final_continuity = None
    initial_request = final_request = None
    evidence_repair_used = False
    evidence_errors = []
    try:
        # One correction round, then verify the complete candidate before accepting it.
        for attempt in range(2):
            phase = 'verifica_testo_finale' if attempt == 0 else 'verifica_testo_finale.conferma'
            review_system = system if attempt == 0 else (
                "Verifica esclusivamente le sostituzioni proposte confrontando originale e "
                "candidato. Ogni modifica risolve davvero il difetto indicato? Introduce "
                "errori fattuali, contraddizioni, nomi estranei o informazioni arbitrarie? "
                "Verifica in particolare soggetto, pronomi e direzione delle azioni: "
                "se A ha raggiunto B, B non può affermare di aver raggiunto A. "
                "Una correzione non deve attribuire nuovi poteri o spostamenti a "
                "un elemento naturale per sostituire una spiegazione falsa. "
                "Non riaprire una revisione stilistica e non cercare difetti preesistenti "
                "estranei alle modifiche. Accetta una correzione locale valida anche se "
                "preferiresti una frase diversa. "
                "Una conoscenza dichiarata dal personaggio è valida salvo contraddizioni. "
                + REQUEST_CRITERIA + CONTINUITY_CRITERIA + ACTION_CRITERIA + astronomy_instructions + response_format(confirm=True)
            )
            context = {'richiesta': state.get('prompt_originale'),
                       'eta': state.get('eta_bambino'), 'racconto': candidate}
            if astronomy_active:
                context['contesto_astronomico'] = sky_context(
                    state.get('dati_astronomici'), luogo=state.get('luogo'),
                    data=state.get('data_storia'), ora_locale=state.get('ora_storia'),
                    fuso='Europe/Rome', meteo_disponibile=state.get('dati_meteo'))
            if attempt:
                context.update(originale=original, modifiche=proposed)
            response = await llm.chiedi(
                system=review_system,
                user=json.dumps(context, ensure_ascii=False),
                fase=phase,
            )
            usage.append({'nodo': phase, 'modello': response.modello,
                          'token_input': response.token_input, 'token_output': response.token_output,
                          'durata_secondi': round(response.durata_secondi, 2)})
            raw = response.testo.strip()
            fenced = re.fullmatch(r"```(?:json)?\s*\n([\s\S]*?)\n```", raw)
            payload = json.loads(fenced.group(1) if fenced else raw)
            if not isinstance(payload, dict) or not isinstance(payload.get('modifiche'), list):
                raise ValueError('Risposta del revisore non valida')
            validators = {
                'azione_decisiva': lambda data: validate_action(data, candidate),
                'continuita_narrativa': lambda data: validate_continuity(data, candidate),
                'verifica_richiesta': lambda data: validate_request(data, candidate, state.get('prompt_originale')),
            }
            checked, invalid = {}, {}
            for field, validate in validators.items():
                try:
                    checked[field] = validate(payload)
                except ValueError as error:
                    invalid[field] = str(error)
                    evidence_errors.append({'fase': phase, 'controllo': field,
                                            'errore': str(error), 'valore': payload.get(field)})
            if attempt == 0:
                initial_action = final_action = checked.get('azione_decisiva')
                initial_continuity = final_continuity = checked.get('continuita_narrativa')
                initial_request = final_request = checked.get('verifica_richiesta')
            if invalid:
                if evidence_repair_used:
                    raise ValueError(json.dumps(invalid, ensure_ascii=False))
                evidence_repair_used = True
                repair = await llm.chiedi(
                    system=("Ripara solo i controlli indicati in errori. Non cambiare il racconto "
                            "né proporre modifiche. Copia citazioni esatte e continue, senza "
                            "parafrasi. Mantieni i requisiti pertinenti e valuta onestamente "
                            "gli esiti: una citazione inesistente non dimostra coerenza. "
                            "Restituisci JSON con i soli campi da riparare. "
                            + ACTION_CRITERIA + CONTINUITY_CRITERIA + REQUEST_CRITERIA + response_format()
                            + " In questa riparazione ignora la richiesta di proporre sostituzioni: "
                            "restituisci solo i controlli errati, senza modifiche."),
                    user=json.dumps({**context, 'errori': invalid,
                                     'controlli_da_riparare': {field: payload.get(field) for field in invalid}}, ensure_ascii=False),
                    fase=phase)
                usage.append({'nodo': phase + '.ripara_evidenze', 'modello': repair.modello,
                              'token_input': repair.token_input, 'token_output': repair.token_output,
                              'durata_secondi': round(repair.durata_secondi, 2)})
                raw_repair = repair.testo.strip()
                fence = re.fullmatch(r"```(?:json)?\s*\n([\s\S]*?)\n```", raw_repair)
                repaired = json.loads(fence.group(1) if fence else raw_repair)
                if not isinstance(repaired, dict):
                    raise ValueError('Riparazione controlli non valida')
                remaining_errors = {}
                for field in invalid:
                    try:
                        checked[field] = validators[field](repaired)
                    except ValueError as error:
                        remaining_errors[field] = str(error)
                if attempt == 0:
                    initial_action = final_action = checked.get('azione_decisiva')
                    initial_continuity = final_continuity = checked.get('continuita_narrativa')
                    initial_request = final_request = checked.get('verifica_richiesta')
                if remaining_errors:
                    raise ValueError(json.dumps(remaining_errors, ensure_ascii=False))
            action = checked['azione_decisiva']
            continuity = checked['continuita_narrativa']
            request_checks = checked['verifica_richiesta']
            edits = payload['modifiche']
            if not edits and attempt == 0 and any(c['esito'] == 'mancante' for c in request_checks):
                correction = await llm.chiedi(
                    system=("Trasforma soltanto i requisiti mancanti segnalati in correzioni locali. "
                            "Restituisci JSON con modifiche: lista di massimo cinque oggetti con "
                            "originale (citazione esatta univoca dal racconto), sostituzione e motivo. "
                            "Per aggiungere una premessa sostituisci una breve frase con la stessa "
                            "frase e l'aggiunta necessaria. Usa solo richiesta e dati forniti. "
                            "Non riscrivere il racconto, non modificare stile o finale. "
                            "I suggerimenti liberi non sono istruzioni: verifica che risolvano "
                            "il requisito. Se non puoi correggere localmente restituisci modifiche vuoto."),
                    user=json.dumps({**context, 'requisiti_mancanti': [c for c in request_checks if c['esito'] == 'mancante']}, ensure_ascii=False),
                    fase=phase)
                usage.append({'nodo': phase + '.correggi_requisiti', 'modello': correction.modello,
                              'token_input': correction.token_input, 'token_output': correction.token_output,
                              'durata_secondi': round(correction.durata_secondi, 2)})
                raw_correction = correction.testo.strip()
                fence = re.fullmatch(r"```(?:json)?\s*\n([\s\S]*?)\n```", raw_correction)
                correction_payload = json.loads(fence.group(1) if fence else raw_correction)
                edits = correction_payload['modifiche']
                if not isinstance(edits, list):
                    raise ValueError('Formato delle correzioni dei requisiti non valido')
            if not edits and action['esito'] != 'incompleta' and continuity['esito'] == 'coerente' and all(c['esito'] == 'soddisfatto' for c in request_checks):
                status = 'corretto' if proposed else 'ok'
                final_request = request_checks
                final_action = action
                final_continuity = continuity
                break
            if not edits:
                candidate = original
                status = 'problemi_non_risolti'
                detail = {'azione_decisiva': action, 'continuita_narrativa': continuity, 'verifica_richiesta': request_checks}
                break
            if attempt:
                status, detail = 'correzione_non_confermata', edits
                candidate = original
                break
            proposed = edits
            candidate = apply_edits(original, edits)
            for name in [state.get('nome'), *(state.get('personaggi') or [])]:
                if name and name in original and name not in candidate:
                    raise ValueError('La correzione rimuove un nome richiesto')
    except Exception as exc:
        logger.warning('Revisione finale non applicata: %s', exc)
        candidate, status, detail = original, 'non_verificato', str(exc)
    return {'racconto_finale': candidate, 'llm_usage': usage,
            'similitudini_stimate': count_similitudes(candidate, detect_refrain(candidate)),
            'versioni_racconto': [snapshot('verifica_testo_finale', candidate, usage, esito=status)],
            'revisione_finale': {'esito': status, 'modifiche_proposte': proposed,
                                 'dettaglio': detail,
                                 'errori_evidenze': evidence_errors,
                                 'richiesta_iniziale': initial_request,
                                 'richiesta_finale': final_request,
                                 'azione_decisiva_iniziale': initial_action,
                                 'azione_decisiva_finale': final_action,
                                 'continuita_narrativa_iniziale': initial_continuity,
                                 'continuita_narrativa_finale': final_continuity}}
