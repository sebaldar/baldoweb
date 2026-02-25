#include <math.h>       /* pow */

#include <vsop.hpp>
#include <solarSystem.hpp>
#include <celestial_body.hpp>
#include <date.hpp>

#include <line.hpp>
#include <plane.hpp>
#include <horizont.hpp>

int main (int /* argc */, char ** /* argv[] */ )
{

	
	Date d ;
	SolarSystem solar( d.now() ) ;
	Orbit o_mercury = solar.orbit( "mercury" );
	Orbit o_venus = solar.orbit( "venus" );
	Orbit o_earth = solar.orbit( "earth" );
	Orbit o_mars = solar.orbit( "mars" );
	Orbit o_juppiter = solar.orbit( "juppiter" );
	Orbit o_saturn = solar.orbit( "saturn" );
	Orbit o_uranus = solar.orbit( "uranus" );
	Orbit o_neptune = solar.orbit( "neptune" );
	
	double x = o_mercury.x ;
	double y = o_mercury.y ;
	double z = o_mercury.z ;

	double r_mercury = pow( x * x + y * y + z * z, 0.5 );

	x = o_venus.x ;
	y = o_venus.y ;
	z = o_venus.z ;

	double r_venus = pow( x * x + y * y + z * z, 0.5 );
	
	x = o_earth.x ;
	y = o_earth.y ;
	z = o_earth.z ;

	double r_earth = pow( x * x + y * y + z * z, 0.5 );

	x = o_mars.x ;
	y = o_mars.y ;
	z = o_mars.z ;

	double r_mars = pow( x * x + y * y + z * z, 0.5 );

	x = o_juppiter.x ;
	y = o_juppiter.y ;
	z = o_juppiter.z ;

	double r_juppiter = pow( x * x + y * y + z * z, 0.5 );

	x = o_saturn.x ;
	y = o_saturn.y ;
	z = o_saturn.z ;

	double r_saturn = pow( x * x + y * y + z * z, 0.5 );

	x = o_uranus.x ;
	y = o_uranus.y ;
	z = o_uranus.z ;

	double r_uranus = pow( x * x + y * y + z * z, 0.5 );

	x = o_neptune.x ;
	y = o_neptune.y ;
	z = o_neptune.z ;

	double r_neptune = pow( x * x + y * y + z * z, 0.5 );

	std::cout << "r_mercury = " << r_mercury << std::endl;
	std::cout << "r_venus = " << r_venus << std::endl;
	std::cout << "r_earth = " << r_earth << std::endl;
	std::cout << "r_mars = " << r_mars << std::endl;
	std::cout << "r_juppiter = " << r_juppiter << std::endl;
	std::cout << "r_saturn = " << r_saturn << std::endl;
	std::cout << "r_uranus = " << r_uranus << std::endl;
	std::cout << "r_neptune = " << r_neptune << std::endl;



/*
	double vx = orbit.vx / 365250 ;
	double vy = orbit.vy / 365250 ;
	double vz = orbit.vz / 365250 ;

	std::cout << "x = " << x << std::endl;
	std::cout << "y = " << y << std::endl;
	std::cout << "z = " << z << std::endl;

	std::cout << "vx = " << vx  << std::endl;
	std::cout << "vy = " << vy  << std::endl;
	std::cout << "vz = " << vz  << std::endl;
	
	double v = pow( vx * vx + vy * vy + vz * vz, 0.5 );

	std::cout << "r = " << r << std::endl;
	std::cout << "v = " << v << std::endl;
	MainBody earth ( "earth", nullptr, & SolarSystem::earth);
	Orbit orbit = earth.orbit( d.now() );
	
	SolarSystem solar( d.now() ) ;
	solar.add( & earth );
*/	
	return (0);

	

	
}
