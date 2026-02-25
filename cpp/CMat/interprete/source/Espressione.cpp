//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>	// per isdigit ...

#ifndef _hash_
    #include <Hash.h>
#endif

#ifndef _espressione_
    #include <Espressione.hpp>
#endif  // #ifndef _espressione_

#ifndef _utils_
    #include <Utils.h>
#endif

/*
	23/03/01
   modificata l'assegnazione di TID aggiungendo la funzionalità
   id(numero) equivalente alla id := numero
*/

TESPRESSIONE::FUNZIONI_ESTERNE TESPRESSIONE::funzione_esterna ;

TESPRESSIONE::TESPRESSIONE () :
    lexeme( new char[81] ), esp(NULL), start_str(NULL), str(NULL), root(NULL), $funz(NULL), $command(NULL)
{
	SinTreeTbl = new HashClass;
}

TESPRESSIONE::TESPRESSIONE (const char * s) :
    TESPRESSIONE()
{

	isError = false;

	InitEspr (s) ;

}

 // inizializza la espressione
void TESPRESSIONE::InitEspr (const char *estern_espr)
{

	isError = false;
	esp = estern_espr ;	// memorizza la espressione da esaminare

	start_str = new char [strlen(esp) + 1] ;
// ne fa una copia per l'uso locale
	strcpy(start_str, esp) ;

	str = start_str ;

}

TESPRESSIONE::~TESPRESSIONE()
{
	if (lexeme) delete [] lexeme;
	if (start_str) delete start_str;
	delete SinTreeTbl ;
}

void TESPRESSIONE::error( const char *msg, const char *str )
{

	isError = true;

	string the_error = "Errore nella elaborazione della espressione " + string(str) + "\t - " +  msg;

	throw    ( the_error ) ;

}

void TESPRESSIONE::Free ()
{
    if (root) delete root;
    root = NULL;
}

int TESPRESSIONE::scan()
{

	while (*str != '\n' && *str != '\0') {

		char *ch = str++;	// preleva il carattere
					// corrente e
	                 		// passa al prossimo

		switch ( GetChar(ch) ) {

			case CH::COMMA:
				break;

			case CH::NUM:
				str += GetNum(lexeme, str);
				return TNODO::TP_NUM;

			case CH::OPERATORE :  {
					 // se l'operatore ha precedenza
				OP op = GetOp (ch) ;
						// quanti digit é l'operatore
				str += NumOpDigit (op) - 1;

				return op;
			}	// end case CH::OPERATORE

			case CH::DOPPIAPICINI:
				str += GetStrTraDoppiApicini(lexeme, str);
				return TNODO::TP_DOPPIAPICINI;

            case CH::$:
				// non considera gli spazi

				while ( *str == ' ' ) str++;
				if ( *str != '(' ) return TNODO::BAD;
				str++;
				str += GetStrInBraket(lexeme, str);

				return TNODO::$FIELD;

			case CH::COMMAND :

                str += 7;
 				str += GetNextOp(lexeme, str);

				if (*str != '(')
					return TNODO::BAD;
//				str += GetStrInBraket(lexeme, ++str) + 1;
// !!!!!! attenzione modificato il +1
				str++;
				str += GetStrInBraket(lexeme, str) ;

               return TNODO::COMMAND ;

            case CH::ALFA:

				str += GetNextOp(lexeme, str);
				FN f;	// numero ordine funzione
				KEY k;	// numero ordine parola chiave

				if ((f = IsFunction(lexeme)) != FN_NONE) {
					if (*str != '(')
						return TNODO::BAD;
//					str += GetStrInBraket(lexeme, ++str) + 1;
// !!!!!! attenzione modificato il +1
					str++;
					str += GetStrInBraket(lexeme, str) ;
					return TNODO::FUNZ + f;
				}
				else if ((k = IsKeyWord (lexeme)) != KEY::NONE)
					return TNODO::KEYWORD + static_cast< int > ( k );
				else if ( IsExtFunction (lexeme) ) {
					char * nome_funzione = new char [strlen(lexeme) + 1];
					strcpy(nome_funzione, lexeme);
					if (*str == '(') {
						str++;
						str += GetStrInBraket(lexeme, str);
					}

					int n = TNODO::FUNZIONE_ESTERNA +
						funzione_esterna.RicavaNumeroFunzione (nome_funzione);
					delete [] nome_funzione;
					return n;
				}
				else {

					TIdent::Tipo tipo = TIdent::GetTipo (lexeme) ;
					if ( tipo ==  TIdent::tpNONE ) {
						return TNODO::$FIELD;
					}
					else
						return TNODO::ID;
				}

			case CH::SPACE :
				break;
			case CH::BRAKET :
				break;
			case CH::NONE :
				break;

		}   // switch (GetChar(ch))


	}   // while (*str != '\n' && *str != '\0')

	return TNODO::EOLN;
}

