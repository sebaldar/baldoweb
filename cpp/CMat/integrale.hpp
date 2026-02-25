#ifndef _integrale_
#define _integrale_

#ifndef _NR3_H_
	#include <nr3.hpp>
#endif

struct NumIntegrale {	// integrazione numerica
	
	virtual Doub func( Doub x1 ) = 0;
		
	const Doub EPS = 1.0e-5;
	const int JMAX = 20;
	
	NumIntegrale () {
	}
	

	Doub operator () ( Doub x1, Doub x2 ) {
/*
Returns the integral of the function func from a to b. The parameters EPS can be set to the
desired fractional accuracy and JMAX so that 2 to the power JMAX-1 is the maximum allowed
number of steps. Integration is performed by the trapezoidal rule.
*/

		Doub s, olds=0.0; // Initial value of olds is arbitrary.

		for ( int j = 1; j <= JMAX; j++ ) {
		
			s  = trapzd( x1, x2,j );
			
			if (j > 5) // Avoid spurious early convergence.
				if ( fabs(s-olds) < EPS * fabs(olds) ||	(s == 0.0 && olds == 0.0)) return s;
			olds=s;
		
		}
		
		throw("Too many steps in routine qtrap");
		return 0.0; // Never get here.

		
	}
	
	Doub trapzd( Doub a, Doub b, int n )
/*
This routine computes the nth stage of refinement of an extended trapezoidal rule. func is input
as a pointer to the function to be integrated between limits a and b, also input. When called with
n=1, the routine returns the crudest estimate of  b
a f(x)dx. Subsequent calls with n=2,3,...
(in that sequential order) will improve the accuracy by adding 2n-2 additional interior points.
*/
	{
		
		Doub x, tnm, sum, del;
		static Doub s;
		int it,j;
		if (n == 1) {
			return ( s = 0.5 * (b-a) * (func(a)+func(b) ) );
		} else {
		
		for ( it=1, j=1; j < n-1; j++) it <<= 1;
		
			tnm=it;
			del = (b-a) / tnm; // This is the spacing of the points to be added.
			x = a + 0.5 * del;
			
			for ( sum=0.0, j=1; j<=it; j++, x+=del) sum += func(x);
			 
			s = 0.5 * ( s + (b-a) * sum / tnm ); // This replaces s by its refined value.
			
			return s;
	
		}
	
	}

};

#endif	// if _integrale_

