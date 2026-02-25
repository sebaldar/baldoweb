#include <stdio.h>

#include <stdlib.h>  		// per atoi
#include <string>
#include <iostream>     // std::cout
#include <sstream>      // std::istringstream
#include <vector>
#include <math.h>

#ifndef _utils_
    #include <Utils.h>
#endif

#ifndef _hash_
    #include <Hash.h>
#endif

#ifndef _TNODO_
	#include <ITRNode.hpp>
#endif


#ifndef _interprete_
	#include <Interprete.hpp>
#endif  // #ifndef _interprete_

// definizione parole chiave utilizzate dall'interprete



TINTERPRETE::TINTERPRETE() :
	compiled(nullptr),
	isCompiled(false),
	$command(NULL)
{

	Esp = new TESPRESSIONE();

	// tabella hash per la memorizzazione delle etichette
	labels = new HashClass;

}

TINTERPRETE:: ~TINTERPRETE()
{

	// libera la memoria allocata per i programmi
	delete Esp;
	delete labels;

	delete compiled;

	// libera la memoria della tabella degli identificatori
	ClearVariables ();

}

void TINTERPRETE::SetExt$Funz( $EXT_FUNZ *n, $FUNZ f )
{
		Esp->$node_funz = n;
		Esp->$funz = f;
}

void TINTERPRETE::SetCommandFunz( $EXT_FUNZ *n,  $COMMAND f )
{
// funziona sia sull'interprete che sull'espressione
		Esp->$node_funz = n;
		Esp->$command = f;

		$node_funz = n;
		$command = f;

}

void TINTERPRETE::error( const char *msg)
{

	isError = true;

	throw    ( string (msg) ) ;

}

void TINTERPRETE::InsertExtFunction  ( const char * nome_funzione, EXT_FUNZ funzione) {
		Esp->DefinisciFunzioneEsterna ( nome_funzione, funzione) ;
}

TINTERPRETE::ComTag TINTERPRETE::GetTagCommand( const char * com) const
{


	// tabella dei comandi
	// ricava il numero di comanto
	struct {

		const char *label;
		ComTag tag;

	}   const CommandTab[] = {

			{_ESPRESSIONE, 	COM_ESPRESSIONE},
			{_COMMAND, 	COM_COMMAND},
			{_VAI, 			COM_VAI},
			{_LABEL, 		COM_LABEL},
			{_SE, 			COM_SE},
			{_MENTRE, 		COM_MENTRE},
			{_INIZIA, 		COM_INIZIA},
			{_NUMERO, 		COM_NUMERO},
			{_STRINGA, 		COM_STRINGA},
			{_TABELLA, 		COM_TABELLA},
			{_MATRICE, 		COM_MATRICE},
			{_STAMPA, 		COM_STAMPA},
			{_FUNZIONE, 	COM_FUNZIONE},
			{_FINE, 		COM_FINE},
			{_CICLO_PER, 	COM_CICLO_PER},
			{"//", 			COM_COMMENTO},
			{"'", 			COM_COMMENTO}

    	};

	int sizecom = sizeof CommandTab / sizeof CommandTab[0];

	const char * iniz = com;
	while ( isspace(*iniz) ) iniz++;
	int i = sizecom - 1;
	for (; i >= 0; i--) {
		int len = strlen(CommandTab[i].label);
		if (lstrncmp(iniz, CommandTab[i].label, len) == 0)
			if ( iniz[len] == ' ' || iniz[len] == '\0' )
				break;
	}


	if ( *iniz == '\0' ) return COM_COMMENTO;

	if (i >= 0)	return CommandTab[i].tag;
	else return COM_ESPRESSIONE;

}


