#ifndef _TYPEDEF_
#define _TYPEDEF_

#include <iostream>

using REAL = double ;
using FLOAT = float;


enum class CH {

	NONE,
	NUM,
	ALFA,
	SPACE,
	OPERATORE,
	BRAKET,
	DOPPIAPICINI,
	COMMA,
	$,
	COMMAND

};


#ifdef v_ita

#define _ESPRESSIONE "ESPRESSIONE"
#define _VAI "VAI"
#define _LABEL "LABEL"
#define _SE "SE"
#define _MENTRE "MENTRE"
#define _ALLORA "ALLORA"
#define _ALTRIMENTI "ALTRIMENTI"
#define _INIZIA "INIZIA"
#define _FINE "FINE"
#define _NUMERO "NUMERO"
#define _TABELLA "TABELLA"
#define _MATRICE "MATRICE"
#define _FUNZIONE "FUNZIONE"
#define _STRINGA "STRINGA"
#define _ESEGUI "ESEGUI"
#define _CICLO_PER "PER"
#define _FINO_A "FINO"
#define _STAMPA "STAMPA"
#define _RISULTATO "RISULTATO"
#define _COMMAND "COMMAND"

#endif

// versione inglese
#ifdef v_eng

#define _ESPRESSIONE "ESPRESSIONE"
#define _VAI "goto"
#define _LABEL "label"
#define _SE "if"
#define _MENTRE "while"
#define _ALLORA "then"
#define _ALTRIMENTI "else"
#define _INIZIA "begin"
#define _FINE "end"
#define _NUMERO "number"
#define _TABELLA "TABLE"
#define _MATRICE "MATRIX"
#define _FUNZIONE "FUNCTION"
#define _STRINGA "string"
#define _ESEGUI "ESEGUI"
#define _CICLO_PER "for"
#define _FINO_A "until"
#define _STAMPA "print"
#define _RISULTATO "result"
#define _COMMAND "COMMAND"

#endif


enum OP {
	
	OP_NULL = 500,
	OP_PIU,
	OP_MENO,
	OP_DIVISO,
	OP_PER,
	OP_POTENZA,
	OP_MINORE,
	OP_MAGGIORE,
	OP_UGUALE,		// 508
	OP_MAGGIOREUGUALE,
	OP_MINOREUGUALE,
	OP_DIVERSO,
	OP_ASSEGNA,
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_OPENBRAKET,	// 516
	OP_CLOSEBRAKET

};

enum FN {FN_NONE = -1, FN_LOG, FN_RAND, FN_COS, FN_COSH, FN_SIN,
			FN_TAN, FN_SQRT, FN_EXP, FN_DEC, FN_INT, FN_POW, FN_FNZ, FN_LOG10};

enum class KEY {

	NONE = -1,
	DATA,
	NOMEFILE,
	PIG,
	NRECORD,
	MAXRECORD,
	ULTIMORECORD,
	ESISTE,
	DEC,
	LINGUA,
	X,
	VERO,
	FALSO

};


class TNODE ;

// typedef REAL  (TNODE::* PFN) ();
using PFN = REAL ( TNODE::* ) ();

// puntatore a funzione che ritorna un float
// utilizzato per il nodo T_EXT_FUNZ

// typedef REAL (*EXT_FUNZ) ( const char * );
using EXT_FUNZ = REAL (*) ( const char * );

// puntatore a funzione esterna che ritorna un riferimento float
// utilizzato per il nodo TFIELD
//typedef REAL & (*EXT_VALUE)( const char * );
using EXT_VALUE = REAL & ( * ) ( const char * );



#endif


