#ifndef _derivata_
#define _derivata_

#ifndef _NR3_H_
	#include <nr3.hpp>
#endif

struct NumDerivatives {	// numerica derivatives
	
	virtual Doub func( Doub x ) = 0;
	Doub h;
	Doub err;
	
	NumDerivatives () :
		NumDerivatives( 1 ) {
	}
	
	NumDerivatives ( Doub _h ) :
		h( _h ) {
		if ( h <= 0 )
			throw("h must be notzero in dfridr.");
	}

	Doub operator () ( Doub x ) {
		
		const Int ntab=10;	// set maximum size of tableau
		const Doub con=1.4, con2=(con*con);	// stepsize decreased by CON at each iteration
		const Doub big=numeric_limits<Doub>::max();
		const Doub safe=2.0;

		Doub ans;
		MatDoub a(ntab, ntab);
		
		Doub hh = h;
		a[0][0]=(func(x+hh)-func(x-hh))/(2.0*hh);
		err=big;
		
		for ( Int i=1; i < ntab; i++ ) {
			/*
			 * successive columns in the Neville tableau will go to smaller stepsizes and higher orders of extrapolation
			*/
			hh /= con;
			a[0][i]=(func(x+hh)-func(x-hh))/(2.0*hh);	// try new smaller stepsize
			Doub fac=con2;
			
			for ( Int j=1; j<i; j++ ) {	// compute extrapolations of varios orders, requiring no new function evaluations
				
				a[j][i]=(a[j-1][i]*fac-a[j-1][i-1])/(fac-1.0);
				fac=con2*fac;
				Doub errt=MAX( abs( a[j][i]-a[j-1][i] ), abs( a[j][i]-a[j-1][i-1] ) );
				/*
				the error strategy is to compare each new estrapolation to one order lower,
				* both at the present stepsize and the previos one
				*/
				if ( errt<=err ) {
					err = errt;
					ans=a[j][i];
				}
			
			}
			
			if ( abs( a[i][i]-a[i-1][i-1] ) >= safe * err ) break;
			// if higher order is worse by a significant factor SAFE, then quit early 
			
		}
		
		return ans;
		
	}

};

#endif	// if _derivata_
