#include <odefunctor.hpp>

struct TestFunction : public OdeFunction {
		
 	Doub eps =  1.0e-3 ;
	TestFunction ( ) : OdeFunction( )  {}
		
	virtual void operator () ( const Doub & /* x */ , VecDoub_I & y, VecDoub_O & dydx ) {
		dydx[0] = y[1];
		dydx[1]= ( ( 1.0 - y[0] * y[0] ) * y[1] - y[0] ) / eps;
	}

} ;
	
int main (int /* argc */, char ** /* argv[] */ )
{



	Doub x1=0.0;
	Doub x2=2.0;

	TestFunction d;
	
	OdeFunctor ode ( x1, x2, 2, d ) ;
	ode.ystart[0]=2.0;
	ode.ystart[1]=0.0;
	
	ode () ;
	
	for ( Int i=0; i < ode.out.count ; ++i ) 	
		cout << ode.out.xsave[i] << " " << ode.out.ysave[0][i] << " " << ode.out.ysave[1][i] << std::endl;

	return (0);
	
}
