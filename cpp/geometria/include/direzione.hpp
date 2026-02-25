#ifndef _direzione_
#define _direzione_

#include <iostream>
#include <vector>
#include <math.h>
#include <point.hpp>

class Direction {
	
public:

	double x, y, z ;

public:

	Direction ( ) {}
	Direction ( const Point & p ) :
		x(p.x), y(p.y), z(p.z) {}
	Direction ( double x, double y, double z ) :
		Direction ( Point (x, y, z) ) {}
	Direction ( const Direction & d ) :
		Direction ( Point (d.x, d.y, d.z) ) {}
	
	// la direzione che unisce il punto p1 al punto p2
	Direction ( const Point & p1, const Point & p2 ) :
		Direction( Point( p2.x-p1.x, p2.y-p1.y, p2.z-p2.z ) ) {}

	Direction &operator = ( const Direction & d ) {

		x = d.x;
		y = d.y;
		z = d.z;

		return *this;

	}
	
	Direction &operator = ( const Point & p ) {

		x = p.x;
		y = p.y;
		z = p.z;


		return *this;

	}

	Direction oppost ( ) {

		return Direction( -x, -y, -z );

	}

};


#endif	// if _direzione_
