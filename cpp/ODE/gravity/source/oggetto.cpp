#include <oggetto.hpp>

#include <vettore.hpp>
#include <odefunctor.hpp>
#include <celestial_body.hpp>

Oggetto::Oggetto () :
	fx(*this), fy(*this), fz(*this), fd(*this) {
}
	
Oggetto::~Oggetto () {
}

void Oggetto::execute ( const OrbitXYZ &orbit, double jd1, double jd2 ) {
	
	O = orbit;
	JD1 = jd1;
	JD2 = jd2;
	
	x = O.x;
	y = O.y;
	z = O.z;
	
	Earth earth;
	d2 = earth.R;
	
	Math < Funzione, Variabile, double > mat;
	mat.verbose = true;
		
	// genera il vettore delle variabili e quello delle funzioni
	std::vector< Variabile *> v_var;
	std::vector< Funzione * > v_funz;
		
	v_funz.push_back( &fx );
	v_funz.push_back( &fy );
	v_funz.push_back( &fz );
	v_funz.push_back( &fd );
		
	v_var.push_back( &x );
	v_var.push_back( &y );
	v_var.push_back( &z );
	v_var.push_back( &d2 );

	try {
		mat.execute ( v_var, v_funz );
	}
	catch ( const std::string & err ) {
		std::cerr << err << std::endl;
	}
	
}

Oggetto::FX::FX( Oggetto &t ) : Funzione(), obj(t), f(t, FOde::Coordinata::X)
{
}

double Oggetto::FX::eval()
{

	// da giorno giuliano a secondi
	double t1 = obj.JD1 * Date::sec_in_a_day;
	double t2 = obj.JD2 * Date::sec_in_a_day;

	OdeFunctor &ode = f.ode;
	ode.set ( t1, t2 ) ;
		
	ode.ystart[0]= obj.O.x;
	ode.ystart[1]= obj.O.vx;

	ode () ;
    
	double x = ode.out.ysave[0][ode.out.count-1];
//	obj.O.vx = ode.out.ysave[1][ode.out.count-1];

	Earth earth;
	OrbitXYZ o_earth = earth.orbit ( MainBody::earth_data, obj.JD2 ) ;
			
	double x_e = o_earth.x * ( CelestialBody::AU * 1000 );
	double y_e = o_earth.y * ( CelestialBody::AU * 1000 );
	double z_e = o_earth.z * ( CelestialBody::AU * 1000 );
			
	Point center_earth ( x_e, y_e, z_e );

	double x_r = x;
	double y_r = obj.y;
	double z_r = obj.z;
			
	Point rck( x_r, y_r, z_r );

	return obj.d2 - Point::distance( rck, center_earth );

}

Oggetto::FY::FY ( Oggetto &t ) : Funzione(), obj(t), f(t, FOde::Coordinata::Y)
{
}

double Oggetto::FY::eval() {

	// da giorno giuliano a secondi
	double t1 = obj.JD1 * Date::sec_in_a_day;
	double t2 = obj.JD2 * Date::sec_in_a_day;

	OdeFunctor &ode = f.ode;
	ode.set ( t1, t2 ) ;
		
	ode.ystart[0]= obj.O.y;
	ode.ystart[1]= obj.O.vy;

	ode () ;
    
	double y = ode.out.ysave[0][ode.out.count-1];
//	obj.O.vy = ode.out.ysave[1][ode.out.count-1];

	Earth earth;
	OrbitXYZ o_earth = earth.orbit ( MainBody::earth_data, obj.JD2 ) ;
			
	double x_e = o_earth.x * ( CelestialBody::AU * 1000 );
	double y_e = o_earth.y * ( CelestialBody::AU * 1000 );
	double z_e = o_earth.z * ( CelestialBody::AU * 1000 );
			
	Point center_earth ( x_e, y_e, z_e );

	double x_r = obj.x;
	double y_r = y;
	double z_r = obj.z;
			
	Point rck( x_r, y_r, z_r );

	return obj.d2 - Point::distance( rck, center_earth );

}

Oggetto::FZ::FZ( Oggetto &t ) : Funzione(), obj(t) , f(t, FOde::Coordinata::Z)
{
}
		
