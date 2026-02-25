
#include <plane.hpp>
#include <line.hpp>


Plane::Plane ( ) :
    O( 0, 0, 1 ), d( 0 )
{
}

Plane::Plane ( const Plane & plane ) :
    O( plane.O ), d( plane.d )
{
}

Plane::Plane ( const class StraightLine & /*line1*/, const class StraightLine & /*line2*/ ) 
{
}

Plane::Plane ( const Point & /*P1*/,  const Point & /*P2*/,  const Point & /*P3*/ ) 
{
}

Plane::Plane ( const StraightLine & line ) :
	O ( line.D )
{
	d = -( O.x * line.D.x + O.y * line.D.y + O.z * line.D.z );
}

Plane::Plane ( const StraightLine & line, const Point & p ) :
	O ( line.D )
{
	d = -( O.x * p.x + O.y * p.y + O.z * p.z );
}
	
Plane::Plane ( const Direction & D, const Point & p  ) :
	O ( D )
{
	d = -( O.x * p.x + O.y * p.y + O.z * p.z );
}
	
Plane::Plane ( const Direction & dir, double coeff ) :
	O ( dir ), d( coeff )
{
}
	

Plane &Plane::operator = ( const Plane & plane )
{

	O = plane.O;
	d = plane.d;

	return *this;

}


double Plane::distance ( const Point & p ) const
{

	double a = O.x ;
	double b = O.y ;
	double c = O.z ;
	
	return 
		fabs( a * p.x + b * p.y + c * p.z + d ) /
			pow( a * a + b * b + c * c, 0.5 ) ;

}
	

Direction Plane::direction () const
{
	return O;
}


