#include <sstream>

#include <angolo.hpp>
#include <line.hpp>


// membro statici
Radiant StraightLine::angle ( const Direction & D1, const Direction & D2 ) 
{
	
	double cosalfa = 
		( D1.x * D2.x + D1.y * D2.y + D1.z * D2.z ) /
		( 
			pow( D1.x * D1.x + D1.y * D1.y + D1.z * D1.z, 0.5) * 
			pow( D2.x * D2.x + D2.y * D2.y + D2.z * D2.z, 0.5)
		) ;

	return acos(cosalfa) ;

}

Radiant StraightLine::angle ( const StraightLine & L1, const StraightLine & L2 ) 
{
	
	return angle( L1.D, L2.D );

}

Point StraightLine::intersect ( const StraightLine & L, const Plane & P ) 
{
/*
	std::ostringstream sys ;
	sys.precision( 18);
	sys << 

		"<iniz>" << std::endl <<

		"	x=" << L.P.x + L.D.x << std::endl <<
		"	y=" << L.P.y + L.D.y  << std::endl <<
		"	z=" << L.P.z + L.D.z  << std::endl <<

		"	d=" << P.d << " const"  << std::endl <<
		"	l=" << P.O.x << " const" << std::endl <<
		"	m=" << P.O.y << " const" << std::endl <<
		"	n=" << P.O.z << " const" << std::endl <<

		"	a=" << L.D.x << " const" << std::endl <<
		"	b=" << L.D.y << " const" << std::endl <<
		"	c=" << L.D.z << " const" << std::endl <<

		"	x0=" << L.P.x << " const" << std::endl <<
		"	y0=" << L.P.y << " const" << std::endl <<
		"	z0=" << L.P.z << " const" << std::endl <<

		"</iniz>" << std::endl <<
		
		"c*(y-y0)=b*(z-z0)" << std::endl <<
		"c*(x-x0)=a*(z-z0)" << std::endl <<
		"x*l+y*m+z*n+d=0" << std::endl ;
		
	try {
		
		Sistema sistema ( sys.str() );
		sistema.verbose( true );
		sistema.esegui ();

		double x = sistema.variabile ( "x" ) ;
		double y = sistema.variabile ( "y" ) ;
		double z = sistema.variabile ( "z" ) ;

		return Point( x, y, z );
	
	} catch ( const std::string & e ) {
		
		throw ( e ) ;

	}

*/

	double 	// direzione piano
		l = P.O.x,
		m = P.O.y,
		n = P.O.z ;
		
	double	// direzione linea
		a = L.D.x,
		b = L.D.y,
		c = L.D.z ;
		
	double 	// punto iniziale linea
		x0 = L.P.x,
		y0 = L.P.y,
		z0 = L.P.z ;
		
	double	// coeffciente piano
		d = P.d ;
		
	double x = ( x0 * ( b * m + c * n ) - a * ( z0 * n + y0 * m + d ) ) /
					( a * l + b * m + c * n ) ;
	double y = ( y0 * ( a * l + c * n ) - b * ( z0 * n + x0 * l + d ) ) /
					( a * l + b * m + c * n ) ;
	double z = ( z0 * ( a * l + b * m ) - c * ( y0 * m + x0 * l + d ) ) /
					( a * l + b * m + c * n ) ;
					
	return Point( x, y, z );
	
}


Direction StraightLine::orto ( const StraightLine & v,  const StraightLine & w )
{

	return Direction (
		( v.D.y * w.D.z - v.D.z * w.D.y ),
		( v.D.z * w.D.x - v.D.x * w.D.z ),
		( v.D.x * w.D.y - v.D.y * w.D.x )
		);
		
}

// fine membri statici




StraightLine::StraightLine ( ) :
    D ( 0, 0, 1 ), P( 0, 0, 0 )
{
}

StraightLine::StraightLine ( const StraightLine & line ) :
    D( line.D ), P( line.P )
{
}

StraightLine::StraightLine ( const Direction & d, const Point & p ) :
    D( d ), P( p )
{
}

StraightLine::StraightLine ( const Point & p1,  const Point & p2 ) : 
    D( p2.x-p1.x, p2.y-p1.y, p2.z-p1.z ), P( p1 )
{
}

	

StraightLine &StraightLine::operator = ( const StraightLine & line )
{

	D = line.D;
	P = line.P;

	return *this;

}

bool StraightLine::isInLine ( const Point & p )
{
	return
		(p.x - P.x) * D.y - (p.y - P.y ) * D.x == 0 &&
		(p.x - P.x) * D.z - (p.z - P.z ) * D.x == 0 ;
}

Radiant StraightLine::angle ( const StraightLine & L ) const
{
	
	return angle( *this, L );

}

Point StraightLine::pointAtDistance ( double r )
{
/*
	Point p1 ( P.x + D.x, P.y + D.y, P.z + D.z );
	std::ostringstream sys ;
	sys << 
		"<iniz>" << std::endl <<
		"	x=" << p1.x << std::endl <<
		"	y=" << p1.y << std::endl <<
		"	z=" << p1.z << std::endl <<
		"	a=" << D.x << " const" << std::endl <<
		"	b=" << D.y << " const" << std::endl <<
		"	c=" << D.z << " const" << std::endl <<
		"	x0=" << P.x << " const" << std::endl <<
		"	y0=" << P.y << " const" << std::endl <<
		"	z0=" << P.z << " const" << std::endl <<
		"	r=" << r << " const" << std::endl <<
		"</iniz>" << std::endl <<
		"c*(y-y0)=b*(z-z0)" << std::endl <<
		"c*(x-x0)=a*(z-z0)" << std::endl <<
		"r*r=(x-x0)^2+(y-y0)^2+(z-z0)^2" << std::endl ;
		
	try {
		
		Sistema sistema ( sys.str() );
		sistema.verbose( _verbose );
		sistema.esegui ();

		double x = sistema.variabile ( "x" ) ;
		double y = sistema.variabile ( "y" ) ;
		double z = sistema.variabile ( "z" ) ;

		return Point( x, y, z );
	
	} catch ( const std::string & e ) {
		
		throw ( e ) ;

	}
*/

	double 
		a = D.x,
		b = D.y,
		c = D.z;
	
	double coeff = r /
		pow( a * a + b * b + c * c, 0.5 );
		
	double x = P.x + a * coeff;
	double y = P.y + b * coeff;
	double z = P.z + c * coeff;
	
	return Point( x, y, z );

}

Direction StraightLine::orto ( const StraightLine & w ) const
{

	return orto ( *this, w ) ;
		
}

Direction StraightLine::direction () const
{
	return D;
}

void StraightLine::invertDirection () 
{

	D.x = -D.x;
	D.y = -D.y;
	D.z = -D.z;

}

