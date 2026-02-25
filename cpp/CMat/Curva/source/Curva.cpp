//---------------------------------------------------------------------------
#include <stdlib.h> // per atof
#include <math.h>
#include <vector>

#include <sstream>

#include <Curva.h>

#ifndef until_h
	#include "Utils.h"
#endif
#ifndef _field_h
	#include <field.hpp>
#endif


//---------------------------------------------------------------------------// implementazione classe base
TCurvaBase::TCurvaBase () :
	El(0), minMem (10)
{

	punto = new TPunto [minMem] ;
	fSize = minMem ;

}

TCurvaBase::~TCurvaBase ()
{
	delete [] punto ;
}

void TCurvaBase::GetNewMemory ()
{

	TPunto * newPointArray = new TPunto [fSize + minMem] ;

	// trasferisce i vecchi punti nel nuovo vettore
	for  ( size_t i = 0; i < El; i++)
		newPointArray [i] = punto [i] ;

	// cancella il vecchio vettore
	delete [] punto ;

	// lo trasforma nel nuovo
	punto = newPointArray ;

	fSize += minMem ;

}

// cerca in curva se esiste x, nel caso ritorna il suo indice
// altrimenti < 0
int TCurvaBase::LookUp ( REAL x )
{

	int low = 0,
		 high = El - 1,
		 mid = 0;

	while ( low <= high ) {

		mid = ( low + high ) / 2;

		if ( x < punto[mid].x )
			high = mid - 1;
		else if (x > punto[mid].x)
			low = mid + 1;
		else
			break;

	}

	return mid <= 0 ? mid :
		( x < punto[mid].x ) ?
			mid - 1 :
			mid;

}

void TCurvaBase::Sort(int left, int right)
{

	int i, last;

	if (left >= right) 	// se il vettore contiene meno di due elementi
		return;		// non fa niente

	//sposta l'elemento discriminante in left
	Swap (left, (left + right)/2);
	last = left;

	for (i = left + 1; i <= right; i++)   	// suddivide
		if (punto[i].x < punto[left].x)
			Swap(++last, i);

	// ripristina l'elemento discriminante
	Swap(left, last);

	Sort(left, last - 1);
	Sort(last + 1, right);

}

void TCurvaBase::Swap(int i, int j)
{

// scambia p[i] con p[j]

	TPunto temp = punto[i];

	punto[i] = punto[j];
	punto[j] = temp;
}

void *TCurvaBase::add ( REAL x, REAL y )
{

	// se siamo al limite chiedi ancora memoria al sistema operativo
	if (El == fSize - 1) GetNewMemory () ;

	int index = LookUp( x );

	if ( El > 0 && punto[index].x == x ) {
		punto[index].y = y;

		return &punto[index];
	}
	else {

		if ( El < fSize ) {

			punto[El].x = x;
			punto[El].y = y;

			El++;

			Sort(0, El - 1);

			return &punto[El - 1];

		}
		else
			return NULL;

	}

}


// ricava y per intepolazione data x
REAL TCurvaBase::value ( REAL  x )
{

	int index = LookUp( x );

	if ( punto[index].x == x )
		return punto[index].y;
	else if (El == 1)
		return punto[0].y;
	else
		return Interpola( index, x );

}

Limiti TCurvaBase::limiti (  )
{

	Limiti lim;

	for ( size_t i = 0; i < El; i++ ) {

		REAL x = punto[i].x;
		REAL y = punto[i].y;

		if ( x < lim.min_x ) lim.min_x = x;
		if ( y < lim.min_y ) lim.min_y = y;
		if ( x > lim.max_x ) lim.max_x = x;
		if ( y > lim.max_y ) lim.max_y = y;

	}

	return lim;

}

REAL TCurvaBase::Xvalue ( REAL y )
{

	// inverte x con y
	REAL value;

	Invert();

	int index = LookUp( y );

	if ( punto[index].x == y )
		value = punto[index].y;
	else if (El == 1)
		value = punto[0].y;
	else
		value = Interpola( index, y );

	// ripristina
	Invert();

	return value;

}

