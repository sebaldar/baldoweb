#include <libnova/libnova.h>

struct OrbitLBR {

	double L = 0, B = 0, R = 0;

};


OrbitLBR nova_lunar ( double JD  )
{
/*
struct ln_equ_posn
{
    double ra;	// !< RA. Object right ascension in degrees.
    double dec;	// !< DEC. Object declination 
};
*/
	struct ln_equ_posn equ;
	
	ln_get_lunar_equ_coords_prec (JD, &equ, 0);
	double r = ln_get_lunar_earth_dist(JD) ;	
	
	OrbitLBR orbit ;
	orbit.L = equ.ra;
	orbit.B = equ.dec;
	orbit.R = r;
	
	return orbit ;

}
