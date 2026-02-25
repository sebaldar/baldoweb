#ifndef _odefunctor_
#define _odefunctor_

#include <iostream>
#include <funzione.hpp>

#ifndef _NR3_H_
	#include <nr3.hpp>
#endif

#include <odeint.hpp>
#include <stepper.hpp>
#include <stepperdopr5.hpp>
//#include <stepperstoerm.hpp>
//#include <stepperross.hpp>

struct OdeFunction {
		
 	OdeFunction ( ) {}
	
	virtual ~OdeFunction () {}		
	
	virtual void operator () ( const Doub & x, VecDoub_I & y, VecDoub_O & dydx ) = 0 ;

} ;
	
typedef Odeint < StepperDopr5 < class OdeFunction > > ODEStepper;

class OdeFunctor : public Funzione {

public:
	
	const Doub atol=1.0e-3;		// tolleranza assoluta
	Doub rtol=atol;			// tollerenza relativa
	
	Doub h1=0.01;		// dimensione primo step
	Doub  hmin=0.0;		// minimo step
	Doub x1, x2;
	
	Int nvar ;

	OdeFunction & d ;
	
	VecDoub ystart;
	
	ODEStepper * ode ;


public:

	OdeFunctor ( OdeFunction & d ) ;
	OdeFunctor ( Doub x1, Doub x2, int n_var, OdeFunction & d ) ;

	void set ( Doub x1, Doub x2 ) ;

	~OdeFunctor() ;
		
	Output out;

	// funzioni membro
	double eval () ;

};

#endif	// if OdeFunctor