CH TESPRESSIONE::GetChar( const char *ch ) const
{

	switch ( *ch ) {

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '.':
			return CH::NUM;

		case ' ':
			return CH::SPACE;

		case '+': case '-': case '&':  case ':':
		case '>': case '=': case '<':  case '!':
		case '*': case '/': case '^':  case '(':
                case ')':
			return CH::OPERATORE;

		case '"':
			return CH::DOPPIAPICINI;

		case '$':
			return CH::$;

		case ',':
			return CH::COMMA;

		case 'C':

            {

                const char * comm = "COMMAND";
                const char * c = ch;
                for ( size_t i = 0; i < 7; ++i ) {
                    if ( !c || *c != comm[i] )
                        return CH::ALFA;
                    c++;
                }

            }

            return CH::COMMAND;

		case 'a':

            {

                const char * comm = " and ";
                const char * c = ch - 1;
                for ( size_t i = 0; i < strlen( comm ); ++i ) {
                    if ( !c || *c != comm[i] )
                        return CH::ALFA;
                    c++;
                }

            }
			return CH::OPERATORE ;

		case 'n' :

            {

                const char * comm = " not ";
                const char * c = ch - 1;
                for ( size_t i = 0; i < strlen( comm ); ++i ) {
                    if ( !c || *c != comm[i] )
                        return CH::ALFA;
                    c++;
                }

            }
			return CH::OPERATORE ;

		case 'o':
            {

                const char * comm = " or ";
                const char * c = ch - 1;
                for ( size_t i = 0; i < strlen( comm ); ++i ) {
                    if ( !c || *c != comm[i] )
                        return CH::ALFA;
                    c++;
                }

            }
            return CH::OPERATORE ;

		default:
			return CH::ALFA;
	}

}

int TESPRESSIONE::GetNum(char *num, char *str)
{

	int i = 0;
    // riposiziona il puntatore al primo numero

	str--;
	   // se la stringa non é finita ed il carattere é un numero
	while (isdigit(*str) || (*str == '.') || (*str == ','))
		num[i++] = *str++;

	num[i] = '\0';
	return --i;
}

OP TESPRESSIONE::GetOp( const char *ch ) const
{
	switch (*ch) {

		case '+':
			return OP_PIU;
		case '-':
			return OP_MENO;
		case '*':
			return OP_PER;
		case '/':
			return OP_DIVISO;
		case '^':
			return OP_POTENZA;
		case '!':
			return OP_NOT;
		case '(':
			return OP_OPENBRAKET;
		case ')':
			return OP_CLOSEBRAKET;
		case '<':
			switch (*(ch + 1)) {
				case '>':
					return OP_DIVERSO;
				case '=' :
					return OP_MINOREUGUALE;
				default :
					return OP_MINORE;
			}
		case '>':
			switch (*(ch + 1)) {
				case '<':
					return OP_DIVERSO;
				case '=' :
					return OP_MAGGIOREUGUALE;
				default :
					return OP_MAGGIORE;
		}
		case '=':
			switch (*(ch + 1)) {
				case '<':
					return OP_MINOREUGUALE ;
				case '>' :
					return OP_MAGGIOREUGUALE;
				default :
					return OP_UGUALE;
			}
		case ':':
			switch (*(ch + 1)) {
				case '=':
					return OP_ASSEGNA ;
				default :
					return OP_NULL;
			}
		case 'a':
			if (*(ch + 1) == 'n' &&	*(ch + 2) == 'd')
				return OP_AND ;
			else return OP_NULL ;
		case 'n' :
			if (*(ch + 1) == 'o' && *(ch + 2) == 't')
				return OP_NOT ;
			else return OP_NULL ;
		case 'o':
			if (*(ch + 1) == 'r')
				return OP_OR ;
			else return OP_NULL ;
	}
	return OP_NULL;
}

int TESPRESSIONE::NumOpDigit(OP op) const
{
	switch (op) {
		case OP_ASSEGNA : case OP_MINOREUGUALE :
		case OP_MAGGIOREUGUALE : case OP_DIVERSO :
        case OP_OR :
			return 2 ;
		case OP_AND: case OP_NOT:
            return 3 ;
		default:
            return 1;
	}
}

