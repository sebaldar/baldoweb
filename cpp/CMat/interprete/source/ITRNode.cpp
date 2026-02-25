#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ctype.h>	// per isdigit ...

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#ifndef _hash_
    #include <Hash.h>
#endif

#ifndef _TNODO_
	#include <ITRNode.hpp>
#endif

#ifndef _espressione_
    #include <Espressione.hpp>
#endif  // #ifndef _espressione_



// membri statici di TIdent
HashClass	TIdent::TipoIdentificatore ;
HashClass *TIdent::VariableTable = NULL;

EXT_VALUE ext_value = NULL;
EXT_FUNZ ext_funz = NULL;

void setExtValue ( EXT_VALUE f ) {
	ext_value = f;
}

// implementazione classe TNODE
TNODO::TNODO()
{
}

TNODO::~TNODO()
{
}

void TNODO::print ()
{
}

// implementazione classe TUNOP
TUNOP::TUNOP (TNODO *o)
{
	operand = o;
}

TUNOP::~TUNOP()
{
	delete operand;
}

void TUNOP::print_op ()
{
}

void TUNOP::print ()
{
	print_op();
	operand->print();
}

// implementazione classe TBINOP
TBINOP::TBINOP (TNODO *l, TNODO *r) :
	left(l), right(r)
{
}

TBINOP::~TBINOP()
{
	delete left;
	delete right;
}

void TBINOP::print ()
{
	printf("%s","(");
	left->print();
	print_op();
	right->print();
	printf("%s",")");
}


// implementazione classe TVALUE
TVALUE::TVALUE (REAL val) :
    value(val)
{
}

TVALUE::~TVALUE()
{
}

REAL TVALUE::eval ()
{
   return value;
}

void TVALUE::print ()
{
    printf("%f", value);
}

// implementazione classe TEQUAL
TEQUAL::TEQUAL (TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}
REAL TEQUAL::eval ()
{
    return left->eval() == right->eval();
}

void TEQUAL::print()
{
	printf(" = ");
}

void TEQUAL::print_op()
{
	printf(" = ");
}

// implementazione classe TEQUAL
TNOTEQUAL::TNOTEQUAL (TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}
REAL TNOTEQUAL::eval ()
{
    return left->eval() != right->eval();
}

void TNOTEQUAL::print()
{
	printf(" <> ");
}

void TNOTEQUAL::print_op()
{
	printf(" <> ");
}

