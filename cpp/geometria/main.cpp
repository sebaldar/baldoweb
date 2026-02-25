#include <limits>

#include <line.hpp>
#include <plane.hpp>

typedef std::numeric_limits< double > dbl;

int main ( int /* argc */, char ** /* argv[] */ )
{

/*	
	Point p1 ( 0, 0, 0 );
	Point p2 ( -2, -2, -2 );
	StraightLine line (p1, p2 );
	
	Plane pl ( line );
	
	Point p = pl.pointToDistance( p1, 1);
	std::cout << p.x <<  " " << p.y <<  " " << " " << p.z << std::endl;

	double r = pl.distance(p);
	std::cout << "distance " << r << std::endl;
	Point p1 ( 1, 2, 1 );
	Point p2 ( 1, 2, 5 );
	Point p3 ( 0, 0, 1 );
	
	StraightLine line1 ( Direction( 1, 1, 1 ), p1 );
	StraightLine line2 ( Direction( 0, 0, 1 ), p1 );
	
	Direction D = line1.orto( line2 );
	std::cout << D.x <<  " " << D.y <<  " " << " " << D.z << std::endl;
	StraightLine line3 ( D, p1 );
	
	Point p = line3.distanceToPoint( 2 );
	std::cout << p.x <<  " " << p.y <<  " " << " " << p.z << std::endl;
*/	
	
	Point p1 ( 0, 0, 0 );
	Point p2 ( 1, 1, 1 );

	StraightLine line ( Direction( 0, 0, 1 ), p1 );
	Plane plane ( Direction( 1, 1, 1 ), p2 );
	
	Point p = StraightLine::intersect( line, plane );
	std::cout << p.x <<  " " << p.y <<  " " << " " << p.z << std::endl;
	
	
	return (0);

	
}
