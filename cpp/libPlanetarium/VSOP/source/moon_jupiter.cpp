
#include <moon_jupiter.hpp>

MoonJupiter::MoonJupiter ( const std::string & _name, CelestialBody * _parent ) :
	CelestialBody( _name, _parent )
{
	gl.shader = "lambert";
}

MoonJupiter::~MoonJupiter() {
}

void MoonJupiter::commonTerms ( double t )
{
	
	l1 = 106.07719 + 203.488955790 * t ;
	l2 = 175.73161 + 101.374724735 * t ;
	l3 = 120.558883 + 50.317609207 * t ;
	l4 = 84.44459 + 21.571071177 * t ;
	
	pi1 = 97.0881 + 0.16138586 * t ;
	pi2 = 154.8663 + 0.04726307 * t ;
	pi3 = 188.1840 + 0.00712734 * t ;
	pi4 = 335.2868 + 0.00184000 * t ;
	
	omega1 = 312.3346 - 0.13279386 * t ;
	omega2 = 100.4411 - 0.03263064 * t ;
	omega3 = 119.1942 - 0.00717703 * t ;
	omega4 = 322.6186 - 0.00175934 * t ;
	
	Grade a1 = 163.679 + 0.0010512 * t ;
	Grade a2 = 34.486 - 0.0161731 * t  ;
	
	Lambda = 0.33033 * sin( Radiant( a1 ) ) + 0.03439 * sin( Radiant( a2 ) );

	PhiA = 199.6766 + 0.17379190 * t ;
	
	Psi = 316.5182 - 0.00000208 * t ;
	
	G = 30.23756 + 0.0830925701 * t + Lambda ;
	G1 = 31.97853 + 0.0334597339 * t ;

}


	
void rotate_vector( const double angle, double & x, double & y)
{
   
   const double sin_angle = sin( angle);
   const double cos_angle = cos( angle);
   const double temp = cos_angle * x - sin_angle * y;

   y = sin_angle * x + cos_angle * y;
   x = temp;

}

