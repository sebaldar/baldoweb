#ifndef _plane_
#define _plane_

#include <point.hpp>
#include <iostream>
#include <math.h>

class Plane  {

public:

	Direction O; 	// direzione ortogonale
	double d;
	
public:

	Plane ( ) ;
	Plane ( const Plane & plane ) ;
	Plane ( const class StraightLine & line1, const class StraightLine & line2 ) ;
	Plane ( const Point & P1,  const Point & P2,  const Point & P3 ) ;
	Plane ( const StraightLine & line ) ;
	Plane ( const StraightLine & line, const Point & P ) ;
	Plane ( const Direction & dir, double coeff ) ;
	Plane ( const Direction & dir, const Point & P  ) ;
	
	
	Plane &operator = ( const Plane & plane );
	
	// distanza del punto p dal piano
	double distance ( const Point & p ) const ;
	Direction direction () const;

};


#endif	// if _plane_
