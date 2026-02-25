#include <fstream>
#include <sstream>
#include <math.h>       /* floor */

#include "angolo.hpp"
#include <map.hpp>
#include <Utils.h>

const std::string Map::DATADIR = std::getenv("VSOP_DIR");


Map::Coord::Coord ( const Map::Coord & c ) :
	lat( c.lat), lng( c.lng)
{
}

Map::Coord::Coord ( double _lng , double _lat ) :
	lat ( _lat ), lng ( _lng )
{
}




Map::Grid::Grid (  ) :
	lat ( 0 ), lng ( 0 )
{
}

Map::Grid::Grid ( short _lat, short _lng ) :
	lat ( _lat ), lng ( _lng )
{
}

Map::Grid::Grid ( const Coord & coordinate )
{
	*this = coordinate;
}

Map::Grid & Map::Grid::operator = ( const Coord & coordinate )
{
	// converte le coordinate nel corrispettivo quadrante
	lat = floor( dLat * ( coordinate.lat + 90 ) / 180 );
	lng = floor( dLng * ( coordinate.lng + 180 ) / 360 );

	return *this;

}


Map::Map ( const std::string & data_file ) :
	connession(
		std::getenv("DB_HOST"),
		std::getenv("DB_USER"),
		std::getenv("DB_PASS"),
		std::getenv("DB_NAME")
	),
	mysql(connession)
	
{
	std::string star_data = DATADIR + data_file ;
	std::ifstream ifs ( star_data.c_str(), std::ifstream::in );

  	if ( !ifs.good() ) {

		std::cout << " good()=" << ifs.good();
		std::cout << " eof()=" << ifs.eof();
		std::cout << " fail()=" << ifs.fail();
		std::cout << " bad()=" << ifs.bad();
		std::cout << " errore " << star_data.c_str() << std::endl;

		ifs.close();
		throw ("") ;

	}

  	while ( !ifs.eof () ) {

		std::string str;

		std::getline( ifs, str ) ;
		str = Trim(str);
		if ( str == "" ) continue;
		if ( str[0] == '#' ) continue;
		if ( str[0] == '-' ) continue;

		std::vector< std::string > values ;
		size_t size = utils::split ( str, ",", values );
		if ( size == 0 ) continue;

		std::string nome, extra, s_lat, s_lng;
		double lat, lng;

		nome = values.at(0);
		extra = values.at(1);
		lat = atof( values.at(2).c_str() );
		lng = atof( values.at(3).c_str() );


		Coord m ( lat, lng ) ;

		Grid p = m ;
		short quadrante = p ;

		grid.insert( std::pair< short, 
					std::string >( quadrante, nome ) );

	}

	ifs.close();

}

std::pair <gMapIter, gMapIter>  Map::find ( double lng, double lat )
{

//	if ( lng < 0 ) lat += 24;
	Grid p = Coord ( lat, lng ) ;
	short quadrante = p ;

	std::pair <gMapIter, gMapIter> ret;
    ret = grid.equal_range( quadrante );

	return ret;

}

std::stringstream Map::object ( double lng, double lat )
{

	auto ret = find( lng, lat ) ;
 	std::stringstream tip ;
	for ( auto it = ret.first; it != ret.second; ++it) {
					
		std::string place = it->second;
		if ( it != ret.first )
			tip << ", " << place ;
		else
			tip << place ;
				
	}
	
	// se non hai trovato niente cerca intorno
//	double step=1;	// step di un grado
	short range=5;	// gradi su cui si scruta intorno
	for ( short i=0; i<range; i++ ) {
		if ( tip.str() != "" ) break;
		ret = find( lng+1+i, lat ) ;
		for ( auto it = ret.first; it != ret.second; ++it) {
							
			std::string place = it->second;
			tip << ", " << place ;
						
		}
		ret = find( lng-1-i, lat ) ;
		for ( auto it = ret.first; it != ret.second; ++it) {
							
			std::string place = it->second;
			tip << ", " << place ;
						
		}
		ret = find( lng, lat+1+i ) ;
		for ( auto it = ret.first; it != ret.second; ++it) {
							
			std::string place = it->second;
			tip << ", " << place ;
						
		}
		auto ret = find( lng, lat-1-i ) ;
		for ( auto it = ret.first; it != ret.second; ++it) {
							
			std::string place = it->second;
			tip << ", " << place ;
	
		}

	}

	return tip;

}

void  Map::createTable ( )
{
return;	
	std::stringstream cit ;
	cit << "create table if not exists cities (" <<
		"city varchar(30)," <<
		"country varchar(2)," <<
		"province varchar(30)," <<
		"population int," <<
		"longitudine FLOAT(10,2)," <<
		"latitudine FLOAT(10,2)," <<
		"quadrante int," <<
		"primary key ( city, country )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;
		
	std::stringstream can ;
	can << "create table if not exists countries (" <<
		"country varchar(2)," <<
		"iso varchar(3)," <<
		"name varchar(30)," <<
		"primary key (country)" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;
		
	try {
		mysql.Open( cit.str() );
		mysql.Open( can.str() );
	}
	catch ( const std::string & str ) {
		
		std::ofstream ofc ( "/home/sergio/log/srv.log",
		std::ofstream::out );
		ofc << str << std::endl;
		ofc << "#END#" << std::endl;
		ofc.close();
	
	}
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      }
	
	std::ofstream log ( "/home/sergio/log/srv.log", 	
	std::ofstream::out | std::ofstream::app  );
	
	std::string star_data = DATADIR + "earth/world_cities.csv" ;
	std::ifstream ifs ( star_data.c_str(), std::ifstream::in );

 	std::string str;
	std::getline( ifs, str ) ;	// header
 	while ( !ifs.eof () ) {

		std::string str;

		std::getline( ifs, str ) ;
		str = Trim(str);
		if ( str == "" ) continue;
		if ( str[0] == '#' ) continue;
		if ( str[0] == '-' ) continue;

		std::vector< std::string > values ;
		size_t size = utils::split ( str, ",", values );
		if ( size == 0 ) continue;

//city,city_ascii,lat,lng,pop,country,iso2,iso3,province
		std::string country_name = values.at(5);
		std::string country = values.at(6);
		std::string country_iso = values.at(7);
		std::string city = values.at(1);
		std::string population = values.at(4);
		std::string s_lat = values.at(2);
		std::string s_lng = values.at(3);
		std::string province = values.at(8);

		double lat = atof( s_lat.c_str() );
		double lng = atof( s_lng.c_str() );
		
		Coord m ( lat, lng ) ;

		Grid p = m ;
		short quadrante = p ;

		std::stringstream ss ;
		ss << "insert into cities values (" <<
			"\"" << city << "\"," <<
			"\"" << country  << "\"," <<
			"\"" << province  << "\"," <<
			"\"" << population  << "\"," <<
			"\"" << s_lng  << "\"," <<
			"\"" << s_lat   << "\"," <<
			"\"" << quadrante   << "\"" <<
			")" ;
		
		try {
			
			mysql.Open( ss.str()  );
		
		}
		catch ( const std::string & str ) {

			log << ss.str() << std::endl;
			log << str << std::endl;

		}
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          
      }

		
		
		std::stringstream cc ;
		cc << "insert into countries values (" <<
			"\"" << country  << "\"," <<
			"\"" << country_iso  << "\"," <<
			"\"" << country_name  << "\"" <<
			")" ;
		
		try {
			
			mysql.Open( cc.str()  );
		
		}
		catch ( const std::string & str ) {

			log << str << std::endl;

		}
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      }
	
	}

	log.close();
	ifs.close();


}