// inverte x<->y dell'array p
void TCurvaBase::Invert()
{
	for ( size_t i = 0; i < El; i++) {
		REAL t = punto[i].x;
		punto[i].x = punto[i].y;
		punto[i].y = t;
	}

	Sort(0, El-1);
}


size_t TCurvaBase::getEl ()
{
	return El;
}

Punto TCurvaBase::getPoint( size_t i)
{
	return punto[i];
}



// implementazione classe curva
Curva::Curva () :	TCurvaBase ()
{
}

Curva::Curva ( const char * xml ) :	TCurvaBase ()
{

	XMLDocument doc ;

	doc.parse ( xml ) ;

	if ( doc.firstChild()->type() == XMLNode::TEXT ) {

		doc.loadFromFile ( xml ) ;

	}

	XMLElement *root = doc.rootElement();
	doc_root = root;

	try {
        caricaTabella( root );
    } catch ( const std::string &e ) {
        throw ( e );
    }

}

Curva::Curva ( XMLElement * root ) :	TCurvaBase ()
{

	doc_root = root;

	try {
        caricaTabella( root );
    } catch ( const std::string &e ) {
        throw ( e );
    }

}

void Curva::caricaTabella ( XMLElement * root )
{

	nome = root->getAttribute( "nome");
	descrizione = root->getAttribute( "descrizione");

	NODES v = root->getElementsByTagName("dato");
	for ( size_t i = 0; i < v.size(); i++) {
		XMLElement *el = v[i];
		REAL x = el->getAttribute("x").AsFloat;
		REAL y = el->getAttribute("y").AsFloat;
		add(x,y);
	}

	/* sotto il tag <dati> i dati sono raggruppati
	 * in righe separate
	 * ogni riga ha alternativamente i valori di x e y ciascuno
	 * separato da uno o più spazi
	*/
	NODES v1 = root->getElementsByTagName("dati");
	for ( size_t i = 0; i < v1.size(); i++) {

		std::vector< std::string > dati;
		XMLNode *n = v1[i]->firstChild();
		while ( n ) {

			if ( n->type() == XMLNode::TEXT ) {

				std::string valori = Trim(n->value());

				std::vector< std::string > temp;
				utils::split ( valori, "\n", temp );
				// elimina le righe vuote
				for ( size_t i= 0; i < temp.size(); i++ ) {
					std::string d = utils::trim(temp.at(i));
					if ( d != "" )
						dati.push_back( d );
				}

			}
			else if ( n->type() != XMLNode::COMMENT ) {

				std::ostringstream oss ( "" );
				oss << "Il tag " << n->value() << " non è previsto nella lettura della tabella " <<  nome ;
				std::string str = oss.str();
				throw ( oss.str() );

			}

			n = n->nextSibling();

		}


		if ( dati.size() % 2 != 0 ) {
			// le righe non sono pari (coordinate x e y)
			std::ostringstream oss ( "" );
			oss << "Errore nella lettura della tabella " <<  nome ;
			oss << " ci sono " << dati.size() << " dati";
			std::string str = oss.str();
			throw ( oss.str() );
		}

		for ( size_t i = 0; i < dati.size(); i+=2 ) {

			std::vector <std::string> x, y;
			utils::split ( dati[i], ",", x);
			utils::split ( dati[i+1], ",", y);

			size_t sx = x.size();
			size_t sy = y.size();

			if ( sx != sy ) {
                std::ostringstream oss ( "" );
                oss << "Errore nella lettura della tabella " <<  nome << ": ci sono "
                    << sx << " valori di x e " << sy <<  " valori di y"  ;
                std::string str = oss.str();
				throw ( oss.str() );
			}

            for ( size_t j=0; j<sx && j<sy; j++ ) {
                TFIELD fx = Trim(x[j]), fy=Trim(y[j]);
                REAL rx = fx.AsFloat;
                REAL ry = fy.AsFloat;

                add( rx, ry);
            }

		}

	}

}
// implementazione classe lcurva
LCurva::LCurva () :	Curva ()
{
}

