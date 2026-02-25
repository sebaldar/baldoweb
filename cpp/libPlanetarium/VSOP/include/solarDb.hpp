#ifndef _solardb_
#define _solardb_

#include <iostream>
	

#include "tmysql.hpp"


class SolarDb  {

	TConnection connession;
	TQuery mysql;


	struct Grid;

	struct Coord {

		double lat ; 	// latitudine
		double lng;		// longitudine
		
		Coord ( double lat, double lng ) ;
		Coord ( const Coord & );

	};
	
	struct Grid {

		short lat ; 	// latitudine
		short lng ;		// longitudine

		Grid() ;
		Grid ( short lat, short lng );
		Grid ( short quadrante );
		Grid ( const Coord & coordinate );

		Grid &operator = ( const Coord &v);
		operator short () {

			// ritorna l'dentificativo del quadrante
			return lat * dLat + lng  ;

			}

	};


	static const short dLat = 50;
	static const short dLng = 50;

	std::string DATADIR ;

public:
	
	SolarDb () ;
	SolarDb ( const std::string & data_dir ) ;
	virtual ~SolarDb() {}

	void  createStars ( ) ;
	void  createMoon ( ) ;
	void  createAsterism ( );

	void  createStarTexture ( );
	void createStarIndex ();

};

#endif	// if _map_
