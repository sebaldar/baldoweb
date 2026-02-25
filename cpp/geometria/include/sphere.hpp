#ifndef _sphere_
#define _sphere_

#include <point.hpp>
#include <iostream>
#include <math.h>

class Sphere  {

public:

	Point C ; 	// centrodirezione ortogonale
	double R;	// raggio
	
public:

	Sphere ( const Point & center,  double radius ) ;

};


#endif	// if _sphere_
