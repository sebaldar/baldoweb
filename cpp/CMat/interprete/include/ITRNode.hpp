#ifndef _TNODO_
#define _TNODO_

#include <iostream>
#include <string>

#include <math.h>

#ifndef _hash_
    #include <Hash.h>
#endif

#ifndef _matrice_
    #include <matrice.h>
#endif


/*
22/03/01
	creata nuova classe TTAB
17/04/01
	creata nuova classe TFUNCTION
*/

class TNODO
{

protected:

	TNODO() ;

public:

	enum {
		TP_ALFA,
		TP_NUM,
		TP_DOPPIAPICINI = 3,
		ID,
		EOLN,
		BAD,
		FIELD,
		COMMAND,
		$FIELD,
		FUNZ = 600,
		KEYWORD = 800,
		FUNZIONE_ESTERNA  = 900	// e- una funzione definita esternamente
         };

	virtual ~TNODO() ;
	virtual REAL eval() = 0;

	virtual void print () ;
};

// operatore unario
class TUNOP : public TNODO
{

protected:

	TNODO *operand;

	TUNOP (TNODO *o) ;
	~TUNOP() ;

	virtual void print_op () ;
	void print () ;
};

// operatore binario
class TBINOP : public TNODO
{
protected:

	TNODO *left, *right;

	TBINOP (TNODO *l, TNODO *r) ;
	~TBINOP() ;

	void print () ;
	virtual void print_op() = 0;
};

class TVALUE : public TNODO
{
protected:

	REAL value;

	TVALUE (REAL val) ;
	~TVALUE() ;

	REAL eval ()	;
	void print () ;
};

class TPLUS : public TBINOP
{
public:

	TPLUS(TNODO *l, TNODO *r) ;

	REAL eval () ;

	void print_op() ;
};

class TEQUAL : public TBINOP
{
public:

	TEQUAL (TNODO *l, TNODO *r) ;

	REAL eval () ;

	void print() ;
	void print_op() ;
};

class TNOTEQUAL : public TBINOP
{
public:

	TNOTEQUAL (TNODO *l, TNODO *r) ;

	REAL eval () ;

	void print() ;
	void print_op() ;
};

class TLESSTHAN : public TBINOP
{
public:

	TLESSTHAN(TNODO *l, TNODO *r) ;

	REAL eval () ;

	void print_op() ;
};

class TGREATERTHAN : public TBINOP
{
public:

	TGREATERTHAN(TNODO *l, TNODO *r) ;

	REAL eval () ;

	void print_op() ;
};

class TLESSEQUALTHAN : public TBINOP
{
public:

