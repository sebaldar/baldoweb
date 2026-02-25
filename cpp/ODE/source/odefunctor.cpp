#include <sstream>

#include <odefunctor.hpp>

OdeFunctor::OdeFunctor (  Doub _x1, Doub _x2, Int n_var, OdeFunction & _d ) :
	Funzione (),
	x1( _x1 ),
	x2( _x2 ),
	nvar( n_var ),
	d( _d ),
	ystart ( nvar ),
	out( 20 )
{
}

OdeFunctor::OdeFunctor ( OdeFunction & _d ) :
	Funzione (),
	x1( 0 ),
	x2( 0 ),
	nvar( 2),
	d( _d ),
	ystart ( nvar ),
	out( 20 )
{
}

void OdeFunctor::set (  Doub _x1, Doub _x2 ) 
{
	x1 = _x1;
	x2 = _x2;
}

OdeFunctor::~OdeFunctor() {
	
}

double OdeFunctor::eval () 
{
	
	ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d ) ;

	ode.integrate();
	return ystart[0];
	
}
