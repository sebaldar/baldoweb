#include <sstream>

#include <propulsore.hpp>

Propulsore::Propulsore()
{
}

PropulsoreLiquido::PropulsoreLiquido() :
	Propulsore()
{
	pV = 0.0;
	Mp = 300000.00;
	Ve = 3000.00;
	Q = 1000.00;
}

PropulsoreSolido::PropulsoreSolido() :
	Propulsore()
{
	pV = 0.0;
	Mp = 800000.00;
	Ve = 4500.00;
	Q = 6000.00;
}

