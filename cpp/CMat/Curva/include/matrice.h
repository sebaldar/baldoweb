
// *******************
// *** MATRICE.H   ***
// *******************

#ifndef _matrice_
#define _matrice_

#ifndef _curva_
    #include <Curva.h>
#endif  // ifndef _curva_

#ifndef _TYPEDEF_
   #include <typedef.h>
#endif

#ifndef __xml__
	#include <xml.hpp>
#endif

#include <iostream>
#include <vector>

// dichiarazione della classe tabmix che memorizza l'insieme della
// curve associate al parametro p

class Matrice {

	std::string nome;				// nome tabella
	std::string descrizione;	// descrizione tabella

	unsigned int El;		// numero di elementi
	bool sorted;	// la matrice è ordinata ?

	void Init ( XMLElement * root );

	struct TAB {

		REAL p;						// parametro
		PCurva *curva;	      // vettore di curve per p

		TAB( REAL val, PCurva * c ) :
			p(val), curva(c) {
		}

		~TAB() {
			delete curva;
		}

	};

	std::vector<TAB *> tab;

	unsigned int LookUp ( REAL x ) const;	// ritorna l'indice di p immediatamente inferiore a x
	void Sort(int left, int right);
	void Swap(int i, int j);

public:

	Matrice( const char * xml_file );
	Matrice( XMLElement * root );

	~Matrice();

	REAL value ( REAL c, REAL t ) ;

};


#endif