LCurva::LCurva ( const char * xml ) :	Curva (xml)
{
}

LCurva::LCurva ( XMLElement * root ) :	Curva ( root )
{
}

REAL LCurva::Interpola ( size_t v, REAL xnt )
{

	/* interpola xnt nel vettore array fino ad un massimo di n
	prima cerca l intervallo da interpolare
	agli estremi esegue estrapolazione
	all interno interpolazione */

	if ( El == 0 ) return 0;
	if ( El == 1 ) return punto[0].y;

	int i = ( v == El - 1 ) ? --v : v ;

	return punto[i].y + (punto[i+1].y - punto[i].y) *
			 ((xnt - punto[i].x) / (punto[i+1].x -  punto[i].x));

}

// implementazione classe PCURVA
PCurva::PCurva () :	Curva ()
{
}

PCurva::PCurva ( const char * xml ) :	Curva (xml)
{
}

PCurva::PCurva ( XMLElement * root ) :	Curva ( root )
{
}

REAL PCurva::Interpola ( size_t /* j1 */, REAL x )
{

	if ( El == 0 ) return 0;
	if ( El == 1 ) return punto[0].y;
	if ( El == 2 ) {

		int i = 0;

		return punto[i].y + (punto[i+1].y - punto[i].y) *
			 ((x - punto[i].x) / (punto[i+1].x -  punto[i].x));

	}


	int mm = El ;

	int i, m, ns = 0;
	REAL y, den, dif, dift, ho, hp, w;

	std::vector < REAL > c(mm), d(mm);
	dif = fabs(x-punto[0].x);

	for ( i = 0; i < mm; i++ ) {

		dift = fabs(x-punto[i].x);
		if ( dift  < dif ) {
			ns = i;
			dif = dift;
		}
		c[i] = punto[i].y;
		d[i] = punto[i].y;

	}

	y=punto[ns--].y;

	for ( m=1; m<mm; m++) {

		for (i=0;i<mm;i++) {
			ho=punto[i].x-x;
			hp=punto[i+m].x-x;
			w=c[i+1]-d[i];
			if  ( (den=ho-hp) == 0.0 )
				throw ( std::string("poly_interp error") );
			den=w/den;
			d[i]=hp*den;
			c[i]=ho*den;
		}

		y += ( dy = ( 2*(ns+1) < (mm-m) ? c[ns+1]  :  d[ns--] ) );

	}

	return y;




}

// implementazione classe CSICURVA
CSICurva::CSICurva () :	Curva ()
{
}

CSICurva::CSICurva ( const char * xml ) :	Curva (xml)
{

	init();

}

CSICurva::CSICurva ( XMLElement * root ) :	Curva ( root )
{

	init();

}

void CSICurva::init (  )
{

	prev_klo = prev_khi = 0;

	x = new REAL[ El + 1];
	y = new REAL[ El + 1];

	for ( size_t i = 0; i < El; i++) {
		x[i+1] = punto[i].x;
		y[i+1] = punto[i].y;
	}

	y2 = new REAL[ El + 1];

	std::string tg1 = doc_root->getAttribute( "tg_y1");
	std::string tgn = doc_root->getAttribute( "tg_yn");

	if ( tg1 == "" )
		tg_y1 = 1e30;
	else
		tg_y1 = doc_root->getAttribute( "tg_y1").AsFloat;

	if ( tgn == "" )
		tg_yn = 1e30;
	else
		tg_y1 = doc_root->getAttribute( "tg_yn").AsFloat;

	spline ( tg_y1, tg_yn );

}

CSICurva::~CSICurva ()
{
	delete [] x;
	delete [] y;
	delete [] y2;

}

REAL CSICurva::Interpola ( size_t /* p */, REAL x_val )
{

	return splint ( x_val );

}

