
#include <propulsore.hpp>
#include <rocket.hpp>

Rocket::Rocket():
	OdeFunction( ) 
{
}

Rocket::~Rocket()
{
	if ( stadio1 )
		delete stadio1;
	if ( stadio2 )
		delete stadio2;
	if ( stadio3 )
		delete stadio3;
}

void Rocket::operator () ( const Doub & , VecDoub_I & y, VecDoub_O & dydx ) 
{
}