OrbitXYZ MoonJupiter::orbit ( double JDE ) 
{
		
	double tau = 0 ;
	double t = JDE - 2443000.5 - tau ;
		
	commonTerms ( t ) ;
	periodicTerm ( ) ;
		
	double T0 = ( JDE - 2433282.423 ) / 36525 ;
		
	double P = 1.3966626 * T0 + 0.0003088 * T0 * T0 ;
		
	L += P ;
	Psi += P ;
		
	Radiant l = Grade( L - Psi ) ;
	Radiant b =  Grade( B ) ;
		
	double X = R * cos( l ) * cos ( b );
	double Y = R * sin( l ) * cos( b );
	double Z = R * sin( b );
		
	Jupiter & jupiter = dynamic_cast < Jupiter & > ( *parent ) ;
	double r_jupiter = jupiter.radius / CelestialBody::AU ;
	Grade jup_obliquity = jupiter.obliquity( JDE );
//	rotate_vector ( Radiant( jup_obliquity ), Y, Z ) ;
		
	return OrbitXYZ ( r_jupiter * X, r_jupiter * Y, r_jupiter * Z ) ;
/*		
	Radiant a = atan2( jupiter.position.y, jupiter.position.x )  ;
//	Radiant a = atan( jupiter.position.y / jupiter.position.x  )  ;
	X = X  * sin( a ) ;
	Y = Y  * cos( a ) ;
	
//	Radiant a = atan2( jupiter.position.y, jupiter.position.x )  ;
	Radiant a = atan( jupiter.position.y / jupiter.position.x  )  ;
	X = X  * sin( a ) ;
	Y = Y  * cos( a ) ;
//	Radiant a = Radiant( Psi + P ) - CelestialBody::pi / 2 - atan2( jupiter.position.y, jupiter.position.x )  ;
//	rotate_vector( a,  X, Y );
		double tau = 0 ;
		double t = JDE - 2443000.5 - tau ;
		
		commonTerms ( t ) ;
		periodicTerm ( t ) ;
		
		double T0 = ( JDE - 2433282.423 ) / 36525 ;
		
		double P = 1.3966626 * T0 + 0.0003088 * T0 * T0 ;
		
		L += P ;
		Psi += P ;
		
		Radiant l = Grade( L - Psi ) ;
		Radiant b = Grade ( B ) ;
		
		double X = R * cos( l ) * cos ( b );
		double Y = R * sin( l ) * cos( b );
		double Z = R * sin( b );
		
		// trasforma le coordinate eclittiche in equatoriali
		double epsilon = Radiant( Grade( 23 ) );

        // Longitude of Jupiter's ascending node;  p. 213  (table 31A)
const double  J2000  = 2451545.0;
const double   J1900 = ( 2451545. - 36525.);
const double dt = (JDE - J2000) / 36525.0;
double asc_node = 100.464407 + 1.0209774 + dt + 0.00040315 * dt * dt + 4.04e-7 * dt * dt * dt ;
double incl_orbit = 1.303267 -.0054965 * dt + 4.66e-6  * dt * dt -2.e-9 * dt * dt * dt;
double incl = 3.120262 + 0.0006 * ( ( JDE - J1900) / 36525.0);

rotate_vector ( incl, &Y, &Z ) ;
rotate_vector ( Psi + P - asc_node, &X, &Y ) ;
rotate_vector( incl_orbit, &Y, &Z );
rotate_vector( asc_node,  &X, &Y );

		double tan_alfa = ( sin ( l ) * cos ( epsilon ) - tan( b ) * sin ( epsilon ) ) / cos ( l );
		double sin_delta = sin ( b ) * cos ( epsilon ) + cos( b ) * sin( epsilon ) * sin( l ) ;

		Radiant alfa = atan( tan_alfa ) ;
		Radiant delta = asin( sin_delta ) ;

		double X1 = R * cos( alfa ) * cos ( delta );
		double Y1 = R * sin( alfa ) * cos( delta );
		double Z1 = R * sin( b );
		
		Jupiter jupiter;
		double r_jupiter = jupiter.radius / CelestialBody::AU ;
		
		return OrbitXYZ ( r_jupiter * X, r_jupiter * Y, r_jupiter * Z ) ;
		
*/		
}

Io::Io ( ) :
	MoonJupiter( "io", nullptr )
{

    D = 3660; // km
    radius = D / 2;
	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	gl.map = "/my_image/default/io.jpg";


}

Io::~Io() {
}

