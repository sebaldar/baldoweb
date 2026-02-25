#ifndef _map_
#define _map_

#include <iostream>
#include <map>
#include <vector>
	

#include "tmysql.hpp"

typedef std::multimap< short, std::string > gMap;
typedef gMap::iterator gMapIter;


class Map  {

	friend struct Coordinate;
	friend struct Grid;
	
	public:

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
	
	private :
	
	TConnection connession;
	TQuery mysql;
	// è diviso in un reticolo dDECxdAR

	static const short dLat = 50;
	static const short dLng = 50;

	static const std::string DATADIR ;
	
	std::pair <gMapIter, gMapIter>  find ( double lng, double lat ) ;
	


public:


	// associa alla posizione nella grip l'identificativo 
	gMap grid ;
	
	Map ( const std::string & data_file ) ;
	virtual ~Map() {}

	std::stringstream object ( double lng, double lat );
	void  createTable ( ) ;

};

#endif	// if _map_
