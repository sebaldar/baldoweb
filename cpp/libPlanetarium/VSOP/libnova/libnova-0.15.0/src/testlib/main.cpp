#include <iostream>

#include <date.hpp>

struct OrbitLBR {

	double L = 0, B = 0, R = 0;

};

OrbitLBR nova_lunar ( double JD  );

int main (int /* argc */, char ** /* argv[] */ )
{
	
	TJD jd ( "31-7-2003 12:0:0" );
	
	OrbitLBR  orbit = nova_lunar ( jd );
	std::cout.precision(10);
	std::cout << orbit.L << " " << orbit.B << " " << orbit.R << std::endl;
	return (0);

	
}
