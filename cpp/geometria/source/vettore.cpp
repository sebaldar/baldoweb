#include <sstream>

#include <vettore.hpp>



Vettore::Vettore ( ) :
    Vettore( Direction( 0, 0, 0 ), 0 )
{
}

Vettore::Vettore ( const Vettore & vettore ) :
	Vettore( vettore.D, vettore.M )
{
}

Vettore::Vettore ( const Direction & d, const double & m ) :
	D( d ), M( abs(m) )
{
	
	// portiamolo a modulo M
	double mod = pow( d.x * d.x + d.y * d.y + d.z * d.z, 0.5 );
		
	double r = M / mod;
		
	D.x = r * D.x;
	D.y = r * D.y;
	D.z = r * D.z;

	x = D.x;
	y = D.y;
	z = D.z;

}

Vettore::Vettore ( double x, double y, double z ) :
	D( Point(x, y, z) ), M( pow( x*x+y*y+z*z, 0.5) )
{

	x = D.x;
	y = D.y;
	z = D.z;

}

Vettore::Vettore ( const Point & p ) :
	D( p), M( p.distance( Point(0,0,0) ) )
{

	x = D.x;
	y = D.y;
	z = D.z;

}

Vettore &Vettore::operator = ( const  Vettore & vettore )
{

	D = vettore.D;
	M = vettore.M;


	x = vettore.x;
	y = vettore.y;
	z = vettore.z;
	return *this;

}


Direction Vettore::direction () const
{
	return D;
}

void Vettore::invertDirection () 
{

	D.x = -D.x;
	D.y = -D.y;
	D.z = -D.z;

}

Vettore operator+( const Vettore& v1, const Vettore& v2 ) {

	Direction D ( v1.D.x+v2.D.x, v1.D.y+v2.D.y, v1.D.z+v2.D.z );
	double M = pow( D.x * D.x + D.y * D.y + D.z * D.z, 0.5 );

	return Vettore ( D, M );

}
