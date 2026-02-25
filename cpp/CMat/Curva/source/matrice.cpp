
// *********************
// *** MATRICE.CPP ***
// *********************

//---------------------------------------------------------------------------
#include <string>

#include <stdlib.h>

#ifndef _matrice_
    #include <matrice.h>
#endif  // ifndef _matrice_


//---------------------------------------------------------------------------

// il costruttore di TabMix
Matrice::Matrice ( const char * xml_file ) :
	El(0), sorted(false)
{

	XMLDocument XML ( xml_file ) ;

	XMLElement * root = XML.rootElement () ;

	Init ( root );

}

Matrice::Matrice ( XMLElement * root ) :
	El(0), sorted(false)
{

	Init ( root );

}

Matrice::~Matrice ()
{
	for ( size_t i = 0; i < El; i++)
		delete tab[i];
}

void Matrice::Init ( XMLElement * root )
{

	nome = root->getAttribute( "nome");
	descrizione = root->getAttribute( "descrizione");

	NODES c = root->getElementsByTagName("curva");
	for ( size_t i = 0; i < c.size(); i++) {

		REAL value = c[i]->getAttribute( "valore").AsFloat;
		PCurva *c = new PCurva ( c[i] );

		TAB * t = new TAB ( value, c );

		tab.push_back( t );
		El++;

	}

}

REAL Matrice::value( REAL p, REAL x )
{

	if ( !sorted ) {
		Sort(0, El-1);
		sorted = true;
	}

	unsigned int i1;
	unsigned int i2;
	unsigned int i3;

	unsigned int i = LookUp( p );

	/*verifica se siamo su una chiave */
	if ( tab[i]->p == x ) {
		return tab[i]->curva->value(p);
	}
	if ( i == El - 1 ) {	// siamo all'ultimo valore
		i2 = i;
		i1 = i-1;
	}
	else {
		i1 = i;
		i2 = i + 1;
	}
	
	// se ci sono più di tre punti crea una nuova curva con tre punti
	if ( El > 3 ) {

		if ( i2 < El ) {
			i3 = i2 + 1;
		}	else {
			i3 = i2;
			i2 = i1;
			i1 = i1-1;
		}

		REAL p1 = tab[i1]->p;
		REAL p2 = tab[i2]->p;
		REAL p3 = tab[i3]->p;

		REAL val1 = tab[i1]->curva->value(p);
		REAL val2 = tab[i2]->curva->value(p);
		REAL val3 = tab[i3]->curva->value(p);
		
		PCurva c;
		c.add( p1, val1 );
		c.add( p2, val2 );
		c.add( p3, val3 );
		
		return c.value( p );

	}	else {

		REAL p1 = tab[i1]->p;
		REAL p2 = tab[i2]->p;

		REAL val1 = tab[i1]->curva->value(p);
		REAL val2 = tab[i2]->curva->value(p);

		return val1 + (p - p1) * (val2-val1) / (p1 - p2) ;
		
	}

}

unsigned int Matrice::LookUp ( REAL x ) const
{

	int low = 0,
		 high = El - 1,
		 mid = 0;

	while ( low <= high ) {

		mid = ( low + high ) / 2;

		if ( x < tab[mid]->p )
			high = mid - 1;
		else if (x > tab[mid]->p)
			low = mid + 1;
		else
			break;

	}

	return mid ;

}

void Matrice::Sort( int left, int right )
{

	int i, last;

	if (left >= right) 	// se il vettore contiene meno di due elementi
		return;		// non fa niente

	//sposta l'elemento discriminante in left
	Swap (left, (left + right)/2);
	last = left;

	for (i = left + 1; i <= right; i++)   	// suddivide
		if ( tab[i]->p < tab[left]->p )
			Swap(++last, i);

	// ripristina l'elemento discriminante
	Swap(left, last);

	Sort(left, last - 1);
	Sort(last + 1, right);

}

void Matrice::Swap( int i, int j )
{

// scambia p[i] con p[j]

	TAB *temp = tab[i];

	tab[i] = tab[j];
	tab[j] = temp;
}


