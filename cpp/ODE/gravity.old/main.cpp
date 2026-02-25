#include <derivata.hpp>
#include <celestial_body.hpp>

#include <rocket.hpp>

#include <vettore.hpp>

struct TestFunction : public OdeFunction {
		
	Doub G = CelestialBody::G;	// costante gravitazionale m3 kg s2
	Doub Mt = 0; // kg massa terra 
	
	TestFunction ( ) : OdeFunction( )  {}
		
	virtual void operator () ( const Doub & x, VecDoub_I & y, VecDoub_O & dydx ) {
		dydx[0] = y[1];
		dydx[1]= -G * Mt / ( y[0] * y[0] ) ;
	}

} ;

	struct Test : NumDerivatives {

		Earth earth;
		
		enum {X, Y, Z} coordinata=X;
		double latitude =0;
		double longitude =0;
		
		Test ( ) : NumDerivatives() {
		}
		
		Doub func( Doub JD ) {
			
			OrbitXYZ orbit = earth.orbit ( MainBody::earth_data, JD ) ;
			double rot = earth.rotation ( JD ) ;
			
			Point coord = earth.bodyCoord ( latitude, longitude + rot );

			switch ( coordinata ) {
				case X:
				default:
					return orbit.x + coord.x / CelestialBody::AU;;
				case Y :
					return orbit.y + coord.y / CelestialBody::AU;;
				case Z :
					return orbit.z + coord.z / CelestialBody::AU;;
				
			}
		}
		
	};


int main (int /* argc */, char ** /* argv[] */ )
{

	Date date;
	JDay JD = date.julianDay();
	Doub t1=0;
	Doub t2=10;
	
Earth earth;	
TestFunction d;
	d.Mt=earth.M;
	
	Doub start=0;
Doub r = earth.R;
	Doub v0 = 1000;
	std::cout.precision(25);	
	int j=0;
	while ( j < 10 ) {
		
		OdeFunctor ode ( t1, t2, 2, d ) ;
		ode.ystart[0]= r+start;
		ode.ystart[1]=v0;
		
		ode () ;
		
		
		for ( Int i=0; i < ode.out.count ; ++i ) {	
			Doub acc =  -d.G * d.Mt / ( ode.out.ysave[0][i] * ode.out.ysave[0][i] ) ;
			Doub altezza = ode.out.ysave[0][i] - r;
			cout << j*10+ode.out.xsave[i] << "\t" << ode.out.ysave[0][i] - earth.R << "\t" << ode.out.ysave[1][i] << "\t" << acc << "\t" << altezza << std::endl;
		}
		
		j++;
		
		start=ode.out.ysave[0][ode.out.count-1]-r;
		v0=ode.out.ysave[1][ode.out.count-1];
	
	}
	
	return (0);
	
}
