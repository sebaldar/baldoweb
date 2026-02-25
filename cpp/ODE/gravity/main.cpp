#include <derivata.hpp>
#include <integrale.hpp>
#include <celestial_body.hpp>

#include <rocket.hpp>
#include <oggetto.hpp>


#include <vettore.hpp>

struct Integrale : NumIntegrale {


		Integrale ( ) : NumIntegrale( ) {
		}

		Doub func( Doub x ) {

			return x*x;

		}

};


struct VEarth : NumDerivatives {

		Earth earth;

		enum {X, Y, Z} coordinata=X;

		VEarth ( ) : NumDerivatives( 10 / Date::sec_in_a_day ) {
		}

		Doub func( Doub JD ) {

			OrbitXYZ orbit = earth.orbit ( MainBody::earth_data, JD ) ;

			switch ( coordinata ) {
				case X:
				default:
					return orbit.x ;
				case Y :
					return orbit.y ;
				case Z :
					return orbit.z ;

			}

		}

};

struct EarthCoordinate : NumDerivatives {

		Earth earth;

		enum {X, Y, Z} coordinata=X;
		double latitude =0;
		double longitude =0;

		EarthCoordinate ( ) : NumDerivatives( 1 ) {
		}

		Doub func( Doub JD ) {

			OrbitXYZ orbit = earth.orbit ( MainBody::earth_data, JD ) ;
			double rot = earth.rotation ( JD ) ;

			Point coord = earth.bodyCoord ( latitude, longitude + rot );

			switch ( coordinata ) {
				case X:
				default:
					return orbit.x + coord.x / CelestialBody::AU;
				case Y :
					return orbit.y + coord.y / CelestialBody::AU;
				case Z :
					return orbit.z + coord.z / CelestialBody::AU;

			}

		}

};


