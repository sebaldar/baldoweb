#include <sstream>
#include <celestial_body.hpp>
#include <derivata.hpp>
#include <odefunctor.hpp>

int main (int /* argc */, char ** /* argv[] */ )
{

	struct Test : NumDerivatives {
		
		Test () : NumDerivatives() {
		}
		
		Doub func( Doub x ) {
			return x*x*x;
		}
		
	};

	try {
		Test test;
		Doub r = test(2);
		std::cout << r << std::endl;
	} catch ( const char * err ) {
		std::cout << err << std::endl;
	}
	
	return (0);
	
}
