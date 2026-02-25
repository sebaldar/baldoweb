#ifndef _cmat_
   #include <cmat.hpp>
#endif

#ifndef _variable_
   #include <variabile.hpp>
#endif

#include <odefunctor.hpp>

int main (int /* argc */, char ** /* argv[] */ )
{
	
	
	struct P {
	

		Variabile x1, x2 ;
		
		Doub ystart0 = 0.0;		// condizione al contorno t0
		Doub ystart1 = 20.0;	// condizione al contorno v0
	
		struct TestFunction : public OdeFunction {
		
			Doub g =  9.81 ;	// accelerazione di gravità
			TestFunction ( ) : OdeFunction( )  {}
		
			virtual void operator () ( const Doub & /* x */ , VecDoub_I & y, VecDoub_O & dydx ) {
				dydx[0] = y[1];
				dydx[1]= -g ;
			}

		} d;
		
		struct funz1 : public OdeFunctor {
	
			P * p;
			funz1 ( P * p ) :
				OdeFunctor ( p->x1, p->x2, 2, p->d ) { this->p = p; }
	
			double eval () {
	
				ystart[0] = p->ystart0;
				ystart[1] = p->ystart1;
	
				Doub x1 = p->x1 ;
				Doub x2 = p->x2 ;
				Output out;

				ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d ) ;

				ode.integrate();
				std::cout << "eval con x2 = " << x2 << " " << ystart[1] << std::endl;

				return ystart[1];
	
	
			}

		} f1;
	
		struct funz2 : public Funzione {
	
			Doub g =  9.81 ;	// accelerazione di gravità
			P * p;
			funz2 ( P * p ) { this->p = p; }
			double eval () {
				return p->ystart0 + p->x1 * p->ystart1 - 0.5 * g * p->x1 * p->x1;
			}
	
		} f2;
		
		P () : f1( this ), f2( this ) {
		
			x1 = 0.0;
			x2 = 20.0;
		
		}
		
		~P () {
		}

	} p ;
	
	std::vector< Funzione * > v_f;
	v_f.push_back( & p.f1);
	v_f.push_back( & p.f2);
	
	std::vector< Variabile * > v_v;
	v_v.push_back ( & p.x1 );
	v_v.push_back ( & p.x2 );
	
	Math < Funzione, Variabile, double > m;
	m.execute ( v_v, v_f );
	
	std::cout << "t(v=0) = " << p.x2 << std::endl;
	std::cout << "t(y=0) = " <<  p.x1 << std::endl;
	
	std::cout << "t(y=0) = " <<  p.f1.out.count << std::endl;

	for ( Int i=0; i < p.f1.out.count ; ++i ) 	
		cout << p.f1.out.xsave[i] << "\t" << p.f1.out.ysave[0][i] << "\t" << p.f1.out.ysave[1][i] << std::endl;
		

	return (0);

	
}
