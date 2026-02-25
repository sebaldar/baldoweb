#include <point.hpp>
#include <line.hpp>

// funzioni statiche
Point Point::middle ( const Point & p1, const Point & p2 )
{

	return Point (
	
		( p1.x + p2.x ) / 2,
		( p1.y + p2.y ) / 2,
		( p1.z + p2.z ) / 2
	
	) ;
		
}

Point Point::intermediate ( const Point & p1, const Point & p2, double percent )
{

	double r = Point::distance( p1, p2 );
	StraightLine line ( p1, p2 );

	return line.pointAtDistance( ( percent / 100 ) * r );

}

double Point::distance ( const Point & p1, const Point & p2 )
{

	return pow( distance2( p1, p2 ), 0.5 );
		
}

double Point::distance2 ( const Point & p1, const Point & p2 )
{

	double sqr =
		(p1.x - p2.x) * (p1.x - p2.x) +
		(p1.y - p2.y) * (p1.y - p2.y) +
		(p1.z - p2.z) * (p1.z - p2.z)
		;

	return sqr;
		
}

// fine funzioni statiche

Point::Point ( ) :
	x(0), y(0), z(0)
{
}

Point::Point ( double _x, double _y, double _z ) :
	x(_x), y(_y), z(_z)
{
}

Point::Point ( const Point & point ) :
	x( point.x ), y( point.y ), z( point.z )
{
}

Point::Point ( const std::vector< double > & v ) :
	x( v[0] ), y( v[1] ), z( v[2] )
{
}

double Point::distance ( const Point & p ) const
{
	return distance( *this, p );
}

Point &Point::operator = ( const Point & p )
{

	x = p.x;
	y = p.y;
	z = p.z;

	return *this;

}


