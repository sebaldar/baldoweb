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
	

		Variabile x, y, vx, vy ;
		
		const Doub x1 = 0, x2 = 7880000;
		

		struct TestFunction : public OdeFunction {
		
			P * p;

			Doub M = 5.974e24;
			Doub G = 6.673e-11;
			Doub R = 6.371e6;
			
			Doub g =  9.81 ;	// accelerazione di gravità
			
			TestFunction ( P * _p ) : OdeFunction( ), p ( _p )  {}
		
			virtual void operator () ( const Doub & /* x */ , VecDoub_I & y, VecDoub_O & dydx ) {
				Doub r = pow( pow ( p->x * p->x + p->y * p->y , 0.5 ), 3 );
				dydx[0] = y[1];
				dydx[1]= -G * M * y[0] / abs(r) ;
			}

		} d ;
		
		
		
		struct funzx : public OdeFunctor {
	
			P * p;
			funzx ( P * p ) :
				OdeFunctor ( p->x1, p->x2, 2, p->d ) { this->p = p; }
	
			double eval () {
	
				ystart[0] = 0;
				ystart[1] = 1.028e+3;
	
				Doub x1 = p->x1 ;
				Doub x2 = p->x2 ;
				Output out;

				ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d) ;

				ode.integrate();
//				std::cout << "eval con fx = " << p->x << " " << ystart[0]  << std::endl;

				return p->x - ystart[0];
	
	
	
			}

		} fx;
	
		struct funzvx : public OdeFunctor {
	
			P * p;
			funzvx ( P * p ) :
				OdeFunctor ( p->x1, p->x2, 2, p->d ) { this->p = p; }
	
			double eval () {
	
				ystart[0] = 0;
				ystart[1] = 1.028e+3;
	
				Doub x1 = p->x1 ;
				Doub x2 = p->x2 ;
				Output out;

				ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d) ;

				ode.integrate();
//				std::cout << "eval con fx = " << p->x << " " << ystart[0]  << std::endl;

				return p->vx - ystart[1];
	
	
	
			}

		} fvx;
	
		struct funzy : public OdeFunctor {
	
			P * p;
			funzy ( P * p ) :
				OdeFunctor ( p->x1, p->x2, 2, p->d ) { this->p = p; }
	
			double eval () {
	
				ystart[0] = 3.85e+8;
				ystart[1] = 0;
	
				Doub x1 = p->x1 ;
				Doub x2 = p->x2 ;
				Output out;

				ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d ) ;

				ode.integrate();
//				std::cout << "eval con fy = " << p->y << " " << ystart[0]  << std::endl;

				return p->y - ystart[0];
	
	
	
			}

		} fy;
	
		struct funzvy : public OdeFunctor {
	
			P * p;
			funzvy ( P * p ) :
				OdeFunctor ( p->x1, p->x2, 2, p->d ) { this->p = p; }
	
			double eval () {
	
				ystart[0] = 3.85e+8;
				ystart[1] = 0;
	
				Doub x1 = p->x1 ;
				Doub x2 = p->x2 ;
				Output out;

				ODEStepper ode ( ystart, x1, x2, atol, rtol, h1, hmin, out, d ) ;

				ode.integrate();
//				std::cout << "eval con fy = " << p->y << " " << ystart[0]  << std::endl;

				return p->vy - ystart[1];
	
	
	
			}

		} fvy;
	
	
	
	
		
		P () : d( this), fx( this ), fvx( this ), fy( this ), fvy( this ) {
		
		
			x = d.R ;
			x.setRange( 3.85e+6, 3.85e+8 );

			y = d.R;
			y.setRange( 3.85e+6, 3.85e+8 );

			vx = 100 ;
			vx.setRange( 0, 100 );

			vy = 0;
			vy.setRange( 0, 100 );
		
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
	v_f.push_back( & p.fx);
	v_f.push_back( & p.fy);
	v_f.push_back( & p.fvx);
	v_f.push_back( & p.fvy);
	
	std::vector< Variabile * > v_v;
	v_v.push_back ( & p.x );
	v_v.push_back ( & p.y );
	v_v.push_back ( & p.vx );
	v_v.push_back ( & p.vy );
	
	Math < Funzione, Variabile, double > m;
	m.execute ( v_v, v_f );
	
	std::cout << "y = " << p.y << std::endl;
	std::cout << "x = " << p.x << std::endl;
	std::cout << "vy = " << p.vy << std::endl;
	std::cout << "vx = " << p.vx << std::endl;

	return (0);

	
}
