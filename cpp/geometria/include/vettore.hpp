#ifndef _vettore_
#define _vettore_

#include <point.hpp>
#include <iostream>
#include <math.h>

class Vettore  {

private:

	bool _verbose = false ;

public:

	// una vettore è definito da una direzione ed un modulo
	
	double x, y, z;
	Direction D ;
	double M ;

	Vettore ( ) ;
	Vettore ( const Vettore & vettore ) ;
	Vettore ( const Direction & d,  const double & m ) ;
	Vettore ( double x, double y, double z) ;
	Vettore ( const Point & p );
	
	
	Vettore &operator = ( const Vettore & vettore );

	Direction direction () const;
	void invertDirection () ;

	void verbose ( bool v ) {
		_verbose = v;
	}

	friend Vettore operator+( const Vettore& lhs, const Vettore& rhs ) ;

};


#endif	// if _line_
