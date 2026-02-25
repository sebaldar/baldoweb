#ifndef _propulsore_
#define _propulsore_

class Propulsore  {

protected:


	double pV;	// peso a vuoto kg
	double Mp;	// massa propellente kg
	
	double Ve;	// velocità espulsione m/s
	double Q;	// portata gas espulso kg/s
	
	bool attivo = false;
	double Mr = 100;	// massa residua percentuale

public:

	Propulsore();

};

class PropulsoreLiquido : public Propulsore {

	public:
		PropulsoreLiquido();
};

class PropulsoreSolido : public Propulsore {

	public:
		PropulsoreSolido();
};

#endif	// if _propulsore_
