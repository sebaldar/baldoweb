#include <iostream>
#include <vector>

std::vector< double > elp82b( double js );


int main (int /* argc */, char ** /* argv[] */ )
{

	double AU = 149597870.700 ;
	
	std::vector< double > xyz;
	xyz = elp82b ( 2389000.5 ) ;
	
	std::cout.precision(12);
	std::cout << AU * xyz[0] << " " << AU * xyz[1] << " " << AU * xyz[2] << std::endl;

	return (0);

	
}