// ritorna il nodo finale
BASE_NODE * TINTERPRETE::CompileProgrammRows (
		BASE_NODE *current,
		BASE_NODE *end_node,
		char **prog,
		int n_righe)
{


	// esegue un nuovo scan
	for (int i = 0; i < n_righe; ++i) {

		char *riga = new char [strlen(prog[i]) + 1] ;
		strcpy(riga, DelBlk(prog[i])) ;

		current = CompileRow (current, end_node, riga) ;

		// verifica se è un nodo SE
		if ( !strcmp(current->get_class(), _SE ) ) {
			// se è un nodo SE compila il nodo ALLORA
			// passa alla riga successiva
			i++;

			char ** true_ptr ;
			int n_row ;  // righe da compilare
			BASE_NODE *true_node = new START_NODE;


			// se c'è tag COM_INIZIA allora esegui fino a COM_FINE
			if (GetTagCommand (prog[i]) == COM_INIZIA) {

				n_row = RicavaRigheSottoProgramma (prog, i, n_righe);

				true_ptr = &prog[i + 1];    // inizio
				i = i + n_row + 1;
			}
			else {   // altrimenti esegui solo la riga successiva
				true_ptr = &prog[i];
				n_row = 1;
			}	// if (GetTagCommand (prog[i]) == COM_INIZIA) {

			CompileProgrammRows(true_node, end_node, true_ptr, n_row);
			((SE_NODE *) current)->SetCondVero(true_node);

			// verifica se la riga successiva è ALTRIMENTI
			if ((i < n_righe - 1) && !strcmp(prog[i + 1], _ALTRIMENTI)) {

				// se è un nodo SE compila il nodo ALTRIMENTI
				// passa alla riga successiva
				i = i + 2;

				char ** false_ptr ;
				int n_row ;  // righe da compilare

				BASE_NODE *false_node = new START_NODE;

				// se c'è tag COM_INIZIA allora esegui fino a COM_FINE
				if (GetTagCommand (prog[i]) == COM_INIZIA) {

					n_row = RicavaRigheSottoProgramma (prog, i, n_righe);

					false_ptr = &prog[i + 1];    // inizio
					i += n_row + 1;
				}
				else {   // altrimenti esegui solo la riga successiva

					false_ptr = &prog[i];
					n_row = 1;
				}

				CompileProgrammRows(false_node, end_node, false_ptr, n_row);
				((SE_NODE *) current)->SetCondFalso(false_node);

			}	// if ((i < n_righe - 1) && !strcmp(prog[i + 1], _ALTRIMENTI)) {
		}	//	if (!strcmp(current->get_class(), _SE)) {


		// verifica se è un nodo MENTRE
		if (!strcmp(current->get_class(), _MENTRE)) {

			// se è un nodo mentre compila il nodo condizione vera
			// passa alla riga successiva

			i++;

			char ** true_ptr ;
			int n_row ;  // righe da compilare
			BASE_NODE *true_node = new START_NODE;

			// se c'è tag COM_INIZIA allora esegui fino a COM_FINE
            if (GetTagCommand (prog[i]) == COM_INIZIA) {

				n_row = RicavaRigheSottoProgramma (prog, i, n_righe);

				true_ptr = &prog[i + 1];    // inizio
				i = i + n_row + 1;
			}
			else {   // altrimenti esegui solo la riga successiva
				true_ptr = &prog[i];
				n_row = 1;
			}

			CompileProgrammRows(true_node, end_node, true_ptr, n_row);
			((MENTRE_NODE *) current)->SetCondVero(true_node);
		}


		// verifica se è un nodo CICLO_PER
		if (!strcmp(current->get_class(), _CICLO_PER)) {

			// se è un nodo mentre compila il nodo condizione vera
			// passa alla riga successiva

			i++;

			char ** true_ptr ;
			int n_row ;  // righe da compilare
			BASE_NODE *true_node = new START_NODE;


			// se c'è tag COM_INIZIA allora esegui fino a COM_FINE
			if (GetTagCommand (prog[i]) == COM_INIZIA) {

				n_row = RicavaRigheSottoProgramma (prog, i, n_righe);

				true_ptr = &prog[i + 1];    // inizio
				i = i + n_row + 1;
			}
			else {   // altrimenti esegui solo la riga successiva
				true_ptr = &prog[i];
				n_row = 1;
			}

			CompileProgrammRows(true_node, end_node, true_ptr, n_row);
			((CICLO_PER_NODE *) current)->SetCondVero(true_node);
		}

		delete [] riga ;

	}	//		for (int i = 0; i < n_righe; ++i) {

	return current;

}