int  TESPRESSIONE::GetNextComma(char *lexeme)
{

    int braket = 0;

    char *arg2 = lexeme;
   // parte dal successivo
   // se la stringa non é finita
   // o il carattere , e braket=0 (tante parentesi chiuse quante aperte)
    while ((*arg2 != 0) && (*arg2 != '\n') && !(!braket && (*arg2 == ','))) {
        if (*arg2 == '(')
            braket++;
        if (*arg2 == ')')
            braket--;
        arg2++;
   }

    if ( *arg2 == ',' ) {
        *arg2++ = '\0';
        return arg2 - lexeme;
    }
    else
        return -1;




}

int TESPRESSIONE::GetStrTraDoppiApicini(char *arg, char *str)
{

   int i = 0;

   // parte dal successivo
   // se la stringa non é finita
   // o il carattere apicino
   while ((*str != '\0') && (*str != '\n') && (*str != '"'))
      arg[i++] = *str++;

	if (*str != '"')
			error("apicini non chiusi", esp);

   arg[i] = '\0';

   return ++i;
}

int TESPRESSIONE::GetNextOp(char *s, char *str)
{

	int i = 0;

	// parte dal successivo
	// se la stringa non é finita
	// o il carattere non é un operatore

	// ritorna l'offset al precedente carattere
	str--;

	while ( (*str != '\0') && (*str != '\n')
		&& (GetChar(str) != CH::OPERATORE)
		&& (GetChar(str) != CH::BRAKET)
		&& (GetChar(str) != CH::COMMA)
		)

		s[i++] = *str++;

	s[i] = '\0';

	// leva gli spazi a destra

	int len = strlen( s );
	while (s[len-1] == ' ' && len >= 0)
		len--;

	s[len] = '\0';

	return --i;

}

int TESPRESSIONE::GetStrInBraket (char *s, char *str)
{
    int i      = 0,
        braket = 0;

   // parte dal successivo
   // se la stringa non é finita
   // o il carattere ) e braket=0 (tante parentesi chiuse quante aperte)
    while ((*str != 0) && (*str != '\n') && !(!braket && (*str == ')'))) {
        if (*str == '(')
            braket++;
        if (*str == ')')
            braket--;
      s[i++] = *str++;
   }

    if (*str != ')')
			error ("Era attesa una parentesi chiusa", esp);

   s[i]='\0';
   return ++i;
}

bool TESPRESSIONE::IsExtFunction (const char * nome_funzione) const
{
	return funzione_esterna.IsFunction(nome_funzione) ;
}

FN TESPRESSIONE::IsFunction ( const char *s ) const
{

	struct {
		const char *fz;
		FN    fn;
	} Fn[] = {
					{"log", 		FN_LOG},
					{"log10", 		FN_LOG10},
					{"ln", 			FN_LOG},
					{"cos", 		FN_COS},
					{"cosh", 		FN_COSH},
					{"sin", 		FN_SIN},
					{"tan", 		FN_TAN},
					{"atan", 		FN_TAN},
					{"exp", 		FN_EXP},
					{"dec", 		FN_DEC},
					{"int", 		FN_INT},
					{"pow", 		FN_POW},
					{"sqrt", 		FN_SQRT},
					{"rand", 		FN_RAND},
					{"fnz", 		FN_FNZ}
    		 };

	int sz = sizeof Fn / sizeof (Fn[0]);

	size_t n1 = strlen( s );

	for (int i = sz - 1; i >= 0; i--) {

		size_t n2 = strlen( Fn[i].fz );
		size_t n = ( n1 >= n2 ) ? n1 : n2 ;

		if (strncmp_no_case(s, Fn[i].fz, n) == 0)
			return Fn[i].fn;

	}

	return FN_NONE;

}

KEY TESPRESSIONE::IsKeyWord (const char *s) const
{
	struct {
		const char *fz;
		KEY  fn;
	} Fn[] = {
				{"PIG",   	KEY::PIG},
				{"VERO",   	KEY::VERO},
				{"FALSO", 	KEY::FALSO}
			};

	int sz = sizeof Fn / sizeof (Fn[0]);

	for (int i = sz - 1; i >= 0; i--)
		if (strcmp(s, Fn[i].fz) == 0)
			return Fn[i].fn;

	return KEY::NONE;

}

