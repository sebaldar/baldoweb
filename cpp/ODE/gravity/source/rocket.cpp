#include <thread>

#include <propulsore.hpp>
#include <rocket.hpp>

void Rocket::Function::operator () ( const Doub & t, VecDoub_I & y, VecDoub_O & dydx ) {

	double x_r = orbit.x;
	double y_r = orbit.y;
	double z_r = orbit.z;

	Earth earth;
	Sun sun;
	Moon moon ;
	Jupiter jupiter;
	Saturn saturn;

	Doub jd = t / Date::sec_in_a_day;
	OrbitXYZ o_earth = earth.orbit ( MainBody::earth_data, jd ) ;

	double Me = earth.M;

	double x_e = o_earth.x * ( CelestialBody::AU * 1000 );
	double y_e = o_earth.y * ( CelestialBody::AU * 1000 );
	double z_e = o_earth.z * ( CelestialBody::AU * 1000 );

	Point center_earth ( x_e, y_e, z_e );

	// i dati della teoria ELP
	OrbitEcli orbit_moon = moon.orbitAlfaELP( jd );

	// converte in coordinate rettangolari
	OrbitXYZ o_moon_xyz = orbit_moon;

	double x_m = ( o_earth.x + o_moon_xyz.x ) * ( CelestialBody::AU * 1000 );
    double y_m = ( o_earth.y + o_moon_xyz.y ) * ( CelestialBody::AU * 1000 );
	double z_m = ( o_earth.z + o_moon_xyz.z ) * ( CelestialBody::AU * 1000 );

	double Mm = moon.M;
	Point center_moon ( x_m, y_m, z_m );

	double Ms = sun.M;
	double x_s = 0;
	double y_s = 0;
	double z_s = 0;
	Point center_sun ( x_s, y_s, z_s );	// tutto eliocentrico

	double Mj = jupiter.M;
	OrbitXYZ o_jupiter = jupiter.orbit ( MainBody::jupiter_data, jd ) ;
	double x_j = o_jupiter.x * ( CelestialBody::AU * 1000 );
	double y_j = o_jupiter.y * ( CelestialBody::AU * 1000 );
	double z_j = o_jupiter.z * ( CelestialBody::AU * 1000 );
	Point center_jupiter ( x_j, y_j, z_j );

	double Mst = saturn.M;
	OrbitXYZ o_saturn = saturn.orbit ( MainBody::saturn_data, jd ) ;
    double x_st = o_saturn.x * ( CelestialBody::AU * 1000 );
	double y_st = o_saturn.y * ( CelestialBody::AU * 1000 );
	double z_st = o_saturn.z * ( CelestialBody::AU * 1000 );
	Point center_saturn ( x_st, y_st, z_st );

	switch ( coordinata ) {

		case X: {

			x_r = y[0];

			Point rck( x_r, y_r, z_r );
			double RE2 = Point::distance2(rck, center_earth );
			double Ge = G * Me / ( RE2 );

			double RM2 = Point::distance2(rck, center_moon );
			double Gm = G * Mm / ( RM2 );

			double RS2 = Point::distance2(rck, center_sun );
			double Gs = G * Ms / ( RS2 );

			double RJ2 = Point::distance2(rck, center_jupiter );
            double Gj = G * Mj / ( RJ2 );

			double RST2 = Point::distance2(rck, center_saturn );
			double Gst = G * Mst / ( RST2 );

			Vettore r_e ( Direction(rck, center_earth), Ge );
			Vettore r_m ( Direction(rck, center_moon), Gm );
			Vettore r_s ( Direction(rck, center_sun), Gs );
			Vettore r_j ( Direction(rck, center_jupiter), Gj );
			Vettore r_st ( Direction(rck, center_saturn), Gst );

			dydx[1] = ( r_e.D.x + r_m.D.x + r_s.D.x + r_j.D.x + r_st.D.x );

        }

		break;
		case Y: {

			y_r = y[0];

			Point rck( x_r, y_r, z_r );
			double RE2 = Point::distance2(rck, center_earth );
			double Ge = G * Me / ( RE2 );

			double RM2 = Point::distance2(rck, center_moon );
			double Gm = G * Mm / ( RM2 );

			double RS2 = Point::distance2(rck, center_sun );
			double Gs = G * Ms / ( RS2 );
			double RJ2 = Point::distance2(rck, center_jupiter );
			double Gj = G * Mj / ( RJ2 );

			double RST2 = Point::distance2(rck, center_saturn );
			double Gst = G * Mst / ( RST2 );

			Vettore r_e ( Direction(rck, center_earth), Ge );
			Vettore r_m ( Direction(rck, center_moon), Gm );
			Vettore r_s ( Direction(rck, center_sun), Gs );
			Vettore r_j ( Direction(rck, center_jupiter), Gj );
			Vettore r_st ( Direction(rck, center_saturn), Gst );

			dydx[1] = ( r_e.D.y + r_m.D.y + r_s.D.y + r_j.D.y + r_st.D.y );

		}
		break;

		case Z: {

			z_r = y[0];

			Point rck( x_r, y_r, z_r );
			double RE2 = Point::distance2(rck, center_earth );
			double Ge = G * Me / ( RE2 );

			double RM2 = Point::distance2(rck, center_moon );
			double Gm = G * Mm / ( RM2 );

			double RS2 = Point::distance2(rck, center_sun );
			double Gs = G * Ms / ( RS2 );

			double RJ2 = Point::distance2(rck, center_jupiter );
			double Gj = G * Mj / ( RJ2 );

			double RST2 = Point::distance2(rck, center_saturn );
			double Gst = G * Mst / ( RST2 );

			Vettore r_e ( Direction(rck, center_earth), Ge );
			Vettore r_m ( Direction(rck, center_moon), Gm );
			Vettore r_s ( Direction(rck, center_sun), Gs );
			Vettore r_j ( Direction(rck, center_jupiter), Gj );
			Vettore r_st ( Direction(rck, center_saturn), Gst );

			dydx[1] = ( r_e.D.z + r_m.D.z + r_s.D.z + r_j.D.z + r_st.D.z );

		}

		break;

	}

	dydx[0] = y[1];

}