void CSICurva::spline( REAL yp1, REAL ypn )
{
/*
Given arrays x[1..n] and y[1..n] containing a tabulated function, i.e.,
yi= f (xi), with
x1 < x2 < . . . < xN , and given values yp1 and ypn for the ﬁrst derivative of the interpolating
function at points 1 and n, respectively, this routine returns an array y2[1..n] that contains
the second derivatives of the interpolating function at the tabulated points xi. If yp1 and/or
ypn are equal to 1 × 10 30
or larger, the routine is signaled to set the corresponding boundary
condition for a natural spline, with zero second derivative on that boundary.
*/

	REAL qn, un;

	REAL * u = new REAL [ El + 1 ];;
	if (yp1 > 0.99e30) 	// The lower boundary condition is set either to be “natural”
		y2[1]=u[1]=0.0;
	else { 				// or else to have a speciﬁed ﬁrst derivative.
		y2[1] = -0.5;
		u[1] = ( 3.0 / ( x[2] - x[1] ) ) * ( ( y[2] - y[1] ) / ( x[2] - x[1] ) - yp1 );
	}

	for ( size_t i = 2; i <= El - 1; i++) {
	/* This is the decomposition loop of the tridiagonal algorithm. y2 and u are used for temporary storage of the decomposed
	 * factors.
	 */
		REAL sig= ( x[i]-x[i-1] ) / ( x[i+1] - x[i-1] );

		REAL p = sig * y2[i-1] + 2.0 ;

		y2[i] = (sig-1.0) / p;

		u[i] = (y[i+1] - y[i]) / ( x[i+1] - x[i] ) - ( y[i] - y[i-1] ) / ( x[i] - x[i-1] );

		u[i]=( 6.0 * u[i] / ( x[i+1] - x[i-1] ) - sig * u[i-1] ) / p;

	}


	if (ypn > 0.99e30) 	// The upper boundary condition is set either to be “natural”
		qn=un=0.0;
	else { 	// or else to have a speciﬁed ﬁrst derivative.
		qn=0.5;
		un=(3.0/(x[El] - x[El-1])) * (ypn-(y[El] - y[El-1])/(x[El]-x[El-1]));
	}

	y2[El] = ( un - qn * u[El-1] ) / ( qn * y2[El-1] + 1.0 );

	for ( size_t k = El - 1; k >= 1; k-- ) 	// This is the backsubstitution loop of the tridiagonal algorithm.
		y2[k] = y2[k] * y2[k+1] + u[k];

	delete [] u;

}

REAL CSICurva::splint( REAL x_val )
{
/*
Given the arrays xa[1..n] and ya[1..n], which tabulate a function (with the xai’s in order),
and given the array y2a[1..n], which is the output from spline above, and given a value of
x, this routine returns a cubic-spline interpolated value y.
* */

	REAL * xa = x;
	REAL * ya = y;
	REAL * y2a = y2;

/*We will ﬁnd the right place in the table by means of
bisection. This is optimal if sequential calls to this
routine are at random values of x. If sequential calls
are in order, and closely spaced, one would do better
to store previous values of klo and khi and test if
they remain appropriate on the next call.
* */

	int klo ;
	int khi ;
	if ( ( xa[prev_klo] < x_val ) && ( x_val < xa[prev_khi] ) ) {

		klo = prev_klo;
		khi = prev_khi;

	}
	else {

		klo = 1;
		khi = El;
		while ( khi - klo > 1) {

			int k = ( khi + klo ) >> 1;

			if ( xa[k] > x_val ) khi=k;
			else klo = k;

		}

		prev_klo = klo;
		prev_khi = khi;

	}

//klo and khi now bracket the input value of x.

	REAL h = xa[khi] - xa[klo];
	if ( h == 0.0 ) {
		throw(std::string("Bad xa input to routine splint"));  //The xa’s must be distict
	}

	REAL a = ( xa[khi] - x_val) / h;
	REAL b = ( x_val - xa[klo]) / h; // Cubic spline polynomial is now evaluated.

	return a * ya[klo] + b * ya[khi] +
			( ( a * a * a - a ) * y2a[klo] +
				( b * b * b - b ) * y2a[khi]) * ( h * h ) / 6.0;


}

