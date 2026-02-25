#ifndef _solarsystem_
#define _solarsystem_

#include <iostream>
#include <map>

#include <celestial_body.hpp>
#ifndef __xml__
	#include <xml.hpp>
#endif
#include <moon_jupiter.hpp>

class SolarSystem  {
	void addBody ( XMLElement * el, CelestialBody * body ) ;

public:

	std::string map_dir;

	std::map < std::string, CelestialBody & >  bodies ;

	Sky sky ;
	Sun sun ;
	Earth earth ;
	Moon moon;
	Mercury mercury ;
	Venus venus ;
	Mars mars ;
	Jupiter jupiter ;
	Io io ;
	Europa europa ;
	Ganimede ganimede ;
	Callisto callisto ;
	Saturn saturn ;
	Uranus uranus ;
	Neptune neptune;
	
	SolarSystem ( const std::string & data ) ;
	virtual ~SolarSystem() ;

	std::string primitive () const ;
	std::string primitive_pos () const ;

};


#endif	// if _solarsystem_
