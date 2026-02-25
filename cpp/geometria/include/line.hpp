#ifndef _line_
#define _line_

#include <plane.hpp>
#include <point.hpp>
#include <iostream>
#include <math.h>

#include <angolo.hpp>

class StraightLine  {

private:

	bool _verbose = false ;

public:

	// una retta è definata da una direzione e un punto
	Direction D ;
	Point P ;

public:

	StraightLine ( ) ;
	StraightLine ( const StraightLine & line ) ;
	StraightLine ( const Point & p1,  const Point & p2 ) ;
	StraightLine ( const Direction & d, const Point & p1 ) ;
	
	
	StraightLine &operator = ( const StraightLine & line );

	static Radiant angle ( const Direction & D1, const Direction & D2 )  ;
	static Radiant angle ( const StraightLine & L1, const StraightLine & L2 ) ;
	// intersezione linea e piano
	static Point intersect ( const StraightLine & L, const Plane & P ) ;
	// direzione perpendicolare a due rette
	static Direction orto ( const StraightLine & L1,  const StraightLine & L2 );
	
	bool isInLine ( const Point & p );
	// trova il punto distante r da P
	Point pointAtDistance ( double r );
	Radiant angle ( const StraightLine & L1 ) const ;
	
	Direction orto ( const StraightLine & L ) const;

	Direction direction () const;
	void invertDirection () ;

	void verbose ( bool v ) {
		_verbose = v;
	}

};

#endif	// if _line_
