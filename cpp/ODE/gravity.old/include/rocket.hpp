#ifndef _rocket_
#define _rocket_

#include <odefunctor.hpp>

class Rocket  : public OdeFunction {

private:

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
	
	void operator () ( const Doub & , VecDoub_I & y, VecDoub_O & dydx ) ;

};


#endif	// if _rocket_