Rocket::Rocket() :
	dx(orbit, Function::Coordinata::X), dy(orbit, Function::Coordinata::Y), dz(orbit, Function::Coordinata::Z)
{
}

Rocket::~Rocket()
{
	if ( stadio1 )
		delete stadio1;
	if ( stadio2 )
		delete stadio2;
	if ( stadio3 )
		delete stadio3;
}

void Rocket::operator () ( double jd1, double jd2 )
{

	// da giorno giuliano a secondi
	double t1 = jd1 * Date::sec_in_a_day;
	double t2 = jd2 * Date::sec_in_a_day;


	OdeFunctor odex ( t1, t2, 2, dx ) ;
	OdeFunctor odey ( t1, t2, 2, dy ) ;
	OdeFunctor odez ( t1, t2, 2, dz ) ;

	odex.ystart[0]= orbit.x;
	odex.ystart[1]= orbit.vx;

	odey.ystart[0]= orbit.y;
	odey.ystart[1]= orbit.vy;

	odez.ystart[0]= orbit.z;
	odez.ystart[1]= orbit.vz;

	odex () ;

	odey () ;

	odez () ;


	orbit.x = odex.out.ysave[0][odex.out.count-1];
	orbit.vx = odex.out.ysave[1][odex.out.count-1];

	orbit.y = odey.out.ysave[0][odey.out.count-1];
	orbit.vy = odey.out.ysave[1][odey.out.count-1];

	orbit.z = odez.out.ysave[0][odez.out.count-1];
	orbit.vz = odez.out.ysave[1][odez.out.count-1];

}