REAL TESPRESSIONE::Risolvi (const char *s)
{

	InitEspr (s) ;

	REAL rtn = MakeTree()->eval();

	Free ();

	return rtn;
}

TNODO *TESPRESSIONE::MakeTree()
{
	token = scan();
	return root = E();
}

void TESPRESSIONE::Print()
{
	root->print();
}

TNODO *TESPRESSIONE::E()
{
	TNODO *root = T();
	while (TRUE) {
		switch (token)
		{

			case OP_PIU:
				token = scan();
				root = new TPLUS (root, T());
				break;

			case OP_MENO:
				token = scan();
				root = new TMINUS (root, T());
				break;

			case OP_UGUALE:
				token = scan();
				root = new TEQUAL (root, T());
				break;

			case OP_DIVERSO:
				token = scan();
				root = new TNOTEQUAL (root, T());
				break;

			case OP_MINORE:
				token = scan();
				root = new TLESSTHAN (root, T());
				break;

			case OP_MAGGIORE:
				token = scan();
				root = new TGREATERTHAN (root, T());
				break;

			case OP_MAGGIOREUGUALE:
				token = scan();
				root = new TGREATEREQUALTHAN (root, T());
				break;

			case OP_MINOREUGUALE:
				token = scan();
				root = new TLESSEQUALTHAN (root, T());
				break;

			case OP_OR:
				token = scan();
				root = new TOROP (root, T());
				break;

			case OP_AND:
				token = scan();
				root = new TANDOP (root, T());
				break;

			default:
				return root;
		}
    }
//	return root;
}

TNODO *TESPRESSIONE::F()
{

	TNODO *root = NULL;

	switch (token)
	{

		case TNODO::ID: {

			TIdent::Tipo tipo = TIdent::GetTipo (lexeme) ;

			switch (tipo) {
				case TIdent::tpNUMERO :
					root = new TID (lexeme);
					break ;
				case TIdent::tpTABELLA :
					root = new TTAB (lexeme);
					break ;
				case TIdent::tpSTRINGA :
					root = new TSTRINGA (lexeme);
					break ;
				case TIdent::tpFUNZIONE :
					root = new TFUNZIONE (lexeme);
					break ;
				case TIdent::tpMATRICE :
					error ("Non ancora implementato", esp);
					break ;
				case TIdent::tpNONE :
					error ("Identificatore non dichiarato", esp);
					break ;
			}

			token = scan();

			return VerificaAssegnazione (root);

		}

		case TNODO::FIELD:
		case TNODO::TP_DOPPIAPICINI:

			root = new FIELD_NODE (lexeme);
			token  = scan();

			return VerificaAssegnazione (root);

		case TNODO::$FIELD:

			root = new $FIELD_NODE (this, lexeme);
			token  = scan();

			return VerificaAssegnazione (root);

		case TNODO::COMMAND: {

			TNODO *n = new FIELD_COMMAND (this, lexeme);
			token = scan();
			return n;

		}
		case TNODO::KEYWORD :
			break;
		case TNODO::KEYWORD + static_cast< int > ( KEY::PIG ):
			root = new TFLOAT ( 4* atan(1) );
			token  = scan();
			return root;

		case TNODO::TP_NUM:
			root = new TFLOAT (atof(lexeme));
			token  = scan();
			return root;

		case OP_OPENBRAKET:
			token = scan();
			root = E();

/*
			if (token != OP_CLOSEBRAKET) ;
      		error ("Era attesa una parentesi chiusa", esp);
*/

			token = scan();
			return root;

		case OP_MENO:
			token = scan();
			return new TUMINUS ( PRIOR1() );

		case OP_PIU:
			token = scan();
			return new TUPLUS ( PRIOR1() );

		case OP_NOT:
			token = scan();
			return new TNOT ( PRIOR1() );

		case TNODO::FUNZ + FN_LOG: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TLOG(n);

		}

		case TNODO::FUNZ + FN_LOG10 : {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TLOG10(n);

		}

		case TNODO::FUNZ + FN_COS: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TCOS(n);
		}

		case TNODO::FUNZ + FN_COSH: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TCOSH(n);
		}

		case TNODO::FUNZ + FN_SIN: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TSIN(n);
		}

		case TNODO::FUNZ + FN_TAN: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TTAN(n);
		}

		case TNODO::FUNZ + FN_SQRT: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TSQRT(n);
		}

		case TNODO::FUNZ + FN_EXP: {

			TESPRESSIONE esp(lexeme);
            esp.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp.MakeTree();

			token = scan();
			return new TEXP(n);
		}

		case TNODO::FUNZ + FN_DEC:
			break;
		case TNODO::FUNZ + FN_INT:
			break;
		case TNODO::FUNZ + FN_POW:	{

			// scompone i due operandi
			int pos = GetNextComma (lexeme);
			if ( pos == -1 )    // c'è solo un operando
				throw ( string("La funzione POW richiede due operandi") ) ;
			char *arg1 = lexeme;
			char *arg2 = lexeme + pos ;
//			if (arg2) *arg2++ = '\0';
			TESPRESSIONE esp1(arg1);
			esp1.set$ext_funz ( $node_funz, $funz);
			TNODO *n1 = esp1.MakeTree();

			TESPRESSIONE esp2(arg2);
			esp2.set$ext_funz ( $node_funz, $funz);
			TNODO *n2 = esp2.MakeTree();

			token = scan();
			return new TPOW(n1, n2);
		}

		case TNODO::FUNZ + FN_RAND:	{

			// scompone i due operandi
			int pos = GetNextComma (lexeme);
			if ( pos == -1 )    // c'è solo un operando
				throw ( string("La funzione rand richiede due operandi") ) ;
			char *arg1 = lexeme;
			char *arg2 = lexeme + pos ;
//			if (arg2) *arg2++ = '\0';
			TESPRESSIONE esp1(arg1);
			esp1.set$ext_funz ( $node_funz, $funz);
			TNODO *n1 = esp1.MakeTree();

			TESPRESSIONE esp2(arg2);
			esp2.set$ext_funz ( $node_funz, $funz);
			TNODO *n2 = esp2.MakeTree();

			token = scan();
			return new TRAND(n1, n2);
		}

		case TNODO::FUNZ + FN_FNZ : {

			TNODO *n = new T_EXT_FUNZ (lexeme);
			token = scan();
			return n;
		}

		case TNODO::EOLN :
                error ("Errore di sintassi", esp);

		default:

			int n_funzione = token - TNODO::FUNZIONE_ESTERNA ;

			if (n_funzione >= 0) {

				EXT_FUNZ funz = funzione_esterna.GetFunctionDaNumero (n_funzione) ;

/*
				if ((int) funz > 16) {
					token = scan () ;
					TNODO *n = new T_FUNZIONE_ESTERNA (funz, lexeme);
					return n ;
				}
				else
					error ("Errore di sintassi", "indefinito");
*/
					token = scan () ;
					TNODO *n = new T_FUNZIONE_ESTERNA (funz, lexeme);
					return n ;

			}
			else
				error ("Errore di sintassi", esp);
	}

	return root;

}