BASE_NODE * TINTERPRETE::CompileRow (
	BASE_NODE *current,
	BASE_NODE *end_node,
	char *riga)
{


	TESPRESSIONE &espr = *Esp;      // utilizza il membro interno Esp


	ComTag COMANDO = GetTagCommand (riga) ;

	switch (COMANDO) {

		case COM_ESPRESSIONE :
			if (current)
				current = current->next = new ESP_NODE (espr.MakeTree(riga));
			else
				current = new ESP_NODE (espr.MakeTree(riga));
			break;

		case COM_VAI : {
			// la sintassi é VAI seguita dalla etichetta
			char *etichetta = riga + strlen(_VAI);
			etichetta = DelBlk(etichetta);

			if (strcmp(etichetta, _FINE) == 0) {
				current->next = end_node ;
				current = NULL;
			}
			else  {
				BASE_NODE *label_node =
					* (BASE_NODE **) labels->GetData (etichetta);
				// se non esiste segnala l'errore
				if (!label_node) {

					string label = etichetta;
					string  str = "Etichetta " + label + " non dichiarata";
					error( str.c_str() );

				}

				current->next = label_node ;
				current = NULL;

			}

			break;

		}

		case COM_STAMPA : {

			// la sintassi é STAMPA seguita dalla stringa da stampare
			char *str = riga + strlen(_STAMPA);

			if (current)
				current = current->next = new STAMPA_NODE ( str );
			else
				current = new STAMPA_NODE ( str );

		}
			break;

		case COM_COMMAND : {

			// la sintassi é COMMAND seguita dal comando da eseguire
			char *str = riga + strlen(_COMMAND);

			if (current)
				current = current->next = new COMMAND_NODE ( this, str );
			else
				current = new COMMAND_NODE ( this, str );

		}
			break;

		case COM_LABEL : {
			// la sintassi é LABEL seguita dalla etichetta
			char *etichetta = riga + strlen(_LABEL);

			etichetta = DelBlk(etichetta);

			if (current)
				current =	current->next = * (BASE_NODE **) labels->GetData (etichetta);
			else current = * (BASE_NODE **) labels->GetData (etichetta);
		}

		break;

		case COM_SE : {

			// sintassi SE condizione ALLORA istruzione ALTRIMENTI istruzione

			char * str = strstr(riga, _ALLORA);

			if ( str ) *str = '\0';

			char *cond = riga + strlen(_SE);
			cond = DelBlk(cond);

			if (current)
				current = current->next = new SE_NODE (espr.MakeTree(cond));
			else
				current = new SE_NODE (espr.MakeTree(cond));

		}   //  			case COM_SE

		break;

		case COM_MENTRE : {

			// sintassi MENTRE condizione ESEGUI istruzione

			char * str = strstr(riga, _ESEGUI);
			if (str) *str = '\0';

			char *cond = riga + strlen(_MENTRE);
			cond = DelBlk(cond);

			if (current)
				current = current->next = new MENTRE_NODE (espr.MakeTree(cond));
			else
				current = new MENTRE_NODE (espr.MakeTree(cond));

		}   //  			case COM_MENTRE

		break;

		case COM_CICLO_PER : {

			// sintassi PER (inizializzazione; condizione; incremento )

			char * str, *str1 ;
			str = riga + strlen(_CICLO_PER);

			while (*str == 32) str++;

			if (*str++ != '(') {
				string r = riga;
				string str = r + ": Manca parentesi aperta";
				error (str.c_str());
			}


			char *scan = str;


			// cerca la parentesi chiusa;
			int n_open_braket = 0 ;
			while (*scan && (*++scan != ')' || n_open_braket  > 0)) {

				if (*scan == '(') n_open_braket++;
				if (*scan == ')') n_open_braket--;

			}


			if (!*str) {
				string r = riga;
				string str = r + ": Manca parentesi chiusa";
				error (str.c_str());
			}


			*(scan++)  = '\0';
			str1 = strchr (str, ';');

			TNODO * N_iniz ;

			if (str1) {
				*str1++ = '\0';
				N_iniz = espr.MakeTree(DelBlk(str)) ;
			}



			str = strchr (str1, ';');




			TNODO * N_cond ;

			if (str) {
				*str++ = '\0';
				N_cond = espr.MakeTree(DelBlk(str1)) ;
			}


			TNODO * N_incr ;

			N_incr = espr.MakeTree(DelBlk(str)) ;

			char *fine = scan ;    // siamo dopo la parentesi chiusa
			str = strstr(fine, _ESEGUI);

			if (str) *str = '\0';


			if (current)
				current = current->next = new CICLO_PER_NODE (N_iniz, N_cond, N_incr);
			else
				current = new CICLO_PER_NODE (N_iniz, N_cond, N_incr);

		}   //  			case COM_CICLO_PER


		break;


		case COM_INIZIA :

			// genera un'altro programma da interpretare fino alla parola chiave FINE

		break;


		case COM_FINE :
			// se si é raggiunto fine blocco in questo punto vuol dire
			// che manca inizio blocco
 			error( "Manca inizio blocco");
			break;

		case COM_COMMENTO :
				// se é un commento non fare nulla
			break;
		case COM_NUMERO :
			break;
		case COM_STRINGA :
			break;
		case COM_MATRICE :
		case COM_TABELLA :
			break;
		case COM_FUNZIONE :
				// non fare nulla
			break;
	} // end switch COMANDO


	return current;

}