void Io::periodicTerm (  )
{

	// longitudine
	Radiant a01 = Grade ( 2 * ( l1 - l2 ) );
	Radiant a02 = Grade ( pi3 - pi4 );
	Radiant a03 = Grade ( l2 - 2 * l3 + pi3 ); 
	Radiant a04 = Grade ( PhiA );
	Radiant a05 = Grade ( l2 - 2 * l3 + pi2 );
	Radiant a06 = Grade ( pi1 + pi3 - 2 * Pi - 2 * G  );
	Radiant a07 = Grade ( l2 - 2 * l3 + pi4 ); 
	Radiant a08 = Grade ( l1 - pi3 ); 
	Radiant a09 = Grade ( l1 - l2 );
	Radiant a10 = Grade ( 2 * Psi - 2 * Pi );
	Radiant a11 = Grade ( l1 - pi4 ); 
	Radiant a12 = Grade ( G ); 
	Radiant a13 = Grade ( pi2 - pi3 ); 
	Radiant a14 = Grade ( 4 * ( l1 - l2 ) );
	Radiant a15 = Grade ( l1 - l3 );
	Radiant a16 = Grade ( Psi + omega3 - 2 * Pi - 2 * G );
	Radiant a17 = Grade ( 2 * ( l1 - 2 * l2 + omega2 ) );
	Radiant a18 = Grade ( pi2 - pi4 );
	Radiant a19 = Grade ( l1 + pi3 - 2 * Pi - 2 * G );
	Radiant a20 = Grade ( omega2 - omega3 );
	Radiant a21 = Grade ( Psi - omega2 );
	
	double	S = 
						+ 0.47259 * sin( a01 )
						 - 0.03478 * sin( a02 )
						 + 0.01081 * sin( a03 )
						 + 0.00738 * sin( a04 )
						 + 0.00713 * sin( a05 )
						 - 0.00674 * sin( a06 )
						 + 0.00666 * sin( a07 )
						 + 0.00445 * sin( a08 )
						 - 0.00354 * sin( a09 )
						 - 0.00317 * sin( a10 )
						 + 0.00265 * sin( a11 )
						 - 0.00186 * sin( a12 )
						 + 0.00162 * sin( a13 )
						 + 0.00158 * sin( a14 )
						 - 0.00155 * sin( a15 )
						 - 0.00138 * sin( a16 )
						 - 0.00115 * sin( a17 )
						 + 0.00089 * sin( a18 )
						 + 0.00085 * sin( a19 )
						 + 0.00083 * sin( a20 )
						 + 0.00053 * sin( a21 );

	L = l1 + S;


	// latitude
	Radiant b01 = Grade ( L - omega1 );
	Radiant b02 = Grade ( L - omega2 );
	Radiant b03 = Grade (  L - omega3  ); 
	Radiant b04 = Grade ( L - Psi );
	Radiant b05 = Grade (  L - omega4  );
	Radiant b06 = Grade ( 3.0 * L - 4.0 * l2 - 1.9927 * S + omega2  );
	Radiant b07 = Grade ( L + Psi - 2.0 * Pi - 2.0 * G ); 
					
	B = atan(
						 + 0.0006393 * sin( b01 )
						 + 0.0001825 * sin( b02 )
						 + 0.0000329 * sin( b03 )
						 - 0.0000311 * sin( b04 )
						 + 0.0000093 * sin( b05 )
						 + 0.0000075 * sin( b06 )
						 + 0.0000046 * sin( b07 )
	);
	
	// raggio vettore
	Radiant r01 = Grade ( 2.0 * (l1 - l2) );
	Radiant r02 = Grade ( l1 - pi3 );
	Radiant r03 = Grade (  l1 - pi4  ); 
	Radiant r04 = Grade ( l1 - l2 );
	Radiant r05 = Grade (  4.0 * (l1 - l2)  );
	Radiant r06 = Grade ( l1 - l3  );
	Radiant r07 = Grade ( l1 + pi3 - 2.0 * Pi - 2.0 * G ); 

	R = 5.90569 * (1.0
						 - 0.0041339 * cos( r01 )
						 - 0.0000387 * cos( r02 )
						 - 0.0000214 * cos( r03 )
						 + 0.0000170 * cos( r04 )
						 - 0.0000131 * cos( r05 )
						 + 0.0000106 * cos( r06 )
						 - 0.0000066 * cos( r07 )
	);
						 
}

Europa::Europa ( ) :
	MoonJupiter( "europa", nullptr )
{

    D = 3121.6; // km
    radius = D / 2;
	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	gl.map = "/my_image/default/europa.jpg";


}

Europa::~Europa() {
}