TNODO *TESPRESSIONE::T()
{
	TNODO *root = PRIOR1();
	while (TRUE)
	{
		switch (token)
		{
			case OP_PER:
				token = scan();
				root = new TTIMES (root, PRIOR1());
				break;

			case OP_DIVISO:
				token = scan();
				root = new TDIVISO (root, PRIOR1());
				break;
			default:
				return root;
		}
    }

}

TNODO *TESPRESSIONE::PRIOR1()
{
	// massima priorità di esecuzione
	// esegue PRIOR1 --> T --> E
	TNODO *root = F();
	while (TRUE)
	{
		switch (token)
		{
			case OP_POTENZA:
				token = scan();
				root = new POTENZA (root, F());
				break;
			default:
				return root;
		}
    }

}

// prepara la espressione creando l'albero sintattico e memorizzandolo
// nella tabella
TNODO *TESPRESSIONE::MakeTree (const char *estern_espr)
{

	InitEspr (estern_espr) ;

	return MakeTree () ;
}

void TESPRESSIONE::Prepare (const char *espr)
{

	isError = false;
	TNODO *radice = MakeTree (espr) ;

	// memorizza l'indirizzo di radice
	if (!isError) {

		char *key = new char[strlen(espr) + 1];
		strcpy(key, espr);

		SinTreeTbl->Insert(key, &radice, sizeof(TNODO **));

		delete [] key;

	}
}

// mette in esecuzione la espressione preparata con Prepare
REAL TESPRESSIONE::Execute (const char *espr)
{
//    TNODO *radice = *(TNODO **) SinTreeTbl->GetData (espr);
	void *data = SinTreeTbl->GetData (espr);
	if (data) {
		TNODO *radice = *(TNODO **) data ;
		return radice->eval() ;
	}
	else return 0;
}

