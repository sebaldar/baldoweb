#include <limits>

#include <date.hpp>
typedef std::numeric_limits< double > dbl;

int main (int /* argc */, char ** /* argv[] */ )
{
	
	Date d, d1("19-06-1987 12:00:00"), d2(1842713.0); ;
	
	std::cout.precision( dbl::max_digits10 );
 	std::cout << d.julianDay () << std::endl ;
 	std::cout << d1.julianDay ()  << std::endl ;
 	std::cout << d2.julianDay ()  << std::endl ;
 	std::cout << d2.toDate()  << std::endl ;
 	
 	d += 1 ;
 	std::cout << d.toDate() << std::endl ;
	
//	for ( long long i=0; i < 1e10; ++i );
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	std::cout << d.toDate() << std::endl ;
	
	TJD jd ("1-1-2000 12:0:0");
	std::cout << jd << std::endl;
	
	return (0);

	
}