void Europa::periodicTerm ( )
{
	
	// longitudine

	double S = 	
						+ 1.06476 * sin_deg( 2.0 * (l2 - l3) )
						 + 0.04256 * sin_deg( l1 - 2.0 * l2 + pi3 )
						 + 0.03581 * sin_deg( l2 - pi3 )
						 + 0.02395 * sin_deg( l1 - 2.0 * l2 + pi4 )
						 + 0.01984 * sin_deg( l2 - pi4 )
						 - 0.01778 * sin_deg( PhiA )
						 + 0.01654 * sin_deg( l2 - pi2 )
						 + 0.01334 * sin_deg( l2 - 2.0 * l3 + pi2 )
						 + 0.01294 * sin_deg( pi3 - pi4 )
						 - 0.01142 * sin_deg( l2 - l3 )
						 - 0.01057 * sin_deg( G )
						 - 0.00775 * sin_deg( 2.0 * (Psi - Pi) )
						 + 0.00524 * sin_deg( 2.0 * (l1 - l2) )
						 - 0.00460 * sin_deg( l1 - l3 )
						 + 0.00316 * sin_deg( Psi - 2.0 * G + omega3 - 2.0 * Pi )
						 - 0.00203 * sin_deg( pi1 + pi3 - 2.0 * Pi - 2.0 * G )
						 + 0.00146 * sin_deg( Psi - omega3 )
						 - 0.00145 * sin_deg( 2.0 * G )
						 + 0.00125 * sin_deg( Psi - omega4 )
						 - 0.00115 * sin_deg( l1 - 2.0 * l3 + pi3 )
						 - 0.00094 * sin_deg( 2.0 * (l2 - omega2) )
						 + 0.00086 * sin_deg( 2.0 * (l1 - 2.0 * l2 + omega2) )
						 - 0.00086 * sin_deg( 5.0 * G1 - 2.0 * G + 52.225 )
						 - 0.00078 * sin_deg( l2 - l4 )
						 - 0.00064 * sin_deg( 3.0 * l3 - 7.0 * l4 + 4.0 * pi4 )
						 + 0.00064 * sin_deg( pi1 - pi4 )
						 - 0.00063 * sin_deg( l1 - 2.0 * l3 + pi4 )
						 + 0.00058 * sin_deg( omega3 - omega4 )
						 + 0.00056 * sin_deg( 2.0 * (Psi - Pi - G) )
						 + 0.00056 * sin_deg( 2.0 * (l2 - l4) )
						 + 0.00055 * sin_deg( 2.0 * (l1 - l3) )
						 + 0.00052 * sin_deg( 3.0 * l3 - 7.0 * l4 + pi3 + 3.0 * pi4 )
						 - 0.00043 * sin_deg( l1 - pi3 )
						 + 0.00041 * sin_deg( 5.0 * (l2 - l3) )
						 + 0.00041 * sin_deg(pi4 - Pi )
						 + 0.00032 * sin_deg( omega2 - omega3 )
						 + 0.00032 * sin_deg( 2.0 * (l3 - G - Pi) );



	L = l2 + S;


	// latitude
	B =  atan(
						 + 0.0081004 * sin_deg( L - omega2 )
						 + 0.0004512 * sin_deg( L - omega3 )
						 - 0.0003284 * sin_deg( L - Psi )
						 + 0.0001160 * sin_deg( L - omega4 )
						 + 0.0000272 * sin_deg( l1 - 2.0 * l3 + 1.0146 * S + omega2 )
						 - 0.0000144 * sin_deg( L - omega1 )
						 + 0.0000143 * sin_deg( L + Psi - 2.0 * Pi - 2.0 * G )
						 + 0.0000035 * sin_deg( L - Psi + G )
						 - 0.0000028 * sin_deg( l1 - 2.0 * l3 + 1.0146 * S + omega3 )
	);

	
	// raggio vettore
	R = 9.39657 * (1.0
						 + 0.0093848 * cos_deg( l1 - l2 )
						 - 0.0003116 * cos_deg( l2 - pi3 )
						 - 0.0001744 * cos_deg( l2 - pi4 )
						 - 0.0001442 * cos_deg( l2 - pi2 )
						 + 0.0000553 * cos_deg( l2 - l3 )
						 + 0.0000523 * cos_deg( l1 - l3 )
						 - 0.0000290 * cos_deg( 2.0 * (l1 - l2) )
						 + 0.0000164 * cos_deg( 2.0 * (l2 - omega2) )
						 + 0.0000107 * cos_deg( l1 - 2.0 * l3 + pi3 )
						 - 0.0000102 * cos_deg( l2 - pi1 )
						 - 0.0000091 * cos_deg( 2.0 * (l1 - l3) )
					);

						 
}





Ganimede::Ganimede ( ) :
	MoonJupiter( "ganimede", nullptr )
{

    D = 5262.4; // km
    radius = D / 2;
	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km

	gl.map = "/my_image/default/ganymede.jpg";


}

Ganimede::~Ganimede() {
}