	TLESSEQUALTHAN(TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

class TGREATEREQUALTHAN : public TBINOP
{

public:

	TGREATEREQUALTHAN (TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

// operatore OR
class TOROP : public TBINOP
{
public:

	TOROP(TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

// operatore AND
class TANDOP : public TBINOP
{
public:

	TANDOP(TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

class TMINUS : public TBINOP
{
public:

	TMINUS(TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

// operatore meno unario
class TUMINUS : public TUNOP
{
public:

	TUMINUS(TNODO *o) ;
	REAL eval () ;
	void print_op() ;
};

// operatore piú unario
class TUPLUS : public TUNOP
{
public:

	TUPLUS(TNODO *o) ;
	REAL eval () ;
	void print_op() ;
};

// operatore not unario
class TNOT : public TUNOP
{
public:

	TNOT(TNODO *o) ;
	REAL eval () ;
	void print_op() ;
};

// operatore moltiplicazione
class TTIMES : public TBINOP
{
public:

	TTIMES (TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

// elevazione a potenza
// esiste anche un tpow che è funzione pow(x,y)
class POTENZA : public TBINOP
{
public:

	POTENZA (TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

class TDIVISO : public TBINOP
{
public:

	TDIVISO (TNODO *l, TNODO *r) ;
	REAL eval () ;
	void print_op() ;
};

// valore numerico
class TFLOAT : public TVALUE
{
public:

	TFLOAT ( REAL f ) ;
	~TFLOAT () ;
};

class TKEY :public TNODO
{

public:

	std::string value;

	TKEY ( const char *s ) ;
	~TKEY () ;

	REAL eval ()	;
	void print () ;

};

// le variabili
class TVAR : public TNODO
{

public:

	TVAR () ;
	~TVAR() ;

	virtual REAL set(REAL s) = 0;

};

// tipo generico identificatore
class TIdent : public TVAR
{

public :
	// tipi di identificatori
	enum  Tipo
		{tpNONE = 0, tpNUMERO, tpTABELLA, tpMATRICE, tpFUNZIONE, tpSTRINGA}  ;

protected :

	std::string id;	// nome identificatore

	// tabella hash per la memorizzazione del nome della variabile
	// e del valore deve essere inizializzato all'esterno di node
	static HashClass	*VariableTable ;

	// tabella corrispondenza nomeIdent tipoIdent
	static HashClass	TipoIdentificatore ;

	bool VerificaEsistenza () ;

public:

	TIdent ( const char *key ) ;

	static void ClearVariableTable ();
	static TIdent::Tipo GetTipo (const char * id) ;
	static void * getData ( const char * id ) {
		return VariableTable->GetData( id );
	}


	static HashClass * getVariableTable () {
		return VariableTable;
	}

	static HashClass * getTipoIdentificatore () {
		return &TipoIdentificatore;
	}

};

// identificatore
class TID : public TIdent
{

protected :

public:

	TID ( const char *key ) ;

	REAL eval() ;
	REAL set (REAL s);    // imposta il valore di una variabile dichiarata
	REAL inizializza (REAL s = 0); // inizializza una variabile non dichiarata
	void print() ;


};

class FIELD_NODE : public TVAR
{

protected:

	REAL *value;

public:

	FIELD_NODE (char *campo) ;
	FIELD_NODE () ;

	REAL eval();
	REAL set (REAL s) ;
	void print() ;

};

class $FIELD_NODE : public FIELD_NODE
{

protected:

	class TESPRESSIONE *esp;

public:

	$FIELD_NODE (class TESPRESSIONE *esp, char *campo) ;
	$FIELD_NODE (class TESPRESSIONE *esp) ;

	void print() ;

};

class FIELD_COMMAND : public $FIELD_NODE
{

	std::string command;

public:

	FIELD_COMMAND (class TESPRESSIONE *esp, char *comando) ;

	REAL eval () ;

	void print() ;

};

// operatore di assignazione
class TASSIGN : public TBINOP
{
public:

	TASSIGN ( TVAR *t, TNODO *e ) ;
	REAL eval() ;
	void print_op() ;
};

// operatore di assignazione
class ASSIGN2: public TNODO
{
	class TAB_MATRICE * var;
	TNODO * arg1;
	TNODO * arg2;

public:

	ASSIGN2 ( TAB_MATRICE *t, TNODO *arg1, TNODO *arg2 ) ;
	REAL eval() ;
	void print_op() ;
};

class TFUNZ : public TNODO
{
protected:

	TNODO *arg;

	TFUNZ (TNODO *o) ;
	~TFUNZ() ;

	virtual void print_op () = 0  ;
	void print () ;
};

class T_EXT_FUNZ : public TNODO
{

    char *arg ;

public:

	T_EXT_FUNZ (const char *arg) ;
	~T_EXT_FUNZ() ;

	REAL eval() ;
 	void print_op() ;
};

class T_FUNZIONE_ESTERNA : public TNODO
{

    char *arg ;
    EXT_FUNZ funz ;

public:

	T_FUNZIONE_ESTERNA (EXT_FUNZ funz, const char *arg) ;
	~T_FUNZIONE_ESTERNA () ;

	REAL eval() ;
 	void print_op() ;
};

// valuta un logaritmo naturale
class TLOG : public TFUNZ
{
public:

	TLOG(TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

// valuta un logaritmo in base 10
class TLOG10 : public TFUNZ
{
public:

	TLOG10 ( TNODO *arg ) ;

	REAL eval() ;
 	void print_op() ;
};

class TCOS : public TFUNZ
{
public:

	TCOS (TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

class TCOSH : public TFUNZ
{
public:

	TCOSH (TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

class TSIN : public TFUNZ
{
public:

	TSIN(TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

class TTAN : public TFUNZ
{
public:

	TTAN (TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

class TSQRT : public TFUNZ
{
public:

	TSQRT (TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

class TEXP : public TFUNZ
{
public:

	TEXP (TNODO *arg) ;

	REAL eval() ;
 	void print_op() ;
};

class TPOW  : public TNODO
{

	TNODO *arg1, *arg2;

public:

	TPOW (TNODO *arg1, TNODO *arg2) ;
	~TPOW () ;

	REAL eval() ;
 	void print () ;
};

class TRAND  : public TNODO
{

	TNODO *arg1, *arg2;

public:

	TRAND (TNODO *arg1, TNODO *arg2) ;
	~TRAND () ;

	REAL eval() ;
 	void print () ;
};

class TSTRINGA : public TIdent
{


	// dichiarati qui privati perchè non utilizzabili per la classe TSTRING
	REAL inizializza (REAL s = 0);

public:

	TSTRINGA ( const char *key ) ;
	~TSTRINGA () ;

	REAL eval() ;

	void set ( const std::string & value  );
	REAL set (REAL /* s*/ ) { return 0; }


};

class TTAB : public TIdent
{

	// valore memorizzato da set(),
	// il valore da ricercare nella tabella
	REAL value ;

	// dichiarati qui privati perchè non utilizzabili per la classe TTAB
	REAL inizializza (REAL s = 0);

public:

	TTAB ( const char *key ) ;
	~TTAB () ;

	REAL eval() ;

	REAL set (REAL s);

	void MemorizzaTabella ( class XMLElement * root ); // memorizza gli elementi della tabella
	void MemorizzaTabella ( const char * data_xml ); // memorizza gli elementi della tabella

	static void ReleaseVariableMem (void * ) ;

	// la struttura per memorizzare i dati
	struct TMem {

		REAL value ;	// memorizza il risultato
		Curva * curva ;

		~TMem () {
			delete curva ;
		};

	} ;

};

class TAB_MATRICE : public TIdent
{

	// valore memorizzato da set(),
	// il valore da ricercare nella tabella
	REAL value ;

	// dichiarati qui privati perchè non utilizzabili per la classe TTAB
	REAL inizializza (REAL s = 0);

	// la struttura per memorizzare i dati
	struct TMem {

		REAL value ;	// memorizza il risultato
		Matrice * matrice ;

		~TMem () {
			delete matrice ;
		};

	} ;

public:

	TAB_MATRICE ( const char *key ) ;
	~TAB_MATRICE () ;

	REAL eval() ;

	REAL set (REAL s);
	REAL set2 (REAL r1, REAL r2 );

	void MemorizzaTabella ( const char * data_xml ); // memorizza gli elementi della tabella
	void MemorizzaTabella ( class XMLElement * root ); // memorizza gli elementi della tabella

	static void ReleaseVariableMem (void * ) ;

};

class TFUNZIONE : public TIdent
{

   // la struttura per memorizzare i dati
	struct TMem {

		char * var_name ;	//  nome della variabile
		REAL value ;	// memorizza il risultato
		TNODO * root ;

		TMem () :
			var_name(NULL), root (NULL)
		{
		} ;

		~TMem () {
			if (var_name) delete [] var_name ;
			if (root) delete root ;
		};

	} ;

public:

	TFUNZIONE (char *key) ;
	~TFUNZIONE () ;

	REAL eval() ;

	REAL set (REAL s);

	// costruisce l'espREALsione
	void Make (const char * ident, TNODO * espr) ;
	static void ReleaseVariableMem (void * ) ;

};

#endif
