#ifndef _moon_jupiter_
#define _moon_jupiter_

#include <celestial_body.hpp>


class MoonJupiter : public CelestialBody  {
	
protected:

	// mean longitudes of the satellites
	double l1, l2, l3, l4;
	// longigudes of the perijoves
	double pi1, pi2, pi3, pi4;
	// longitudes of the nodes on the equaorial plane of jupiter
	double omega1, omega2, omega3, omega4;
	// èrincipal enequlity in the longitude of jupiter
	double Lambda;
	// libretion
	double PhiA;
	// longitude of th node of the equator of jupiter on the eliptic
	double Psi;
	// mean anomalies of jupiter and saturn
	double G;
	double G1;
	// longitude of the perihelion of jupietr
	double Pi = 13.469942;
	
	double L ;	// longitude
	double B ; 	// latitude
	double R;	// vector radius
	
	void commonTerms ( double t );
	virtual void periodicTerm ( ) = 0 ;
	

public:



	MoonJupiter ( const std::string & name, CelestialBody * parent ) ;
	virtual ~MoonJupiter () ;

    using CelestialBody::orbit; // evita hiding se hai overload
    OrbitXYZ orbit(double /* JDE */) ;
};

class Io : public MoonJupiter  {

	void periodicTerm ( );

public:

	Io ( ) ;
	virtual ~Io () ;


};

class Europa : public MoonJupiter  {

	void periodicTerm ( );

public:

	Europa ( ) ;
	~Europa () ;


};

class Ganimede : public MoonJupiter  {

	void periodicTerm ( );

public:

	Ganimede ( ) ;
	~Ganimede () ;


};

class Callisto : public MoonJupiter  {

	void periodicTerm ( );

public:

	Callisto ( ) ;
	~Callisto () ;


};


#endif	// if _moon_jupiter_