// ritorna il nodo iniziale
BASE_NODE	*TINTERPRETE::CompileProgramm ( const char *source )
{


	std::string prog = source;

	std::vector< string > programma;

	//  analizza il sorgente, se trova degli ALLORA non seguiti
	// da /n lo mette, lo stesso per ALTRIMENTI E MENTRE

	size_t pos = 0;
	do {
		pos = prog.find (  _ALLORA, pos ) ;
		if ( pos != string::npos ) {
			pos += strlen( _ALLORA );
			// elimina spazi e tab
			while ( isspace( prog[ pos ] ) ) pos++;
			if ( prog[ pos ] != '\n' )
				prog[ pos - 1 ] = '\n' ;
		}
	} while ( pos != string::npos ) ;

	pos = 0;
	do {
		pos = prog.find (  _ESEGUI, pos ) ;
		if ( pos != string::npos ) {
			pos += strlen( _ESEGUI );
			// elimina spazi e tab
			while ( isspace( prog[ pos ] ) ) pos++;
			if ( prog[ pos ] != '\n' )
				prog[ pos - 1 ] = '\n' ;
		}
	} while ( pos != string::npos ) ;

	pos = 0;
	do {
		pos = prog.find (  _ALTRIMENTI, pos ) ;
		if ( pos != string::npos ) {
			pos += strlen( _ALTRIMENTI );
			// elimina spazi e tab
			while ( isspace( prog[ pos ] ) ) pos++;
			if ( prog[ pos ] != '\n' )
				prog[ pos - 1 ] = '\n' ;
		}
	} while ( pos != string::npos ) ;


	// imposta le righe del programma
/*
	string::iterator it;
	size_t i = 0;
	for ( it = prog.begin() ; it < prog.end(); it++, i++ ) {

		size_t inizio_riga = i;
		// leva gli spazi sinistri
		while ( it < prog.end() && isspace(*it) ) {
			it++;
			i++;
			inizio_riga++;
		}

		while ( it < prog.end() && *it != '\n' ) {
			it++;
			i++;
		}

		if ( inizio_riga != i ) {
			string riga = prog.substr ( inizio_riga, i - inizio_riga );
			programma.Add( riga );
		}

	}
*/
    utils::split ( prog, "\n", programma );


	// dichiara la variabile RISULTATO utilizzata come ritorno dei programmi
	TID * id_node = new TID( _RISULTATO );
	id_node->inizializza (0) ;
	delete id_node;

    auto it = programma.begin();
	while (  it != programma.end() ) {

		std::string riga = utils::trim( *it ) ;
		if ( riga == "" ) {
            programma.erase( it );
            continue;
		}


		ComTag COMANDO  = GetTagCommand ( riga.c_str() ) ;

		switch (COMANDO) {

			case COM_LABEL : {	// non esegue
				// prima crea tutti i nodi label e li memorizza in una tabella hash
				// la sintassi é LABEL seguita dalla etichetta

				size_t pos = riga.find( _LABEL );
				pos += strlen( _LABEL );
				while ( isspace(riga[pos] ) ) pos++;
				string etichetta = riga.substr( pos );

				BASE_NODE *lab_node = new LABEL_NODE ;
				labels->Insert ( etichetta.c_str(), &lab_node, sizeof (BASE_NODE **) );
				break;

			}

			case COM_NUMERO : {

			// la sintassi é NUMERO seguito dall'identificatore
			// o identificatori separati da virgola

				size_t pos = riga.find( _NUMERO );
				pos += strlen( _NUMERO );

				while ( isspace( riga[pos] ) ) pos++;

				// tutto quello che viene dopo NUMERO
				riga = riga.substr( pos );

				/* separa gli elementi separati da virgola*/
				vector< string > v;
				utils::split ( riga, ",", v );

				/* per ogni elemento */
				for ( unsigned int i = 0; i < v.size(); i++ ) {

					string el = utils::trim( v[i] ) ;

					size_t p = el.find( ":=" );
					if ( p != string::npos ) {
						// c'è una assegnazione
						string ident = utils::trim(el.substr(0, p));
						string value = el.substr(p+2).c_str() ;
						float val = atof( value.c_str() );
						TID * id_node = new TID ( ident.c_str() );
						id_node->inizializza ( val ) ;
						delete id_node;
					}
					else {
						string ident = utils::trim( el.c_str() );
						TID * id_node = new TID ( el.c_str() );
						id_node->inizializza ( 0 ) ;
						delete id_node;
					}

				}


				break;

			}

			case COM_STRINGA : {

			// la sintassi é STRINGA seguito dall'identificatore
			// o identificatori separati da virgola

				size_t pos = riga.find( _STRINGA );
				pos += strlen( _STRINGA );

				while ( isspace( riga[pos] ) ) pos++;

				// tutto quello che viene dopo _STRINGA
				riga = riga.substr( pos );

				/* separa gli elementi separati da virgola*/
				vector< string > v;
				utils::split ( riga, ",", v );

				/* per ogni elemento */
				for ( unsigned int i = 0; i < v.size(); i++ ) {

					string el = utils::trim( v[i] ) ;

					size_t p = el.find( ":=" );
					if ( p != string::npos ) {
						// c'è una assegnazione
						string ident = utils::trim(el.substr(0, p));
						string val = el.substr(p+2);
						TSTRINGA * id_node = new TSTRINGA ( ident.c_str() );
						id_node->set ( val ) ;
						delete id_node;
					}
					else {
						string ident = utils::trim( el.c_str() );
						TSTRINGA * id_node = new TSTRINGA ( ident.c_str() );
						id_node->set ( "" ) ;
						delete id_node;
					}

				}


				break;

			}

			case COM_STAMPA : {


				}

			case COM_COMMAND : {


				}


				break;

				case COM_TABELLA : {

			// la sintassi é tabella seguito dall'identificatore
			// quindi tra parentesi tonde l'argomento

				size_t pos = riga.find( _TABELLA );
				pos += strlen( _TABELLA );

				while ( isspace( riga[pos] ) ) pos++;

				// tutto quello che viene dopo TABELLA
				riga = riga.substr( pos );

				size_t p1 = riga.find("(");
				size_t p2 = riga.find(")");

				string ident = utils::trim(riga.substr( 0, p1 ) );
				string arg = utils::trim(riga.substr( p1 + 1, p2 - p1 -1 ) );

				/* se ci sono le virgolette considera solo
				quello all'interno */
				if ( arg[0] == '"' ) {
					size_t pos = arg.find( "\"", 1 );
					if ( pos == string::npos )
						error ( "Nella inizializzazione della tabella mancano le virgolette di chiusura" );
					arg = utils::trim( arg.substr( 1, pos - 1 ) );
				}

				TTAB * id_node = new TTAB ( ident.c_str() );
				id_node->MemorizzaTabella ( arg.c_str() ) ;

				break;

			}


			case COM_MATRICE : {

			// la sintassi é tabella seguito dall'identificatore
			// quindi tra parentesi tonde l'argomento

				size_t pos = riga.find( _MATRICE );
				pos += strlen( _MATRICE );

				while ( isspace( riga[pos] ) ) pos++;

				// tutto quello che viene dopo TABELLA
				riga = riga.substr( pos );

				size_t p1 = riga.find("(");
				size_t p2 = riga.find(")");

				string ident = utils::trim(riga.substr( 0, p1 ) );
				string arg = utils::trim(riga.substr( p1 + 1, p2 - p1 -1 ) );

				/* se ci sono le virgolette considera solo
				quello all'interno */
				if ( arg[0] == '"' ) {
					size_t pos = arg.find( "\"", 1 );
					if ( pos == string::npos )
						error ( "Nella inizializzazione della tabella mancano le virgolette di chiusura" );
					arg = utils::trim( arg.substr( 1, pos - 1 ) );
				}

				TAB_MATRICE * id_node = new TAB_MATRICE ( ident.c_str() );
				id_node->MemorizzaTabella ( arg.c_str() ) ;

				break;

			}

			case COM_FUNZIONE : {
/*
			// la sintassi é TABELLA seguito dall'identificatore
			// quindi = e l'elenco di inizializzazioni

				char *ident = riga + strlen(_FUNZIONE);

				char * str = strchr(ident, '(') ;

				char * arg = NULL; // argomento della funzione
				if (str) {

					int pos = GetStrInBraket(str + 1) ;	// in utils
					if (pos > 0) {
						// mette fine stringa al carattere chiusa parentesi
						*(str + pos) = '\0';
						// l'argomento punta al carattere dopo aperta parentesi
						arg = str + 1 ;
						// chiude l'ident
						*str = '\0';
						// str punta al carattere successivo la parentesi
						str = str + pos + 1 ;
					}
				}

				str = strchr(str, '=') ;

				if (str) {
					// toglie gli spazi a destra
					str = lRTrim(str);	// in str.h
					*str++ = '\0';
				}
				else
					error("FUNZIONE non inizializzata");

            // inizializza l'argomento della funzione
				TID * id_node = new TID(arg);
				id_node->inizializza (0) ;
				delete id_node;

            // costruisce il nodo funzione
				TFUNCTION * id_funz = new TFUNCTION (DelBlk(ident));

				char * espr = str ;	// l'espressione della funzione

				id_funz->Make (arg, TESPRESSIONE().MakeTree(espr)) ;

				delete id_funz;
*/
				break;

			}
			case COM_ESPRESSIONE : break;
			case COM_VAI : break;
			case COM_SE : break;
			case COM_INIZIA : break;
			case COM_FINE : break;
			case COM_MENTRE : break;
			case COM_CICLO_PER : break;
			case COM_COMMENTO : break;

		} // end switch COMANDO

		++it;

	}   // while

	// ci deve essere sempre un nodo di inizio ed uno di fine
	BASE_NODE *start_node = new START_NODE ;
	BASE_NODE *end_node = new END_NODE ;

	BASE_NODE *current ;

	size_t n_righe = programma.size() ;

	char **src = new char * [n_righe];

	for ( unsigned int i = 0; i < n_righe; i++ ) {
		const char *riga = programma[i].c_str();
		src[i] = new char [ strlen(riga) + 1];
		strncpy( src[i], riga, strlen(riga) + 1 );
	}

	current = CompileProgrammRows	(start_node, end_node, src, n_righe ) ;

	for ( unsigned int i = 0; i < n_righe; i++ ) {
		delete []	src[i] ;
	}
	delete []	src ;


/*
	try {
		current = CompileProgrammRows	(start_node, end_node, prog, n_righe) ;
   }
   catch ( ... ) {
	}
*/

	if (current) current->next = end_node ;

	return start_node;

}

