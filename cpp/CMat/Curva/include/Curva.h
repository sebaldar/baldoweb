#ifndef _curva_
#define _curva_

#include <iostream>
#include <vector>

#include <string>
#include <stdio.h>

#ifndef _TYPEDEF_
   #include <typedef.hpp>
#endif

#ifndef __xml__
	#include "xml.hpp"
#endif

struct TPunto {
	REAL x, y;
};

typedef TPunto Punto;

struct Limiti {

	REAL min_x;
	REAL max_x;
	REAL min_y;
	REAL max_y;

	Limiti () :
		min_x(0), max_x(0), min_y(0), max_y(0)
	{}
};

// una struttura di base delle curva
class TCurvaBase {

protected :

	size_t El ;		// numero degli elementi presenti
	const int minMem ;	// minima memoria richiesta

	size_t fSize ;		// dimensione massima

	TPunto * punto ;

	// chiede nuova memoria al sistema operativo
	void GetNewMemory () ;

	// cerca in curva se esiste x, nel caso ritorna il suo indice
	// altrimenti < 0
	int LookUp ( REAL x ) ;

	// ordina i punti della curva
	void Sort( int left, int right ) ;

	// scambia due elementi della curva
	void Swap( int i, int j ) ;

	// interpola il valore x
	virtual REAL Interpola ( size_t index, REAL x ) = 0;
	// inverte x<->y dell'array p
	void Invert();

public :

	TCurvaBase () ;
	virtual ~TCurvaBase ();

	// aggiunge un nuovo punto alla struttura Point
	// i ritorna un puntatore alla struttura creata,
	// altrimenti NULL
	void *add ( REAL x, REAL y );

	// ricava y per interpolazione data x
	REAL value ( REAL x );
	// ricava x per interpolazione data y
	REAL Xvalue ( REAL y );

	Limiti limiti () ;

	size_t getEl ();	// numero di elementi
	Punto getPoint( size_t i);	// ritorna il punto i-esimo


} ;

class Curva : public TCurvaBase {


	std::string nome;
	std::string descrizione;


	virtual REAL Interpola ( size_t /* index */, REAL /* x */ )  {
		throw ( "" );
	}

protected:

	XMLElement * doc_root;

public:

	Curva () ;
	Curva ( const char * xml_file ) ;
	Curva ( XMLElement * root ) ;
	void caricaTabella ( XMLElement * root ) ;

};

class LCurva : public Curva {


	virtual REAL Interpola ( size_t index, REAL x ) ;

public:

	LCurva () ;
	LCurva ( const char * xml_file ) ;
	LCurva ( XMLElement * root ) ;

};

class PCurva : public Curva {


	REAL dy;
	virtual REAL Interpola ( size_t index, REAL x ) ;

public:

	PCurva () ;
	PCurva ( const char * xml_file ) ;
	PCurva ( XMLElement * root ) ;

};

class CSICurva : public Curva {

	size_t prev_klo;
	size_t prev_khi;

	REAL *x ;
	REAL *y ;
	REAL *y2 ;

	REAL tg_y1;
	REAL tg_yn;

	void init ();
	void spline( REAL yp1, REAL ypn );
	REAL splint( REAL x );

	virtual REAL Interpola ( size_t index, REAL x ) ;

public:

	CSICurva () ;
	CSICurva ( const char * xml_file ) ;
	CSICurva ( XMLElement * root ) ;

	~CSICurva () ;

};

#endif
