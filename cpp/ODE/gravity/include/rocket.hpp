
#ifndef _rocket_
#define _rocket_


#include <vettore.hpp>
#include <odefunctor.hpp>
#include <celestial_body.hpp>

class Rocket  {

public:

	OrbitXYZ orbit;

	struct Function : public OdeFunction {

		const OrbitXYZ &orbit;
		enum Coordinata { X, Y, Z } coordinata;


		Doub G = CelestialBody::G;	// costante gravitazionale m3 kg s2

		Function ( const OrbitXYZ & o, Coordinata c) :
			OdeFunction( ),
			orbit( o ),
			coordinata(c)	{
		}

		virtual void operator () ( const Doub & t, VecDoub_I & y, VecDoub_O & dydx ) ;

	} dx, dy, dz ;

	struct Orientamento {
		double AR;
		double dec;
	} direzione, lookat;

	double M;	// massa rocket kg

	class Propulsore * stadio1 = nullptr;
	class Propulsore * stadio2 = nullptr;
	class Propulsore * stadio3 = nullptr;

public:

	Rocket();
	~Rocket();

	void operator () ( double jd1, double jd2 ) ;

};


#endif	// if _rocket_
