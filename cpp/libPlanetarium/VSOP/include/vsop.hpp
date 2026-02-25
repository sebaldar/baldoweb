#ifndef _vsop_
#define _vsop_

#include <iostream>
#include <vector>

#include <orbit.hpp>


class VSOP {


    protected :

	struct Var {

		unsigned short n = 0;

		std::vector < double > A, B, C;

	} ;

	public:

		Var X[6], Y[6], Z[6];

		VSOP ( const std::string & nome_file );

		OrbitXYZ getOrbit ( double JD ) const ;
		OrbitEcli getOrbitLBR ( double JD ) const ;


};


#endif


