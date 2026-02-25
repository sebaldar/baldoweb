#ifndef _espressione_
#define _espressione_

#include <iostream>
#include <string>
using namespace std;

#ifndef _TNODO_
	#include <ITRNode.hpp>
#endif

#include <ext_funz.hpp>


using Espressione = TESPRESSIONE ;
/*
	23/03/01
   aggiunto metodo VerificaAssegnazione che per i nodi tipo TVAR
   verifica se è da effettuare una assegnazione
*/

class TESPRESSIONE
{

	friend class $FIELD_NODE;
	friend class FIELD_COMMAND;
	friend class TINTERPRETE;
// tabella per la memorizzazione dei nodi degli alberi sintattici
	HashClass *SinTreeTbl;

// tabella per la memorizzazione del nome delle funzioni
// definite esternamente
	static class FUNZIONI_ESTERNE
	{

		int n_funzione;

		HashClass * numero_funzione ;	// dato il nome della funzione ritorna il numero di ordine
		HashClass * ordine_funzione ;	// dato il numero d-ordine della funzione ritorna il nome
		HashClass * funzione_esterna ;	// dato il nome della funzione ritorna il puntatore alla funzione

		public :

		FUNZIONI_ESTERNE () ;
		~FUNZIONI_ESTERNE () ;

		bool Definisci (const char * nome_funzione, EXT_FUNZ funzione) ;
		bool IsFunction (const char * nome_funzione ) const ;
		EXT_FUNZ GetFunction (const char * nome_funzione ) const;
		EXT_FUNZ GetFunctionDaNumero (int numero) const;
		int RicavaNumeroFunzione (const char * nome_funzione ) const;

	}  funzione_esterna ;

	char * const lexeme;

	int token;	// token corrente
	const char *esp ;	// l'espressione da esaminare
	char * start_str ;	// l'indirizzo della espressione copiata
	char *str;	// lo scanning lungo la espressione

	void InitEspr (const char *estern_espr) ; // inizializza la espressione

	int scan();

	TNODO *root;

	TNODO *E();
	TNODO *F();
	TNODO *T();
	TNODO *PRIOR1();

	TNODO *MakeTree();

	OP GetOp ( const char *s ) const;
	int NumOpDigit(OP op) const;
	CH GetChar(const char *ch) const;
	int GetNum(char *, char *);
	int GetStrTraDoppiApicini(char *arg, char *str);
	int GetStrInBraket (char *s, char *str) ;
	int  GetNextComma(char *lexeme) ;

	int GetNextOp(char *, char*);

	FN IsFunction(const char *)    const;
	KEY IsKeyWord(const char *) const;

	// ritorna true se è una funzione esterna
	bool IsExtFunction (const char * nome_funzione) const;

	bool isError;
	void error( const char *msg, const char *str ) ;

	$EXT_FUNZ * $node_funz;

	$FUNZ $funz;
	$COMMAND $command;


public:

	TESPRESSIONE (const char * str);
	TESPRESSIONE ();
	~TESPRESSIONE() ;

	void set$ext_funz ( $EXT_FUNZ *n, $FUNZ f) {
		$node_funz = n;
		$funz = f;
	}

	// risolve la espressione e disalloca l'albero sintattico
	REAL Risolvi(const char *);

	void Print() ;

   // disalloca l'albero sintattico
	void Free () ;

	// da implementare
	void VarClear() {}	// pulisce le variabili

	// prepara la espressione creando l'albero sintattico e memorizzandolo
	// nella tabella  , ritorna il puntatore alla radice
	TNODO * MakeTree ( const char *espr )  ;
	void Prepare ( const char *espr ) ;

    // mette in esecuzione la espressione preparata con Prepare
	REAL Execute (const char *espr) ;

    // disalloca l'albero sintattico
	void Clear();

	void DefinisciFunzioneEsterna ( const char * nome_funzione,
   											EXT_FUNZ funzione) ;

	// verifica se si tratta di nodo di assegnazione
	TNODO * VerificaAssegnazione (TNODO * root) ;


} ;

#endif
