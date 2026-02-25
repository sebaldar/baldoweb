#ifndef _orbit_
#define _orbit_

#include <angolo.hpp>

struct OrbitXYZ {

	double x = 0, y = 0, z = 0;	// astronomic unit
	double vx = 0, vy = 0, vz = 0;	// astonomic units per day
	double rot_x = 90, rot_y = 0, rot_z = 0;	// gradi
	
	OrbitXYZ () ;
	OrbitXYZ ( double x, double y, double z ) ;
	OrbitXYZ ( const struct OrbitXYZ & orbit) :
		x(orbit.x),
		y(orbit.y),
		z(orbit.z),
		vx(orbit.vx),
		vy(orbit.vy),
		vz(orbit.vz),
		rot_x(orbit.rot_x),
		rot_y(orbit.rot_y),
		rot_z(orbit.rot_z)
		 {}

	OrbitXYZ ( const struct OrbitEqu & orbit) ;
	OrbitXYZ ( const struct OrbitEcli & orbit) ;

	OrbitXYZ &operator = ( const struct OrbitEqu & orbit );
	OrbitXYZ &operator = ( const struct OrbitEcli & orbit );
	OrbitXYZ &operator = ( const struct OrbitXYZ & orbit );
	
	double R ();	// distanza
	
	OrbitXYZ toEclittic ( double JD )  const;
	OrbitXYZ toEqutorial ( double JD ) const;

};

struct OrbitEcli {

	// ecliptic spherical coordinates
	double L = 0, B = 0 ;	// gradi
	double R = 0;	// UA

	OrbitEcli ( double l, double b, double d )	:
				L(l), B(b), R(d) {}
	OrbitEcli ( ) {}

};

// orbita equatoriale
struct OrbitEqu {

	// coordinate equatoriali
	double epsilon ;	// gradi
	Grade alfa = 0, 	// gradi -180 - + 180 - ascensione retta
		delta = 0 ;	// gradi -90 - +90 - declinazione
	double R = 0;	// UA

	OrbitEqu () ;
	OrbitEqu ( double alfa, double delta, double R )	;		
	OrbitEqu ( const OrbitXYZ & orbit ) ;
	OrbitEqu ( double jd ) ;

	OrbitEqu &operator = ( const OrbitEcli & orbit );
	OrbitEqu &operator = ( const OrbitXYZ & orbit );

};


#endif