// registra e prepara un programma
void TINTERPRETE::RegisterProgramm ( const char *prog )
{

	// tabella hash per la memorizzazione delle etichette
	labels->Clear ();

	// memorizza la fine programma
	BASE_NODE *lab_node = new END_NODE ;
	labels->Insert ("FINE", &lab_node, sizeof (BASE_NODE **));

	// compila il programma

	compiled = CompileProgramm ( prog );
	isCompiled = true;

}

bool TINTERPRETE::existsProgramm ( ) const
{

	return isCompiled;

}

   // esegue il programma con n_ord numero di ordine
REAL TINTERPRETE::ExecProgramm ( ) const
{


	BASE_NODE *start_node = compiled;

	try {
		start_node->eval() ;
	}
	catch ( const std::string & err ) {
		throw ( err );
	}

	return Esp->Risolvi ( _RISULTATO ) ;

}

void TINTERPRETE::ClearProgramm( )
{

	if ( !isCompiled ) return;

	delete compiled ;

}

void TINTERPRETE::ClearVariables ()
{

	TIdent::ClearVariableTable ();

}

int TINTERPRETE::RicavaRigheSottoProgramma (char ** prog, int riga_inizio, int righe_totali)
{


	int n_row = 0;
	int inizi = 1;

	ComTag tag ;
	do {


		tag = GetTagCommand (prog[riga_inizio + n_row + 1]) ;


		if (tag == COM_INIZIA) inizi++;
		if (tag == COM_FINE) inizi--;


		n_row++;


		if (riga_inizio + n_row == righe_totali)
		error ("Manca blocco FINE");


	} while (inizi > 0 || tag != COM_FINE) ;


	return n_row - 1 ;  // toglie primo blocco INIZIO e ultimo blocco FINE

}