int main (int /* argc */, char ** /* argv[] */ )
{

		Integrale itg ;
		Doub res = itg( 0, 1);
		std::cout << res << std::endl;

	std::stringstream result;
	result.precision(25);

	Date date;
	JDay JD = date.julianDay();

	double sec = 1;
	double s = sec / Date::sec_in_a_day	;
	Date d1 (JD+s);

	std::cout.precision(8);
	std::cout << date.toDate() << std::endl;
	std::cout << d1.toDate() << std::endl;
	std::cout << JD << std::endl;


	Earth earth;
	OrbitXYZ orbit = earth.orbit ( MainBody::earth_data, JD ) ;

	double x = orbit.x ;
	double y = orbit.y ;
	double z = orbit.z ;

	double vtx = orbit.vx ;
	double vty = orbit.vy ;
	double vtz = orbit.vz ;

	VEarth v_earth;

	v_earth.coordinata =VEarth::X;
	double vxca = v_earth(JD);
	v_earth.coordinata =VEarth::Y;
	double vyca = v_earth(JD);
	v_earth.coordinata =VEarth::Z;
	double vzca = v_earth(JD);

	result <<
		"velocità terra vx=" << orbit.vx << " vy=" << orbit.vy <<  " vz=" << orbit.vz << std::endl <<
		"calcolata cx=" << vxca << " cy=" << vyca <<  " cz=" << vzca << std::endl <<
		std::endl << std::endl;

	double rot = earth.rotation ( JD ) ;
	double latitude =45.167;
	double longitude =9.167;
	Point coord = earth.bodyCoord ( latitude, longitude + rot );

	double xc = x + coord.x / CelestialBody::AU;
	double yc = y + coord.y / CelestialBody::AU;
	double zc = z + coord.z / CelestialBody::AU;

	EarthCoordinate v_periferica;
	v_periferica.latitude=latitude;
	v_periferica.longitude=longitude;

	v_periferica.coordinata =EarthCoordinate::X;
	double vxc = v_periferica(JD);
	v_periferica.coordinata =EarthCoordinate::Y;
	double vyc = v_periferica(JD);
	v_periferica.coordinata =EarthCoordinate::Z;
	double vzc = v_periferica(JD);

	Rocket rocket;

	rocket.orbit.x = xc * ( CelestialBody::AU * 1000 );
	rocket.orbit.y = yc * ( CelestialBody::AU * 1000 );
	rocket.orbit.z = zc * ( CelestialBody::AU * 1000 );

	rocket.orbit.vx = vxc * ( CelestialBody::AU * 1000 ) / Date::sec_in_a_day;
	rocket.orbit.vy = vyc * ( CelestialBody::AU * 1000 ) / Date::sec_in_a_day;
	rocket.orbit.vz = vzc * ( CelestialBody::AU * 1000 ) / Date::sec_in_a_day;
/*
	Oggetto obj;
	obj.execute(rocket.orbit, JD, JD+s );
	double d2 = obj.d2;
	std::cout << "************ " << pow(d2, 0.5) << std::endl;


	rocket.orbit.vx = vtx * ( CelestialBody::AU * 1000 ) / Date::sec_in_a_day;
	rocket.orbit.vy = vty * ( CelestialBody::AU * 1000 ) / Date::sec_in_a_day;
	rocket.orbit.vz = vtz * ( CelestialBody::AU * 1000 ) / Date::sec_in_a_day;
*/


	double xt = x * ( 1000 * CelestialBody::AU );
	double yt = y * ( 1000 * CelestialBody::AU );
	double zt = z * ( 1000 * CelestialBody::AU );

	Point c1t( xt, yt, zt);
	Point c1r( rocket.orbit.x, rocket.orbit.y, rocket.orbit.z);

	double D = Point::distance( c1r, c1t );

	result <<
		"Distanza centro terra " << D << std::endl <<
		"Raggio terra " << earth.radius << std::endl <<
		"posizione rocket x=" << xc << " y=" << yc <<  " z=" << zc << std::endl <<
		"pos rocket m xr=" << rocket.orbit.x << " yr=" << rocket.orbit.y <<  " zr=" << rocket.orbit.z << std::endl <<
		"velocità terra vtx=" << vtx << " vty=" << vty <<  " vtz=" << vtz << std::endl <<
		"velocità rocket ua/day vxc=" << vxc << " vyc=" << vyc <<  " vzc=" << vzc << std::endl <<
		"velocità rocket m/s vx=" << rocket.orbit.vx << " vy=" << rocket.orbit.vy <<  " vz=" << rocket.orbit.vz << std::endl <<
		std::endl;

	rocket( JD, JD+s );

	result <<
		"posizione finale rocket m xr=" << rocket.orbit.x << " yr=" << rocket.orbit.y <<  " zr=" << rocket.orbit.z << std::endl <<
		"velocità finale rocket m/s vx=" << rocket.orbit.vx << " vy=" << rocket.orbit.vy <<  " vz=" << rocket.orbit.vz << std::endl <<
		std::endl;


	OrbitXYZ o2 = earth.orbit ( MainBody::earth_data, JD+s ) ;
	Point c2t( o2.x*( 1000 * CelestialBody::AU ),o2.y*( 1000 * CelestialBody::AU ),o2.z*( 1000 * CelestialBody::AU ));
	Point c2r( rocket.orbit.x, rocket.orbit.y, rocket.orbit.z);
	double D2 = Point::distance( c2t, c2r );

	result << "Distanza centro terra " << D2 << std::endl;
	result << "Spostamento relativo alla terra " << D2-D << std::endl;
	result << "Spostamento terra " << Point::distance(c1t, c2t) << std::endl;
	result << "Spostamento rocket " << Point::distance(c1r, c2r) << std::endl;
	result << "Spostamento rocket teorico " << -(sec*sec/2)*CelestialBody::G * earth.M / ( D2 * D2 ) << std::endl;
	result << "Gravità " << CelestialBody::G * earth.M / ( D2 * D2 );

	std::cout << result.str() << std::endl;

	Vettore v1( Direction(1,1,0), 1);
	Vettore v2( Direction(-1,-1,0), 1);

	Vettore v=v1+v2;
	std::cout << "v1=" << v1.M << std::endl;
	std::cout << "v2=" << v2.M << std::endl;
	std::cout << "v=" << v.M << std::endl;

	return (0);

}
