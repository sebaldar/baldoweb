#include <stdlib.h>

#include <iostream>
#include <math.h>

#ifndef _funzione_
   #include <funzione.hpp>
#endif

#ifndef _variable_
   #include <variabile.hpp>
#endif

#ifndef _cmat_
   #include <cmat.hpp>
#endif

#include <vettore.hpp>
#include <odefunctor.hpp>
#include <celestial_body.hpp>

struct FOde : public OdeFunction {
			
	class Oggetto &obj;
	enum Coordinata { X, Y, Z } coordinata;
	OdeFunctor ode;

	FOde ( class Oggetto & o, Coordinata c ) ;
			
	virtual void operator () ( const Doub & t, VecDoub_I & y, VecDoub_O & dydx ) ;

} ;


class Oggetto  
{

public:
	
	OrbitXYZ O;
	double JD1, JD2;
	Variabile d2, x, y, z;
	
	Oggetto () ;
	~Oggetto () ;

	struct FX : public Funzione {


		Oggetto &obj;
		FOde f;
		
		FX( Oggetto &t ) ;
		double eval();
	
	} fx ;

	struct FY : public Funzione {


		Oggetto &obj;
		FOde f;
		
		FY( Oggetto &t ) ;
		double eval();
	} fy;

	struct FZ : public Funzione {


		Oggetto &obj;
		FOde f;
		
		FZ( Oggetto &t ) ;
		double eval();
	} fz ;
	
	struct FD : public Funzione {

		Oggetto &obj;
		
		FD( Oggetto &t ) ;
		double eval();
	
	} fd ;
	
	void execute ( const OrbitXYZ &orbit, double jd1, double jd2 ) ;
	
} ;