void Ganimede::periodicTerm ( )
{

	// longitudine
	double S = + 0.16490 * sin_deg( l3 - pi3 )
						 + 0.09081 * sin_deg( l3 - pi4 )
						 - 0.06907 * sin_deg( l2 - l3 )
						 + 0.03784 * sin_deg( pi3 - pi4 )
						 + 0.01846 * sin_deg( 2.0 * (l3 - l4) )
						 - 0.01340 * sin_deg( G )
						 - 0.01014 * sin_deg( 2.0 * (Psi - Pi) )
						 + 0.00704 * sin_deg( l2 - 2.0 * l3 + pi3 )
						 - 0.00620 * sin_deg( l2 - 2.0 * l3 + pi2 )
						 - 0.00541 * sin_deg( l3 - l4 )
						 + 0.00381 * sin_deg( l2 - 2.0 * l3 + pi4 )
						 + 0.00235 * sin_deg( Psi - omega3 )
						 + 0.00198 * sin_deg( Psi - omega4 )
						 + 0.00176 * sin_deg( PhiA )
						 + 0.00130 * sin_deg( 3.0 * (l3 - l4) )
						 + 0.00125 * sin_deg( l1 - l3 )
						 - 0.00119 * sin_deg( 5.0 * G1 - 2.0 * G + 52.225 )
						 + 0.00109 * sin_deg( l1 - l2 )
						 - 0.00100 * sin_deg( 3.0 * l3 - 7.0 * l4 + 4.0 * pi4 )
						 + 0.00091 * sin_deg( omega3 - omega4 )
						 + 0.00080 * sin_deg( 3.0 * l3 - 7.0 * l4 + pi3 + 3.0 * pi4 )
						 - 0.00075 * sin_deg( 2.0 * l2 - 3.0 * l3 + pi3 )
						 + 0.00072 * sin_deg( pi1 + pi3 - 2.0 * Pi - 2.0 * G )
						 + 0.00069 * sin_deg( pi4 - Pi )
						 - 0.00058 * sin_deg( 2.0 * l3 - 3.0 * l4 + pi4 )
						 - 0.00057 * sin_deg( l3 - 2.0 * l4 + pi4 )
						 + 0.00056 * sin_deg( l3 + pi3 - 2.0 * Pi - 2.0 * G )
						 - 0.00052 * sin_deg( l2 - 2.0 * l3 + pi1 )
						 - 0.00050 * sin_deg( pi2 - pi3 )
						 + 0.00048 * sin_deg( l3 - 2.0 * l4 + pi3 )
						 - 0.00045 * sin_deg( 2.0 * l2 - 3.0 * l3 + pi4 )
						 - 0.00041 * sin_deg( pi2 - pi4 )
						 - 0.00038 * sin_deg( 2.0 * G )
						 - 0.00037 * sin_deg( pi3 - pi4 + omega3 - omega4 )
						 - 0.00032 * sin_deg( 3.0 * l3 - 7.0 * l4 + 2.0 * pi3 + 2.0 * pi4 )
						 + 0.00030 * sin_deg( 4.0 * (l3 - l4) )
						 + 0.00029 * sin_deg( l3 + pi4 - 2.0 * Pi - 2.0 * G )
						 - 0.00028 * sin_deg( omega3 + Psi - 2.0 * PhiA - 2.0 * G )
						 + 0.00026 * sin_deg( l3 - Pi - G )
						 + 0.00024 * sin_deg( l2 - 3.0 * l3 + 2.0 * l4 )
						 + 0.00021 * sin_deg( 2.0 * (l3 - Pi - G) )
						 - 0.00021 * sin_deg( l3 - pi2 )
						 + 0.00017 * sin_deg( 2.0 * (l3 - pi3) );

	L = l3 + S;

	// latitude
	B =  atan (
						 + 0.0032402 * sin_deg( L - omega3 )
						 - 0.0016911 * sin_deg( L - Psi )
						 + 0.0006847 * sin_deg( L - omega4 )
						 - 0.0002797 * sin_deg( L - omega2 )
						 + 0.0000321 * sin_deg( L + Psi - 2.0 * PhiA - 2.0 * G )
						 + 0.0000051 * sin_deg( L - Psi + G )
						 - 0.0000045 * sin_deg( L - Psi - G )
						 - 0.0000045 * sin_deg( L + Psi - 2.0 * Pi )
						 + 0.0000037 * sin_deg( L + Psi - 2.0 * Pi - 3.0 * G )
						 + 0.0000030 * sin_deg( 2.0 * l2 - 3.0 * L + 4.03 * S + omega2 )
						 - 0.0000021 * sin_deg( 2.0 * l2 - 3.0 * L + 4.03 * S + omega3 )
	);


	// raggio vettore
	R = 14.98832 * (1.0
						 - 0.0014388 * cos_deg( l3 - pi3 )
						 - 0.0007919 * cos_deg( l3 - pi4 )
						 + 0.0006342 * cos_deg( l2 - l3 )
						 - 0.0001761 * cos_deg( 2.0 * (l3 - l4) )
						 + 0.0000294 * cos_deg( l3 - l4 )
						 - 0.0000156 * cos_deg( 3.0 * (l3 - l4) )
						 + 0.0000156 * cos_deg( l1 - l3 )
						 - 0.0000153 * cos_deg( l1 - l2 )
						 + 0.0000070 * cos_deg( 2.0 * l2 - 3.0 * l3 + pi3 )
						 - 0.0000051 * cos_deg( l3 + pi3 - 2.0 * Pi - 2.0 * G)
				 );
						 
}