// ritorna il nodo terminale
BASE_NODE *TINTERPRETE::GetEndNode (BASE_NODE *node)
{

	BASE_NODE *n = node;
	while (n->next) n = n->next;

	return n;

}

// albero sintattico per la elaborazione di un programma interpretato

// inizializza il membro static finito
bool BASE_NODE::finito = true;

BASE_NODE::BASE_NODE () :
	visited(false),	// non e' stato visitato
	next(NULL)
{
}

BASE_NODE::~BASE_NODE ()
{
	visited = true ;
	if (next && !next->visited) delete next ;
}

LABEL_NODE::LABEL_NODE ()
{
}

LABEL_NODE::~LABEL_NODE ()
{
}

void LABEL_NODE::eval ()
{
	next->eval();
}

START_NODE::START_NODE () :
	BASE_NODE ()
{
}

START_NODE::~START_NODE ()
{
}

void START_NODE::eval ()
{
	finito = false ;
	if (next) next->eval() ;
}

END_NODE::END_NODE () :
	BASE_NODE()
{
}

END_NODE::~END_NODE ()
{
}

void END_NODE::eval ()
{
	finito = true;
}

ESP_NODE::ESP_NODE (TNODO *_radice) :
	BASE_NODE (),
   radice(_radice)
{
}

