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
	

		Variabile r ;
		
		const Doub x1 = 0, x2 = 10000;
		

		struct TestFunction1 : public OdeFunction {
		
			P * p;

			Doub M = 5.974e24;
			Doub G = 6.673e-11;
			Doub R = 6.371e6;
			
			Doub g =  9.81 ;	// accelerazione di gravità
			
			TestFunction1 ( P * _p ) : OdeFunction( ), p ( _p )  {}
		
			virtual void operator () ( const Doub & /* x */ , VecDoub_I & y, VecDoub_O & dydx ) {
				dydx[0] = y[1];
				dydx[1]= -G * M  / ( p->r *  p->r  ) ;
			}

		} d ;
		
		
		struct funzr : public OdeFunctor {
	
			P * p;
			funzr ( P * p ) :
				OdeFunctor ( p->x1, p->x2, 2, p->d ) { this->p = p; }
	
			double eval () {
	
				ystart[0] = p->d.R;
				ystart[1] = 100000;
	
				Doub x1 = p->x1 ;
				Doub x2 = p->x2 ;
				Output out;

				ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d ) ;

				ode.integrate();
//				std::cout << "eval con fr = " << p->r << " " << ystart[1]  << std::endl;

				return ystart[1];
	
	
	
			}

		} fr;
	
	
		
		P () : d( this), fr( this ) {
		
			r = d.R;
		
			r.setRange( 1e+2, 1e+10 );
		}
		
		~P () {
		}

	} p ;
	
/*	

	Doub x1=0.0;
	Doub x2=60000.0;
	
//	TestFunction d;
	
	OdeFunctor ode ( x1, x2, 2, p.d ) ;
	ode.ystart[0] = p.d.R;
	ode.ystart[1] = 100000;
	
	ode () ;
	
	for ( Int i=0; i < ode.out.count ; ++i ) 	
		cout << ode.out.xsave[i] << "\t" << ode.out.ysave[0][i] << "\t" << ode.out.ysave[1][i] << std::endl;
*/
		
//	cout << ystart[0] << std::endl;
	std::vector< Funzione * > v_f;
	v_f.push_back( & p.fr);
	
	std::vector< Variabile * > v_v;
	v_v.push_back ( & p.r );
	
	Math < Funzione, Variabile, float > m;
	m.execute ( v_v, v_f );
	
	std::cout << "r = " << p.r << std::endl;

	return (0);

	
}