Callisto::Callisto ( ) :
	MoonJupiter( "callisto", nullptr )
{

    D = 4820.6; // km
    radius = D / 2;
	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km

	gl.map = "/my_image/default/callisto.jpg";


}

Callisto::~Callisto() {
}

void Callisto::periodicTerm ( )
{

	// longitudine
	double S = + 0.84287 * sin_deg(l4 - pi4)
						 + 0.03431 * sin_deg(pi4 - pi3)
						 - 0.03305 * sin_deg(2.0 * (Psi - Pi))
						 - 0.03211 * sin_deg(G)
						 - 0.01862 * sin_deg(l4 - pi3)
						 + 0.01186 * sin_deg(Psi - omega4)
						 + 0.00623 * sin_deg(l4 + pi4 - 2.0 * G - 2.0 * Pi)
						 + 0.00387 * sin_deg(2.0 * (l4 - pi4))
						 - 0.00284 * sin_deg(5.0 * G1 - 2.0 * G + 52.225)
						 - 0.00234 * sin_deg(2.0 * (Psi - pi4))
						 - 0.00223 * sin_deg(l3 - l4)
						 - 0.00208 * sin_deg(l4 - Pi)
						 + 0.00178 * sin_deg(Psi + omega4 - 2.0 * pi4)
						 + 0.00134 * sin_deg(pi4 - Pi)
						 + 0.00125 * sin_deg(2.0 * (l4 - G - Pi))
						 - 0.00117 * sin_deg(2.0 * G)
						 - 0.00112 * sin_deg(2.0 * (l3 - l4))
						 + 0.00107 * sin_deg(3.0 * l3 - 7.0 * l4 + 4.0 * pi4)
						 + 0.00102 * sin_deg(l4 - G - Pi)
						 + 0.00096 * sin_deg(2.0 * l4 - Psi - omega4)
						 + 0.00087 * sin_deg(2.0 * (Psi - omega4))
						 - 0.00085 * sin_deg(3.0 * l3 - 7.0 * l4 + pi3 + 3.0 * pi4)
						 + 0.00085 * sin_deg(l3 - 2.0 * l4 + pi4)
						 - 0.00081 * sin_deg(2.0 * (l4 - Psi))
						 + 0.00071 * sin_deg(l4 + pi4 - 2.0 * Pi - 3.0 * G)
						 + 0.00061 * sin_deg(l1 - l4)
						 - 0.00056 * sin_deg(Psi - omega3)
						 - 0.00054 * sin_deg(l3 - 2.0 * l4 + pi3)
						 + 0.00051 * sin_deg(l2 - l4)
						 + 0.00042 * sin_deg(2.0 * (Psi - G - Pi))
						 + 0.00039 * sin_deg(2.0 * (pi4 - omega4))
						 + 0.00036 * sin_deg(Psi + Pi - pi4 - omega4)
						 + 0.00035 * sin_deg(2.0 * G1 - G + 188.37)
						 - 0.00035 * sin_deg(l4 - pi4 + 2.0 * Pi - 2.0 * Psi)
						 - 0.00032 * sin_deg(l4 + pi4 - 2.0 * Pi - G)
						 + 0.00030 * sin_deg(2.0 * G1 - 2.0 * G + 149.15)
						 + 0.00029 * sin_deg(3.0 * l3 - 7.0 * l4 + 2.0 * pi3 + 2.0 * pi4)
						 + 0.00028 * sin_deg(l4 - pi4 + 2.0 * Psi - 2.0 * Pi)
						 - 0.00028 * sin_deg(2.0 * (l4 - omega4))
						 - 0.00027 * sin_deg(pi3 - pi4 + omega3 - omega4)
						 - 0.00026 * sin_deg(5.0 * G1 - 3.0 * G + 188.37)
						 + 0.00025 * sin_deg(omega4 - omega3)
						 - 0.00025 * sin_deg(l2 - 3.0 * l3 + 2.0 * l4)
						 - 0.00023 * sin_deg(3.0 * (l3 - l4))
						 + 0.00021 * sin_deg(2.0 * l4 - 2.0 * Pi - 3.0 * G)
						 - 0.00021 * sin_deg(2.0 * l3 - 3.0 * l4 + pi4)
						 + 0.00019 * sin_deg(l4 - pi4 - G)
						 - 0.00019 * sin_deg(2.0 * l4 - pi3 - pi4)
						 - 0.00018 * sin_deg(l4 - pi4 + G)
						 - 0.00016 * sin_deg(l4 + pi3 - 2.0 * Pi - 2.0 * G);

	L = l4 + S;

	// latitude
	B =  atan (
						 - 0.0076579 * sin_deg(L - Psi)
						 + 0.0044134 * sin_deg(L - omega4)
						 - 0.0005112 * sin_deg(L - omega3)
						 + 0.0000773 * sin_deg(L + Psi - 2.0 * Pi - 2.0 * G)
						 + 0.0000104 * sin_deg(L - Psi + G)
						 - 0.0000102 * sin_deg(L - Psi - G)
						 + 0.0000088 * sin_deg(L + Psi - 2.0 * Pi - 3.0 * G)
						 - 0.0000038 * sin_deg(L + Psi - 2.0 * Pi - G)
	);

	
	// raggio vettore
	R = 26.36273 * (1.0
						 - 0.0073546 * cos_deg(l4 - pi4)
						 + 0.0001621 * cos_deg(l4 - pi3)
						 + 0.0000974 * cos_deg(l3 - l4)
						 - 0.0000543 * cos_deg(l4 + pi4 - 2.0 * Pi - 2.0 * G)
						 - 0.0000271 * cos_deg(2.0 * (l4 - pi4))
						 + 0.0000182 * cos_deg(l4 - Pi)
						 + 0.0000177 * cos_deg(2.0 * (l3 - l4))
						 - 0.0000167 * cos_deg(2.0 * l4 - Psi - omega4)
						 + 0.0000167 * cos_deg(Psi - omega4)
						 - 0.0000155 * cos_deg(2.0 * (l4 - Pi - G))
						 + 0.0000142 * cos_deg(2.0 * (l4 - Psi))
						 + 0.0000105 * cos_deg(l1 - l4)
						 + 0.0000092 * cos_deg(l2 - l4)
						 - 0.0000089 * cos_deg(l4 - Pi - G)
						 - 0.0000062 * cos_deg(l4 + pi4 - 2.0 * Pi - 3.0 * G)
						 + 0.0000048 * cos_deg(2.0 * (l4 - omega4))
					);


						 
}