// disalloca l'albero sintattico
void TESPRESSIONE::Clear ()
{
    HashIterator iter(SinTreeTbl) ;

    iter.First();
    while (!iter.Eof()) {
        TNODO *radice = *(TNODO **) iter.GetData ();
        delete radice;
        iter.Next();
    }
    SinTreeTbl->Clear();
}

void TESPRESSIONE::DefinisciFunzioneEsterna (const char * nome_funzione,
															EXT_FUNZ funzione)
{
	funzione_esterna.Definisci(nome_funzione, funzione) ;
}

TNODO * TESPRESSIONE::VerificaAssegnazione (TNODO * root)
{

	// se è operatore di assegnazione
	if (token == OP_ASSEGNA) {
		token = scan();
		root = new TASSIGN ((TVAR*) root, E());
	}
	// l'assegnazione viene effettuata anche facendo seguire
	// alla variabile l'operando racchiuso tra parentesi
	else if (token == OP_OPENBRAKET) {

		// str è posizionata al carattere successivo
		// alla parantesi aperta
		str += GetStrInBraket(lexeme, str);

		// verifica se ci sono 1 o 2 argomenti (in questo caso sono separati da una virgola)
		char *arg1 = lexeme;
		char *arg2 = strstr(  lexeme, "," );
		if ( arg2 ) {
			*arg2 = '\0';
			arg2++;

			TESPRESSIONE esp1 ( arg1 );
			TESPRESSIONE esp2 ( arg2 );

            esp1.set$ext_funz ( $node_funz, $funz);
            esp2.set$ext_funz ( $node_funz, $funz);

			TNODO *n1 = esp1.MakeTree();
			TNODO *n2 = esp2.MakeTree();

			// assegna l'espressione n
			root = new ASSIGN2 ((TAB_MATRICE*) root, n1, n2 );

		}
		else {

			TESPRESSIONE esp1(lexeme);
            esp1.set$ext_funz ( $node_funz, $funz);

			TNODO *n = esp1.MakeTree();

			// assegna l'espressione n
			root = new TASSIGN ((TVAR*) root, n);

		}

		// va all'operatore successivo
		token = scan();


	}

   return root ;
}

//	implementazione della classe FUNZIONI_ESTERNE

TESPRESSIONE::FUNZIONI_ESTERNE::FUNZIONI_ESTERNE ()
{
		n_funzione = 0 ;

		funzione_esterna = new HashClass ;
		ordine_funzione = new HashClass ;
		numero_funzione = new HashClass ;
}

TESPRESSIONE::FUNZIONI_ESTERNE::~FUNZIONI_ESTERNE ()
{
		delete funzione_esterna  ;
		delete ordine_funzione  ;
		delete numero_funzione  ;
}

bool TESPRESSIONE::FUNZIONI_ESTERNE::Definisci (const char * nome_funzione,
															EXT_FUNZ funzione)
{
	if (funzione_esterna->Insert
    			(nome_funzione,(void *) (& funzione), sizeof ( EXT_FUNZ))) {

		char str [4];
		sprintf(str, "%d", n_funzione);

		ordine_funzione->Insert
        		(str, (void *) nome_funzione, strlen (nome_funzione) + 1);
		numero_funzione->Insert
			(nome_funzione, (void *) (& n_funzione), sizeof (int));

		n_funzione++;

		return true;
	}
	else return false;
}

bool TESPRESSIONE::
FUNZIONI_ESTERNE::IsFunction (const char * nome_funzione ) const
{
	return funzione_esterna->Exist (nome_funzione) ;
}

EXT_FUNZ TESPRESSIONE::
FUNZIONI_ESTERNE::GetFunction (const char * nome_funzione ) const
{
	return * (EXT_FUNZ *) funzione_esterna->GetData(nome_funzione) ;
}

EXT_FUNZ TESPRESSIONE::
FUNZIONI_ESTERNE::GetFunctionDaNumero (int numero) const
{

	char str [4];
	sprintf(str, "%d", numero);

	char *nome_funzione = (char *) ordine_funzione->GetData (str) ;

	if (!nome_funzione) return NULL;
	else return * (EXT_FUNZ *) funzione_esterna->GetData(nome_funzione) ;

}

int TESPRESSIONE::
FUNZIONI_ESTERNE::RicavaNumeroFunzione (const char * nome_funzione ) const
{
		return * (int *) numero_funzione->GetData (nome_funzione);
}