double Oggetto::FZ::eval() {

	// da giorno giuliano a secondi
	double t1 = obj.JD1 * Date::sec_in_a_day;
	double t2 = obj.JD2 * Date::sec_in_a_day;

	OdeFunctor &ode = f.ode;
	ode.set ( t1, t2 ) ;
		
	ode.ystart[0]= obj.O.z;
	ode.ystart[1]= obj.O.vz;

	ode () ;
    
	double z = ode.out.ysave[0][ode.out.count-1];
//	obj.O.vz = ode.out.ysave[1][ode.out.count-1];

	Earth earth;
	OrbitXYZ o_earth = earth.orbit ( MainBody::earth_data, obj.JD2 ) ;
			
	double x_e = o_earth.x * ( CelestialBody::AU * 1000 );
	double y_e = o_earth.y * ( CelestialBody::AU * 1000 );
	double z_e = o_earth.z * ( CelestialBody::AU * 1000 );
			
	Point center_earth ( x_e, y_e, z_e );

	double x_r = obj.x;
	double y_r = obj.y;
	double z_r = z;
			
	Point rck( x_r, y_r, z_r );

	return obj.d2 - Point::distance( rck, center_earth );

}

Oggetto::FD::FD( Oggetto &t ) : Funzione(), obj(t)
{
}
		
double Oggetto::FD::eval() {

	Earth earth;
	OrbitXYZ o_earth = earth.orbit ( MainBody::earth_data, obj.JD2 ) ;
			
	double x_e = o_earth.x * ( CelestialBody::AU * 1000 );
	double y_e = o_earth.y * ( CelestialBody::AU * 1000 );
	double z_e = o_earth.z * ( CelestialBody::AU * 1000 );
			
	Point center_earth ( x_e, y_e, z_e );

	double x_r = obj.x;
	double y_r = obj.y;
	double z_r = obj.z;
			
	Point rck( x_r, y_r, z_r );

	return obj.d2 - Point::distance( rck, center_earth );

}



FOde::FOde ( class Oggetto & o, Coordinata c) :
	OdeFunction( ),
	obj( o ),
	coordinata(c),
	ode( *this )	{
}
			
void FOde::operator () ( const Doub & t, VecDoub_I & y, VecDoub_O & dydx ) {
			
	double x_r = obj.x;
	double y_r = obj.y;
	double z_r = obj.z;
			
	Earth earth;
	Sun sun;
	Moon moon ;

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

	switch ( coordinata ) {
				
		case X: {
					
			x_r = y[0];
			obj.x = y[0];

			Point rck( x_r, y_r, z_r );
			double RE2 = Point::distance2(rck, center_earth );
			double Ge = CelestialBody::G * Me / ( RE2 );

			double RM2 = Point::distance2(rck, center_moon );
			double Gm = CelestialBody::G * Mm / ( RM2 );

			double RS2 = Point::distance2(rck, center_sun );
			double Gs = CelestialBody::G * Ms / ( RS2 );

			Vettore r_e ( Direction(rck, center_earth), Ge ); 
			Vettore r_m ( Direction(rck, center_moon), Gm ); 
			Vettore r_s ( Direction(rck, center_sun), Gs ); 

			dydx[1] = ( r_e.D.x + r_m.D.x + r_s.D.x );

		}

		break;

		case Y: {
					
			y_r = y[0];
			obj.y = y[0];
					
			Point rck( x_r, y_r, z_r );
			double RE2 = Point::distance2(rck, center_earth );
			double Ge = CelestialBody::G * Me / ( RE2 );

			double RM2 = Point::distance2(rck, center_moon );
			double Gm = CelestialBody::G * Mm / ( RM2 );

			double RS2 = Point::distance2(rck, center_sun );
			double Gs = CelestialBody::G * Ms / ( RS2 );

			Vettore r_e ( Direction(rck, center_earth), Ge ); 
			Vettore r_m ( Direction(rck, center_moon), Gm ); 
			Vettore r_s ( Direction(rck, center_sun), Gs ); 
					

			dydx[1] = ( r_e.D.y + r_m.D.y + r_s.D.y );
				
		}
		break;

		case Z: {
					
			z_r = y[0];
			obj.z = y[0];
				
			Point rck( x_r, y_r, z_r );
			double RE2 = Point::distance2(rck, center_earth );
			double Ge = CelestialBody::G * Me / ( RE2 );

			double RM2 = Point::distance2(rck, center_moon );
			double Gm = CelestialBody::G * Mm / ( RM2 );

			double RS2 = Point::distance2(rck, center_sun );
			double Gs = CelestialBody::G * Ms / ( RS2 );

			Vettore r_e ( Direction(rck, center_earth), Ge ); 
			Vettore r_m ( Direction(rck, center_moon), Gm ); 
			Vettore r_s ( Direction(rck, center_sun), Gs ); 

			dydx[1] = ( r_e.D.z + r_m.D.z + r_s.D.z );
		}
				
		break;
			
	}
			
	dydx[0] = y[1];
		
}

