#ifndef _skymap_
#define _skymap_

#include <iostream>
#include <map>
#include <vector>
#include "tmysql.hpp"
	

/**************************************
***************************************
***************************************
**************************************/

class SkyGrid  {

	std::string
		host,
		user,
		pass,
		db;
	
	public:

	struct Grid;

	struct SkyCoord {

		double Dec;		// declinazione -90 - 90
		double AR ; 	// ascensione retta  0 - 24
		
		SkyCoord ( double a, double d ) ;
	
		SkyCoord ( const SkyCoord & );
		SkyCoord ( const Grid & grid );

		SkyCoord &operator = ( const Grid &v);

	};
	
	struct Grid {

		// il firmamento è diviso in un reticolo dDECxdAR
		short dDEC = 200;
		short dAR = 200;

		short dec ;		// declinazione -dDEC/2 - dDEC/2
		short ar ; 	// ascensione retta 0 - dAR

		Grid() ;
		Grid ( short dDec, short dAR );
		Grid ( short quadrante );

		Grid &operator = ( const SkyCoord &v);
		operator short () {

			// ritorna l'dentificativo del quadrante
			return dec * dDEC + ar ;

			}

	};
	
	private:


	const std::vector < std::string > zodiaco = {
		"ARI",
		"TAU",
		"GEM",	
		"CNC",
		"LEO",
		"VIR",
		"LIB",
		"SCO",
		"SGR",
		"CAP",
		"AQR",
		"PSC"
	};
	
	// costruisce i poligono dei confini della costellazione
	struct Point {
		
		double x, y;
		
		Point ( ) : Point( 0, 0 ) {}
		Point ( const Point & p) : Point( p.x, p.y ) {}
		Point ( double _x, double _y ) : x(_x), y(_y) {}
		Point &operator = ( const Point & p) {
			x = p.x;
			y = p.y;
			return *this;
		}
	
	};
	
	class WSsrv &srv ;
	class clientData *client;
	
  mutable TConnection connection;
	
  bool isInBounds_old ( double AR, double Dec, const std::vector< Point > & c_polig ) const ;
	short int  YisInBounds ( double AR, double Dec, const std::vector< Point > & c_polig ) const ;

public:


	
	SkyGrid ( class clientData * client ) ;
	virtual ~SkyGrid() ;

	
	std::vector < double > equ2ecli ( const std::vector  < double > & ) const;
	std::string ecliptic ( short year ) const ;
	std::string trackEquatorial ( bool hidden ) const ;
	std::string trackEcliptic ( bool is_hidden ) const ;

	void erasePlanetsFromIndex ( ) ;
	void insertPlanetIntoIndex ( short id, double AR, double Dec );
	std::string  find ( double AR, double Dec ) ;
	
	std::string  asterism ( const std::string & constellation, bool hidden=true  ) const ;
	std::string trackAsterism() const;

	std::string createSkyStars (  float magnitudine  ) const ;

	std::string drawBounds (  const std::string & constellation, bool hidden ) const ;
	std::string whichConstellation ( double x, double y ) const ;
	std::string whichConstellation_old ( double x, double y ) const ;
	std::string whichZodiac ( double AR ) const ;
	std::string constellationCodeFromName ( std::string name ) const ;
	
	std::string hitPoint ( double AR, double Dec, bool hidden = false ) const ;

	std::vector< std::string> constellationStars ( const std::string & constellation_code ) const ;

};


#endif	// if _skymap_