// implementazione classe TLESSTHAN
TLESSTHAN::TLESSTHAN(TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TLESSTHAN::eval ()
{
	return left->eval() < right->eval();
}

void TLESSTHAN::print_op()
{
	printf(" < ");
}

// implementazione classe TGREATERTHAN
TGREATERTHAN::TGREATERTHAN(TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TGREATERTHAN::eval ()
{
	return left->eval() > right->eval();
}

void TGREATERTHAN::print_op()
{
	printf(" > ");
}

// implementazione classe TLESSEQUALTHAN
TLESSEQUALTHAN::TLESSEQUALTHAN (TNODO *l, TNODO *r):
TBINOP(l, r)
{
}

REAL TLESSEQUALTHAN::eval ()
{
	return left->eval() <= right->eval();
}

void TLESSEQUALTHAN::print_op()
{
	printf(" <= ");
}

// implementazione classe TGreaterEqualThan
TGREATEREQUALTHAN::TGREATEREQUALTHAN(TNODO *l, TNODO *r) :
TBINOP(l, r)
{
}

REAL TGREATEREQUALTHAN::eval ()
{
	return left->eval() >= right->eval();
}

void TGREATEREQUALTHAN::print_op()
{
	printf(" >= ");
}

// implementazione classe TOROP
TOROP::TOROP(TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TOROP::eval ()
{
	return left->eval() || right->eval();
}

void TOROP::print_op()
{
	printf(" or ");
}

// implementazione classe TANDOP
TANDOP::TANDOP(TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TANDOP::eval ()
{
	return left->eval() && right->eval();
}

void TANDOP::print_op()
{
	printf(" and ");
}

// implementazione classe TMINUS
TMINUS::TMINUS(TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TMINUS::eval ()
{
	return left->eval() - right->eval();
}

void TMINUS::print_op()
{
	printf(" - ");
}

// implementazione classe TUMINUS
TUMINUS::TUMINUS(TNODO *o) :
	TUNOP(o)
{
}

REAL TUMINUS::eval ()
{
	return -operand->eval();
}

void TUMINUS::print_op()
{
	printf("-");
}

// implementazione classe TUPLUS
TUPLUS::TUPLUS(TNODO *o) :
	TUNOP(o)
{
}

REAL TUPLUS::eval ()
{
	return operand->eval();
}

void TUPLUS::print_op()
{
	printf("-");
}

// implementazione classe TNOT
TNOT::TNOT(TNODO *o) :
TUNOP(o)
{
}

REAL TNOT::eval ()
{
	return !operand->eval();
}

void TNOT::print_op()
{
	printf(" not ");
}

// implementazione classe TTMES
TTIMES::TTIMES (TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TTIMES::eval ()
{
	return left->eval() * right->eval();
}

void TTIMES::print_op()
{
	printf("*");
}

// implementazione classe POW
POTENZA::POTENZA (TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL POTENZA::eval ()
{

	REAL val1 = left->eval() ;
	REAL val2 = right->eval() ;

	/*
	 * se l'esponente non è un intero la base deve essere maggiore di 0
	*/

	if ( ( floor(val2) != ceil(val2) ) && val1 < 0 ) {
		ostringstream oss ( "" );
		oss << "Calcolo di funzione esponenziale con esponente non intero " << val2 << " e base negativo " << val1 << endl;
		oss << "Provare a modificare le variabili interne."  << endl ;
		throw( oss.str() );
	}

	return pow( val1, val2 );

}

void POTENZA::print_op()
{
	printf("^");
}

// implementazione classe TDIVISO
TDIVISO::TDIVISO (TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TDIVISO::eval ()
{
	REAL divisore = right->eval();
	if ( divisore != 0 )
		return left->eval() / right->eval();
	else {
		ostringstream oss ( "" );
		oss << "Calcolo di una divisione con divisore zero." << endl;
		throw( oss.str() );
	}
}

void TDIVISO::print_op()
{
	printf("/");
}

 // implementazione classe TFLOAT
TFLOAT::TFLOAT (REAL f) :
    TVALUE (f)
{
}

TFLOAT::~TFLOAT ()
{
}

// implementazione classe TKEY
TKEY::TKEY ( const char *s ) :
    TNODO()
{
	value = s;
}

TKEY::~TKEY ()
{
}

REAL TKEY::eval ()
{
    return 0;
}

void TKEY::print ()
{
}
// implementazione classe TVAR
TVAR::TVAR ()
{
}

TVAR::~TVAR()
{
}

// implementazione classe TIdent
TIdent::TIdent ( const char *key ) :
    TVAR(),
    id(key)
{
// lo inizializza solo la prima volta
    if (!VariableTable) VariableTable = new HashClass ;
// la memoria non e' rilasciata in TVAR per conservare il
// valore delle variabili
// viene rilasciata da WEP
}

bool TIdent::VerificaEsistenza ()
{

// prima controlla se la variabile è stata dichiarata
	if (!VariableTable->Exist ( id.c_str() )) {

		std::string error = "L'identificatore " + id + " non risulta dichiarato!!";

		throw    ( error ) ;

	}
	return true;

}

void TIdent::ClearVariableTable ()
{

	HashIterator iter (&TipoIdentificatore) ;

	iter.First();
	while (!iter.Eof()) {
		Tipo tipo = * (TIdent::Tipo *) iter.GetData() ;
		char * key = iter.GetKey () ;

		switch (tipo) {
			case TIdent::tpTABELLA :
				TTAB::ReleaseVariableMem(VariableTable->GetData(key));
				break;
			case TIdent::tpFUNZIONE :
				TFUNZIONE::ReleaseVariableMem(VariableTable->GetData(key));
				break;
			default :
				break;
		}
		iter.Next () ;
	}

	iter.Clear () ; // pulisce la tabella tipoIdentificatore

	if (VariableTable) delete VariableTable ;
	VariableTable = NULL ;

}

TIdent::Tipo TIdent::GetTipo (const char * nome_id)
{
	if (TipoIdentificatore.Exist(nome_id))
		return  * (TIdent::Tipo *) TipoIdentificatore.GetData(nome_id);
   else
   	return tpNONE;
}

// implementazione classe TID
TID::TID ( const char *key ) :
    TIdent (key)
{
	Tipo tipo = tpNUMERO ;
	TipoIdentificatore.Insert(id.c_str(), &tipo, sizeof(TIdent::Tipo));
}

void TID::print()
{
	cout << id;
}

REAL TID::eval()
{

	REAL *a = (REAL *) VariableTable->GetData(id.c_str());
	// se non esiste costruiscili

	if (!a)	{

		std::string error = "Identificatore " + id + " non dichiarato!";
/*
		throw    ( error ) ;
*/
    REAL val = 0;
    a = (REAL *) VariableTable->Insert(id.c_str(), &val, sizeof(REAL));
	}

	return *a;
}

REAL TID::set (REAL s)
{

	VerificaEsistenza () ;

	// memorizza i valore
	VariableTable->Insert(id.c_str(), &s, sizeof(REAL));
	return s;

}

REAL TID::inizializza ( REAL s )
{

// inizializza una variabile non dichiarata

	VariableTable->Insert( id.c_str(), &s, sizeof(REAL) );
	return s;

}

// implementazione classe FIELD_NODE
FIELD_NODE::FIELD_NODE ( char  * name )
{
	if ( ext_value )
		value = &(*ext_value) (name);
	else
		throw  ( std::string("Identificatore ") +  name + " non dichiarato!!!" );

}

FIELD_NODE::FIELD_NODE (  )
{
}

REAL FIELD_NODE::eval()
{
	return *value;
}

void FIELD_NODE::print()
{
	printf("FIELD");
}

REAL FIELD_NODE::set (REAL f)
{
    return *value = f;
}

// implementazione classe $FIELD_NODE
$FIELD_NODE::$FIELD_NODE ( TESPRESSIONE *_esp, char  * name ) :
	FIELD_NODE(), esp(_esp)
{
	if ( esp->$funz ) {
		value =  &( (esp->$node_funz)->*(esp->$funz) ) (name) ;
	}
	else if ( ext_value ) {
		value = &(*ext_value) (name);
	}
	else {
		throw  ( string("Identificatore ") +  name + " non dichiarato!!!" );
	}

}

$FIELD_NODE::$FIELD_NODE ( TESPRESSIONE *_esp ) :
	FIELD_NODE(), esp(_esp)
{
}


void $FIELD_NODE::print()
{
	printf("$FIELD");
}

// implementazione classe $FIELD_NODE
FIELD_COMMAND::FIELD_COMMAND ( TESPRESSIONE *esp, char  * comando ) :
	$FIELD_NODE(esp), command(comando)
{

}


void FIELD_COMMAND::print()
{
	printf("$COMMAND");
}

REAL FIELD_COMMAND::eval()
{

	if ( esp->$command ) {
		( (esp->$node_funz)->*(esp->$command) ) ( command.c_str() ) ;
	}
	else
		throw  ( string("Identificatore ") + command + " non dichiarato!!!" );

	return (0);
}


// implementazione della classe TPLUS
TPLUS::TPLUS (TNODO *l, TNODO *r) :
	TBINOP(l, r)
{
}

REAL TPLUS::eval ()
{
	return left->eval() + right->eval();
}

void TPLUS::print_op()
{
	printf(" + ");
}

// implementazione classe TASSIGN
TASSIGN::TASSIGN (TVAR *t, TNODO *e) :
	TBINOP(t, e)
{
}

REAL TASSIGN::eval()
{
 	return ((TVAR*) left)->set(right->eval());
}

void TASSIGN::print_op()
{
	printf(" := ");
}

// implementazione classe TASSIGN
ASSIGN2::ASSIGN2 ( TAB_MATRICE *t, TNODO *a1, TNODO *a2 ) :
	var(t), arg1(a1),arg2(a2)
{
}

REAL ASSIGN2::eval()
{
 	return (  var->set2( arg1->eval(), arg2->eval() ) );
}

void ASSIGN2::print_op()
{
	printf(" := ");
}

// implementazione classe TFUNZ
TFUNZ::TFUNZ (TNODO *node) :
	arg(node)
{
}

TFUNZ::~TFUNZ()
{
	delete arg;
}

void TFUNZ::print ()
{
	print_op();
	arg->print();
}

// implementazione classe TLOG
// logaritmo naturale
TLOG::TLOG(TNODO *arg) :
	TFUNZ(arg)
{
}

REAL TLOG::eval()
{

	REAL val = arg->eval();
	if ( val > 0 ) {
		return log( val );
	}
	else if ( val <= 0 ) {
		ostringstream oss ( "" );
		oss << "Calcolo del logaritmo del numero negativo " << val << "." << endl;

		throw( oss.str() );
	}

	return (-1);    // questo punto non viene mai raggiunto

}

void TLOG::print_op()
{
	printf(" log");
}

// implementazione classe TLOG10
// logaritmo in base 10
TLOG10::TLOG10(TNODO *arg) :
	TFUNZ(arg)
{
}

REAL TLOG10::eval()
{
	REAL val = arg->eval();
	if ( val <= 0 ) {
		ostringstream oss ( "" );
		oss << "Calcolo del logaritmo di numero negativo " << val << "." << endl;
		throw( oss.str() );
	}
	return log10( val );
}

void TLOG10::print_op()
{
	printf(" log10");
}

// implementazione  classe TCOS
TCOS::TCOS (TNODO *arg) :
	TFUNZ(arg)
{
}

REAL TCOS::eval()
{
	return cos(arg->eval());
}

void TCOS::print_op()
{
	printf(" cos");
}

// implementazione  classe TCOS
TCOSH::TCOSH (TNODO *arg) :
	TFUNZ(arg)
{
}

REAL TCOSH::eval()
{
	return cosh(arg->eval());
}

void TCOSH::print_op()
{
	printf(" cos");
}

// implementazione classe TSIN
TSIN::TSIN(TNODO *arg) :
	TFUNZ(arg)
{
}

REAL TSIN::eval()
{
	return sin(arg->eval());
}

void TSIN::print_op()
{
	printf(" log");
}

// implementazione classe TFUNZ
TTAN::TTAN (TNODO *arg) :
TFUNZ(arg)
{
}

REAL TTAN::eval()
{
	return atan( arg->eval() );
}

void TTAN::print_op()
{
	printf(" tan");
}

// implementazione classe TSQRT
TSQRT::TSQRT (TNODO *arg) :
TFUNZ(arg)
{
}

REAL TSQRT::eval()
{

	REAL val = arg->eval();

	if ( val >= 0 )
		return sqrt( val );
	else {
		ostringstream oss ( "" );
		oss << "Calcolo della radice quadrata di un  del numero negativo " << val << "." << endl;

		throw( oss.str() );

	}
}

void TSQRT::print_op()
{
	printf(" tan");
}

// implementazione classe TEXP
TEXP::TEXP (TNODO *arg) :
	TFUNZ(arg)
{
}

REAL TEXP::eval()
{
	return exp( arg->eval() );
}

void TEXP::print_op()
{
	printf(" exp");
}

// implementazione classe TPOW
TPOW::TPOW (TNODO *_arg1, TNODO *_arg2) :
	arg1(_arg1), arg2(_arg2)
{
}

TPOW::~TPOW()
{
	delete arg1;
	delete arg2;
}

REAL TPOW::eval()
{

	REAL val1 = arg1->eval() ;
	REAL val2 = arg2->eval() ;

	/*
	 * se l'esponente non è un intero la base deve essere maggiore di 0
	*/

	if ( ( floor(val2) != ceil(val2) ) && val1 < 0 ) {
		ostringstream oss ( "" );
		oss << "Calcolo di funzione esponenziale con esponente non intero " << val2 << " e base negativo " << val1 << "." << endl;
		throw( oss.str() );
	}

	return pow( val1, val2 );

}

void TPOW::print()
{
	printf(" pow(");
   arg1->print();
	printf(", ");
   arg2->print();
	printf(")");

}

// implementazione classe TPOW
// ritorna un numero casuale tra arg1 e arg2-1
TRAND::TRAND (TNODO *_arg1, TNODO *_arg2) :
	arg1(_arg1), arg2(_arg2)
{
}

TRAND::~TRAND()
{
	delete arg1;
	delete arg2;
}

REAL TRAND::eval()
{

	/* initialize random seed: */
	srand ( time(NULL) );

	int val1 = floor ( arg1->eval() ) ;
	int val2 = floor ( arg2->eval() );


	return rand() % val2 + val1;

}

void TRAND::print()
{
	printf(" rand(");
   arg1->print();
	printf(", ");
   arg2->print();
	printf(")");

}

T_EXT_FUNZ::T_EXT_FUNZ (const char *_arg) :
    arg(NULL)
{
    if (_arg) {
        arg = new char [strlen(_arg) + 1];
        strcpy(arg, _arg) ;
    }
}

T_EXT_FUNZ::~T_EXT_FUNZ ()
{
    if (arg) delete [] arg;
}

REAL T_EXT_FUNZ::eval()
{
    return (*ext_funz) (arg);
}

void T_EXT_FUNZ::print_op()
{
}

T_FUNZIONE_ESTERNA::T_FUNZIONE_ESTERNA (EXT_FUNZ _funz, const char *_arg) :
    arg(NULL)
{
	funz = _funz ;

	if (_arg) {
		arg = new char [strlen(_arg) + 1];
		strcpy(arg, _arg) ;
	}
}

T_FUNZIONE_ESTERNA::~T_FUNZIONE_ESTERNA ()
{
	if (arg) delete [] arg;
}

REAL T_FUNZIONE_ESTERNA::eval()
{
std::cout << "***" << std::endl;
    return (* funz) (arg);
}

void T_FUNZIONE_ESTERNA::print_op()
{
}

TSTRINGA::TSTRINGA ( const char *key ) :
	TIdent (key)
{

	Tipo tipo = tpSTRINGA ;
	TipoIdentificatore.Insert( id.c_str(), &tipo, sizeof(TIdent::Tipo) );

}

TSTRINGA::~TSTRINGA ()
{
}

void TSTRINGA::set ( const string & value  )
{
	// memorizza i valore

    VariableTable->Insert(id.c_str(), value.c_str() , value.size()+1);
}

REAL TSTRINGA::eval()
{

	return 0 ;

}

// implementazione della classe TTAB
// valutazioni di tabelle di dati

TTAB::TTAB ( const char *key ) :
	TIdent (key)
{

	Tipo tipo = tpTABELLA ;
	TipoIdentificatore.Insert( id.c_str(), &tipo, sizeof(TIdent::Tipo) );

}

TTAB::~TTAB ()
{
}

void TTAB::MemorizzaTabella ( const char * dati_xml ) // memorizza gli elementi della tabella
{

	XMLDocument doc ;

	doc.parse ( dati_xml ) ;

	XMLElement *root = doc.rootElement();

	try {
		MemorizzaTabella ( root ); // memorizza gli elementi della tabella
	} catch ( const std::string & e ) {
		throw( e );
	}

}

void TTAB::MemorizzaTabella ( class XMLElement * root ) // memorizza gli elementi della tabella
{

	// verifica se esiste già una struttura dati
	if ( VariableTable->Exist (id.c_str()) )	{

		TTAB::TMem * mem = * (TTAB::TMem **) VariableTable->GetData( id.c_str() ) ;

		if ( mem ) {

			// se si aggiunge i dati
			LCurva * curva = ( LCurva * ) mem->curva ;
			try {
				curva->caricaTabella( root );
			} catch ( const std::string &e ) {
				throw(e);
			}

		}	// if data

	}	// if (!VariableTable->Exist (id))
	else {

		// crea una Curva
		Curva * curva;
		try {
			curva = new LCurva( root ) ;
		} catch ( const std::string &e ) {
			throw(e);
		}

		// una struttura nuova per memorizzare i dati
		TTAB::TMem * mem = new TTAB::TMem ;

		mem->curva = curva;
		mem->value = 0;

		VariableTable->Insert(id.c_str(), &mem, sizeof( TTAB::TMem **));

	}


}

REAL TTAB::set (REAL s)
{

	// prima controlla se la variabile è stata dichiarata
	VerificaEsistenza () ;

	// la struttura per memorizzare i dati
	TTAB::TMem * mem = * (TMem **) VariableTable->GetData(id.c_str());

	mem->value = mem->curva->value (s) ;

	VariableTable->Insert(id.c_str(), &mem, sizeof( TTAB::TMem **));
	return mem->value ;

}

REAL TTAB::eval()
{

	// la struttura per memorizzare i dati
	TTAB::TMem * mem = * (TTAB::TMem **) VariableTable->GetData(id.c_str());

	return mem->value ;

}

void TTAB::ReleaseVariableMem (void * data)
{

	if (data) {
		TTAB::TMem * mem = * reinterpret_cast<TTAB::TMem **> (data) ;
		delete mem ;
	}

}

// implementazione della classe TAB_MATRICE
// valutazioni di una TAB_MATRICE di dati

TAB_MATRICE::TAB_MATRICE ( const char *key ) :
	TIdent (key)
{

	Tipo tipo = tpMATRICE ;
	TipoIdentificatore.Insert( id.c_str(), &tipo, sizeof(TIdent::Tipo) );

}

TAB_MATRICE::~TAB_MATRICE ()
{
}

void TAB_MATRICE::MemorizzaTabella ( const char * dati_xml ) // memorizza gli elementi della tabella
{

	// verifica se esiste già una struttura dati
	if ( VariableTable->Exist (id.c_str()) )	{

		void * data = VariableTable->GetData( id.c_str() ) ;

		if (data) {

			TAB_MATRICE::TMem * mem = * (TAB_MATRICE::TMem **) data ;

			// se si la distrugge
			if (mem) delete mem ;

		}	// if data

	}	// if (!VariableTable->Exist (id))


	// crea una matrice
	Matrice * matrice = new Matrice ( dati_xml ) ;

	// una struttura nuova per memorizzare i dati
	TAB_MATRICE::TMem * mem = new TAB_MATRICE::TMem ;

	mem->matrice = matrice;
	mem->value = 0;

	VariableTable->Insert( id.c_str(), &mem, sizeof( TAB_MATRICE::TMem **));

}

void TAB_MATRICE::MemorizzaTabella ( class XMLElement * root ) // memorizza gli elementi della tabella
{

	// verifica se esiste già una struttura dati
	if ( VariableTable->Exist (id.c_str()) )	{

		void * data = VariableTable->GetData( id.c_str() ) ;

		if (data) {

			TAB_MATRICE::TMem * mem = * (TAB_MATRICE::TMem **) data ;

			// se si la distrugge
			if (mem) delete mem ;

		}	// if data

	}	// if (!VariableTable->Exist (id))

	// crea una
	Matrice * matrice = new Matrice ( root ) ;

	// una struttura nuova per memorizzare i dati
	TAB_MATRICE::TMem * mem = new TAB_MATRICE::TMem ;

	mem->matrice = matrice;
	mem->value = 0;

	VariableTable->Insert(id.c_str(), &mem, sizeof( TAB_MATRICE::TMem **));

}

REAL TAB_MATRICE::set (REAL s)
{

	// prima controlla se la variabile è stata dichiarata
	VerificaEsistenza () ;

	// la struttura per memorizzare i dati
	TAB_MATRICE::TMem * mem = * (TMem **) VariableTable->GetData(id.c_str());

	mem->value = mem->matrice->value ( 0, s) ;

	VariableTable->Insert(id.c_str(), &mem, sizeof( TAB_MATRICE::TMem **));
	return mem->value ;

}

REAL TAB_MATRICE::set2 (REAL r1, REAL r2 )
{

	// prima controlla se la variabile è stata dichiarata
	VerificaEsistenza () ;

	// la struttura per memorizzare i dati
	TAB_MATRICE::TMem * mem = * (TMem **) VariableTable->GetData(id.c_str());

	mem->value = mem->matrice->value (r1, r2 ) ;

	VariableTable->Insert(id.c_str(), &mem, sizeof( TAB_MATRICE::TMem **));
	return mem->value ;

}

REAL TAB_MATRICE::eval()
{

	// la struttura per memorizzare i dati
	TAB_MATRICE::TMem * mem = * (TAB_MATRICE::TMem **) VariableTable->GetData( id.c_str() );

	return mem->value ;

}

void TAB_MATRICE::ReleaseVariableMem (void * data)
{

	if (data) {
		TAB_MATRICE::TMem * mem = * reinterpret_cast<TAB_MATRICE::TMem **> (data) ;
		delete mem ;
	}

}

// implementazione della classe TFUNZIONE

TFUNZIONE::TFUNZIONE (char *key) :
	TIdent (key)
{

	Tipo tipo = tpFUNZIONE ;
	TipoIdentificatore.Insert(id.c_str(), &tipo, sizeof(TIdent::Tipo));

}

TFUNZIONE::~TFUNZIONE ()
{
}

REAL TFUNZIONE::set (REAL s)
{

	// prima controlla se la variabile è stata dichiarata
	VerificaEsistenza () ;

	// la struttura per memorizzare i dati
	TFUNZIONE::TMem * mem = * (TMem **) VariableTable->GetData(id.c_str());

	// assegna alla variabile name il valore s
	if (mem->var_name) {
		TID ident (mem->var_name) ;
		ident.inizializza (s) ;
	}

	// risolve l'espressione
	mem->value = mem->root->eval() ;
	VariableTable->Insert(id.c_str(), &mem, sizeof( TFUNZIONE::TMem **));

	return mem->value ;

}

REAL TFUNZIONE::eval()
{

	// la struttura per memorizzare i dati
	TFUNZIONE::TMem * mem = * (TFUNZIONE::TMem **) VariableTable->GetData(id.c_str());

	return mem->root->eval () ;

}

void TFUNZIONE::Make (const char * ident, TNODO * espr)
{

	// verifica se esiste già una struttura dati
	if (!VariableTable->Exist (id.c_str()))	{

		void * data = VariableTable->GetData(id.c_str()) ;

		if (data) {
			TFUNZIONE::TMem * mem = * (TFUNZIONE::TMem **) data ;

	   	// se si la distrugge
			if (mem) delete mem ;
      }	// if data

   }	// if (!VariableTable->Exist (id))

   // ne crea una nuova
   TFUNZIONE::TMem * mem = new TFUNZIONE::TMem ;

   mem->root = espr ;

	if (ident) {
		mem->var_name = new char [strlen(ident) + 1] ;
		strcpy(mem->var_name, ident) ;
	}
	else
		mem->var_name = NULL ;

	mem->value = 0 ;

	VariableTable->Insert(id.c_str(), &mem, sizeof( TFUNZIONE::TMem **));

}

void TFUNZIONE::ReleaseVariableMem (void * data)
{

	if (data) {
		TFUNZIONE::TMem * mem = * reinterpret_cast<TFUNZIONE::TMem **> (data) ;
		delete mem ;
	}

}