ESP_NODE::~ESP_NODE ()
{
    delete radice ;
}

void ESP_NODE::eval ()
{
    radice->eval() ;
    if (next) next->eval();
}

STAMPA_NODE::STAMPA_NODE ( char * str ) :
	BASE_NODE(),
	to_print(str)
{
}

STAMPA_NODE::~STAMPA_NODE ()
{
}

void STAMPA_NODE::stampa()
{

    const char * str = to_print.c_str();
	std::ostringstream s;

	while ( *str ) {

		switch ( *str ) {

			case '"' :

				str++;
				while ( *str && *str != '"' ) {
					s << *str;
					str++;
				}
				if ( *str != '"' ) {
					// solleva l'eccezione
				}
				s <<  ' ';
				str++;

			break;

			case ' ' :

				while ( *str && *str == ' ' ) str++;
				continue;

			break;

			default : {

				string word;
				while ( *str && *str != ' ' ) {
					word += *str;
					str++;
				}

				TIdent::Tipo tipo = TIdent::GetTipo ( word.c_str() );
				switch (tipo) {
					case TIdent::tpNUMERO :
						s << *(REAL *) TIdent::getData( word.c_str() ) << " ";
					break ;

					case TIdent::tpTABELLA :
					break ;
					case TIdent::tpSTRINGA :
						s << (char *) TIdent::getData( word.c_str() ) << " ";
					break ;
					case TIdent::tpFUNZIONE :
					break ;
					case TIdent::tpNONE :
						s << word << " ";
					break ;
					case TIdent::tpMATRICE :
					break ;
				}


			}	// case default

			break;

		}


	}

	// la variabile di ambiente utilizzata per la visualizzazione
	std::string amb;
	const char * c_amb = getenv( "INT_STAMPA" );
	if ( c_amb ) {
        amb = c_amb;
    }

	amb += s.str();
	setenv( "INT_STAMPA", amb.c_str(), 1 );

//	cout << s.str() << endl;

}

const char * STAMPA_NODE::get_class ()
{
        return _STAMPA ;
}

void STAMPA_NODE::eval ()
{

	stampa() ;

	if (!finito && next)
		next->eval();

}

COMMAND_NODE::COMMAND_NODE (  class TINTERPRETE * _interpreter, const std::string & _command ) :
	BASE_NODE(),
	interpreter ( _interpreter ),
	command ( _command )
{
}

COMMAND_NODE::~COMMAND_NODE ()
{
}


void COMMAND_NODE::eval ()
{

	if ( interpreter->$command ) {
		( (interpreter->$node_funz)->*(interpreter->$command) ) ( command.c_str() ) ;
	}
	else
		throw  ( string("Identificatore ") + command + " non dichiarato!!!" );

	if ( !finito && next )
		next->eval();

}

SE_NODE::SE_NODE (TNODO *_condizione) :
	BASE_NODE (),
   condizione(_condizione),
   cond_vero(NULL),
   cond_falso(NULL)
{
}

SE_NODE::~SE_NODE ()
{
    delete condizione ;

 // attenzione tiene conto che se cond_vero o cond_falso
 // sono NULL non controlla ...->visited
    if (cond_vero && !cond_vero->visited) delete cond_vero ;
    if (cond_falso && !cond_falso->visited) delete cond_falso ;
}

const char * SE_NODE::get_class ()
{
        return _SE ;
}

void SE_NODE::eval ()
{
	bool cond = condizione->eval();

	if (cond) {
		if (cond_vero) cond_vero->eval();
	}
	else
		if (cond_falso) cond_falso->eval();


	if (!finito && next)
		next->eval();
}

// implementazione nodo _MENTRE
MENTRE_NODE::MENTRE_NODE (TNODO *_condizione) :
	BASE_NODE (),
    condizione(_condizione),
    cond_vero(NULL)
{
}

MENTRE_NODE::~MENTRE_NODE ()
{
    delete condizione ;

 // attenzione tiene conto che se cond_vero o cond_falso
 // sono NULL non controlla ...->visited
    if (cond_vero && !cond_vero->visited) delete cond_vero ;
}

const char * MENTRE_NODE::get_class ()
{
	return _MENTRE ;
}

void MENTRE_NODE::eval ()
{

    while (condizione->eval())
		if (cond_vero) cond_vero->eval();

   	if (!finito && next)
    	next->eval();
}

// implementazione nodo _CICLO_PER

CICLO_PER_NODE::CICLO_PER_NODE (TNODO * _inizializzazione,
        TNODO * _condizione,
        TNODO * _incremento) :
	BASE_NODE (),
    inizializzazione(_inizializzazione),
    condizione (_condizione),
    incremento (_incremento)
{
}

CICLO_PER_NODE::~CICLO_PER_NODE ()
{

	delete inizializzazione;
	delete condizione;
	delete incremento;


	// attenzione tiene conto che se cond_vero o cond_falso
	// sono NULL non controlla ...->visited
	if (cond_vero && !cond_vero->visited) delete cond_vero ;

}

const char * CICLO_PER_NODE::get_class ()
{
	return _CICLO_PER ;
}

void CICLO_PER_NODE::eval ()
{

	inizializzazione->eval();


	while (condizione->eval()) {

		if (cond_vero) cond_vero->eval();
		incremento->eval();

	}

   	if (!finito && next)
    	next->eval();

}

