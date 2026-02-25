#include <fstream>
#include <sstream>
#include <math.h>       /* floor */

#include <skymap.hpp>
#include <solarDb.hpp>
#include <Utils.h>
#include "bitmap_image.hpp"

SolarDb::Coord::Coord ( const SolarDb::Coord & c ) :
	lat( c.lat), lng( c.lng)
{
}

SolarDb::Coord::Coord ( double _lng , double _lat ) :
	lat ( _lat ), lng ( _lng )
{
}

SolarDb::Grid::Grid (  ) :
	lat ( 0 ), lng ( 0 )
{
}

SolarDb::Grid::Grid ( short _lat, short _lng ) :
	lat ( _lat ), lng ( _lng )
{
}

SolarDb::Grid::Grid ( const Coord & coordinate )
{
	*this = coordinate;
}

SolarDb::Grid & SolarDb::Grid::operator = ( const Coord & coordinate )
{
	// converte le coordinate nel corrispettivo quadrante
	lat = floor( dLat * ( coordinate.lat + 90 ) / 180 );
	lng = floor( dLng * ( coordinate.lng + 180 ) / 360 );

	return *this;

}

SolarDb::SolarDb ( ) :
	connession(
		std::getenv("DB_HOST"),
		std::getenv("DB_USER"),
		std::getenv("DB_PASS"),
		std::getenv("DB_NAME")
	),
	mysql(connession),
	DATADIR( std::getenv("VSOP_DIR") )
{
}

SolarDb::SolarDb ( const std::string & data_dir ) :
	connession(
		std::getenv("DB_HOST"),
		std::getenv("DB_USER"),
		std::getenv("DB_PASS"),
		std::getenv("DB_NAME")
	),
	mysql(connession),
	DATADIR( data_dir )
{
}

void  SolarDb::createStars ( )
{

	std::stringstream can ;
	can << "create table if not exists stars (" <<
		"id MEDIUMINT NOT NULL AUTO_INCREMENT," <<
    	"costellazione varchar(4)," <<
		"nome varchar(30)," <<
		"soprannome varchar(30)," <<
		"AR FLOAT(10,2)," <<
		"DECL FLOAT(10,2)," <<
		"primary key ( id )," <<
		"index ( costellazione, nome )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;

	std::stringstream me ;
	me << "create table if not exists messier (" <<
		"id MEDIUMINT NOT NULL AUTO_INCREMENT," <<
		"nome varchar(4)," <<
		"ncg varchar(10)," <<
		"tipo varchar(5)," <<
		"costellazione varchar(4)," <<
		"soprannome varchar(30)," <<
		"AR FLOAT(10,2)," <<
		"DECL FLOAT(10,2)," <<
		"primary key ( id )," <<
		"index ( nome )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;

	std::stringstream bo ;
	bo << "create table if not exists const_bounds (" <<
		"id MEDIUMINT NOT NULL AUTO_INCREMENT," <<
		"costellazione char(4)," <<
		"_RAJ FLOAT(10,6)," <<
		"_DEJ FLOAT(10,6)," <<
		"RAJ FLOAT(10,6)," <<
		"DE FLOAT(10,6)," <<
		"tipo char(2)," <<
		"primary key ( id )," <<
		"index ( costellazione )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;

	std::stringstream co ;
	co << "create table if not exists costellazioni (" <<
		"codice varchar(4)," <<
		"nome varchar(40)," <<
		"nome_italiano varchar(40)," <<
		"genitivo varchar(40)," <<
		"primary key ( codice )," <<
		"index ( nome )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;


	try {

		mysql.Open( "drop table if exists stars" );
		mysql.Open( "drop table if exists messier" );
		mysql.Open( "drop table if exists const_bounds" );
		mysql.Open( "drop table if exists costellazioni" );

		mysql.Open( can.str() );
		mysql.Open( me.str() );
		mysql.Open( bo.str() );
		mysql.Open( co.str() );


	}
	  catch (const std::runtime_error& e) {
		  std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
		  return ;
		
	  }
	  catch (const std::exception& e) {
		  // intercetta anche altri errori standard
		  std::cerr << "Eccezione generica: " << e.what() << "\n";
		  return ;
	  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }


	{	// constellations

		std::string data = DATADIR + "stars/constellations_table" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::stringstream query;
		query << "insert into costellazioni " <<
			"(" <<
			"codice," <<
			"nome," <<
			"nome_italiano," <<
			"genitivo" <<
			") " <<
			"values ";

		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, ",", values );
			if ( size != 4 ) continue;

			// costellazione, nome_ita, sigla, genitivo
			std::string costellazione = utils::trim(values.at(0));
			std::string nome_italiano = utils::trim(values.at(1));
			std::string sigla = utils::trim(values.at(2));
			std::string genitivo = utils::trim(values.at(3));

			if ( !first_value ) {
				query << ",";
			}
			first_value = false;

			query << "(" <<
				"\"" << sigla  << "\"," <<
				"\"" << costellazione << "\"," <<
				"\"" << nome_italiano  << "\"," <<
				"\"" << genitivo  << "\"" <<
			")" ;

		}


		ifs.close();


		try {
      
      
      query << " ON DUPLICATE KEY UPDATE " <<
        "codice = VALUES(codice)";

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

	}

	{	// greek

		std::string data = DATADIR + "stars/greek" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::stringstream query;
		query << "insert into greek " <<
			"(" <<
			"sigla," <<
			"lettera" <<
			") " <<
			"values ";
		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, "\t", values );
			if ( size != 2 ) continue;

			std::string lettera = utils::trim(values.at(0));
			std::string sigla = utils::trim(values.at(1));

			if ( !first_value ) {
				query << ",";
			}
			first_value = false;

			query << "(" <<
				"\"" << sigla << "\"," <<
				"\"" << lettera  << "\"" <<
			")" ;

		}

		ifs.close();
		mysql.Open( "drop table if exists greek" );

		std::stringstream mo ;
		mo << "create table if not exists greek (" <<
		"sigla varchar(3)," <<
		"lettera varchar(10)," <<
		"primary key ( sigla )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;

		try {

			mysql.Open( mo.str()  );
			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

	}	// fine greek



	{	// stars

		std::string data = DATADIR + "stars/sky_stars1" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::string str;
		std::getline( ifs, str ) ;	// header
		std::stringstream query;
		query << "insert into stars " <<
			"(" <<
			"costellazione," <<
			"nome," <<
			"soprannome," <<
			"AR," <<
			"DECL" <<
			") " <<
			"values ";
		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, ",", values );
			if ( size == 0 ) continue;

		// #progressivo, costellazione, genitivo, nome, soprannome, ascensione retta, declinazione
			std::string costellazione = utils::trim(values.at(1));
			std::string genitivo = utils::trim(values.at(2));
			std::string nome = utils::trim(values.at(3));
			std::string soprannome = utils::trim(values.at(4));
			std::string f_ar = utils::trim(values.at(5));
			std::string f_dec = utils::trim(values.at(6));

			std::stringstream ss;
			ss << "select codice from costellazioni " <<
				"where nome=\"" <<
				costellazione << "\"";

			std::string codice;
			try {

				mysql.Open(ss.str());
				if ( !mysql.isEmpty() ) {

					codice = mysql.FieldByName("codice") ;

				}

			}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }



			if ( !first_value ) {
				query << ",";
			}
			first_value = false;

			query << "(" <<
				"\"" << codice << "\"," <<
				"\"" << nome  << "\"," <<
				"\"" << soprannome  << "\"," <<
				"\"" << f_ar  << "\"," <<
				"\"" << f_dec  << "\"" <<
			")" << std::endl;


		}

		ifs.close();


		try {

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

	}	// fine stars



	{	// messier

		std::string data = DATADIR + "stars/messier" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::string str;
		std::getline( ifs, str ) ;	// header
		std::stringstream query;
		query << "insert into messier " <<
			"(" <<
			"nome," <<
			"ncg," <<
			"tipo," <<
			"costellazione," <<
			"soprannome," <<
			"AR," <<
			"DECL" <<
			") " <<
			"values ";
		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, "\t", values );
			if ( size == 0 ) continue;

//	M	NGC	Type	Mag.	Size arcmin	Distance (ly) 	Right Ascension	Declination	Con	Viewing Season	Common Name _______________
//	M6	6405	Oc	4.2	25	1600 	17h 40.1m	-32° 13′	Sco	summer	Butterfly Cluster
			std::string nome = utils::trim(values.at(0));
			std::string ncg = utils::trim(values.at(1));
			std::string type = utils::trim(values.at(2));
			std::string f_ar = utils::trim(values.at(6));
			std::string f_dec = utils::trim(values.at(7));
			std::string costellazione = utils::trim(values.at(8));
			utils::toUpper( costellazione );

			std::string soprannome =  "";
			if ( values.size() > 10 )
				soprannome =  utils::trim(values.at(10));

			utils::str_replace( "h", "", f_ar);
			utils::str_replace( "m", "", f_ar);
			utils::str_replace( "°", "", f_dec);
			utils::str_replace( "′", "", f_dec);

			std::vector< std::string > v_ar;
			utils::split( f_ar, " ", v_ar );

			std::vector< std::string > v_dec;
			utils::split( f_dec, " ", v_dec );

/*
			float ar = utils::atof(v_ar[0]) + utils::atof( v_ar[1] ) / 60;
			float dec = utils::atof(v_dec[0]) + utils::atof( v_dec[1] ) / 60;
*/
			float ar = atof(v_ar[0].c_str()) + atof( v_ar[1].c_str() ) / 60;
			float dec = atof(v_dec[0].c_str()) + atof( v_dec[1].c_str() ) / 60;

/*
			std::string costellazione = utils::trim(values.at(1));
			std::string genitivo = utils::trim(values.at(2));
*/

			if ( !first_value ) {
				query << ",";
			}
			first_value = false;

			query << "(" <<
				"\"" << nome << "\"," <<
				"\"" << ncg << "\"," <<
				"\"" << type << "\"," <<
				"\"" << costellazione << "\"," <<
				"\"" << soprannome  << "\"," <<
				"\"" << ar  << "\"," <<
				"\"" << dec  << "\"" <<
			")" << std::endl;

		}

		ifs.close();


		try {

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

		// oggetti messier
		mysql.Open( "drop table if exists messier_obj" );

		std::stringstream mo ;
		mo << "create table if not exists messier_obj (" <<
		"id char(2)," <<
		"name varchar(20)," <<
		"primary key ( id )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;


		try {

			mysql.Open( mo.str() );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

		std::stringstream dat ;
		dat <<
			"insert into messier_obj " <<
			"(id,name) " <<
			"values " <<
			"(\"Oc\",\"Open Cluster\")," <<
			"(\"Sp\",\"Spiral Galaxy\")," <<
			"(\"Gc\",\"Globular Cluster\")," <<
			"(\"Ba\",\"Barred Galaxy\")," <<
			"(\"Pl\",\"Planetary Nebula\")," <<
			"(\"Ln\",\"Lenticular Galaxy\")," <<
			"(\"Di\",\"Diffuse Nebula\")," <<
			"(\"El\",\"Elliptical Galaxy\")," <<
			"(\"As\",\"Asterism\")," <<
			"(\"Ir\",\"Irregular Galaxy\")," <<
			"(\"Ds\",\"Double Star\")," <<
			"(\"Sn\",\"Supernova Remnant\")," <<
			"(\"MW\",\"Milky Way Patch\")" ;

		try {

			mysql.Open( dat.str() );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }


	}	// fine messier


	{	// bounds

		std::string data = DATADIR + "stars/bounds" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::string str;
		do {

			ifs >> str ;

		} while ( str != "#START#" ) ;

		std::stringstream query;
		query << "insert into const_bounds " <<
			"(" <<
			"_RAJ," <<
			"_DEJ," <<
			"RAJ," <<
			"DE," <<
			"costellazione," <<
			"tipo" <<
			") " <<
			"values ";

		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, " ", values );
			if ( size == 0 ) continue;

			std::string _RAJ = utils::trim(values.at(0));
			std::string _DEJ = utils::trim(values.at(1));
			std::string RAJ = utils::trim(values.at(2));
			std::string DE = utils::trim(values.at(3));
			std::string costellazione = utils::trim(values.at(4));
			std::string type = utils::trim(values.at(5));

			if ( !first_value ) {
				query << ",";
			}
			first_value = false;

			query << "(" <<
				"\"" << _RAJ << "\"," <<
				"\"" << _DEJ << "\"," <<
				"\"" << RAJ << "\"," <<
				"\"" << DE << "\"," <<
				"\"" << costellazione  << "\"," <<
				"\"" << type  << "\"" <<
			")" << std::endl;

		}

		ifs.close();


		try {

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

	}	// fine bounds

	{	// bsc5

		std::string data = DATADIR + "stars/bsc5/bsc5.dat" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::string str;
		std::getline( ifs, str ) ;	// header
		std::stringstream query;
		query << "insert into bsc5 " <<
			"(" <<
			"HR," <<
			"Flamsteed," <<
			"Bayer," <<
			"HD," <<		// [1/225300]? Henry Draper Catalog Number
			"ADS," <<		// Aitken's Double Star Catalog (ADS) designation
			"costellazione," <<
			"Vmag," <<
			"SpType," <<
			"B_V," <<
			"AR," <<
			"DECL" <<
			") " <<
			"values ";
		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
//			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, "\t", values );
			if ( size == 0 ) continue;

//	M	NGC	Type	Mag.	Size arcmin	Distance (ly) 	Right Ascension	Declination	Con	Viewing Season	Common Name _______________
//	M6	6405	Oc	4.2	25	1600 	17h 40.1m	-32° 13′	Sco	summer	Butterfly Cluster
			std::string HR = str.substr( 0, 4 );
			std::string Name = str.substr( 4, 10 );
			std::string HD = utils::trim( str.substr( 25, 6 ) );
			std::string ADS = str.substr( 44, 7 );
			std::string RAh = str.substr( 75, 2 );
			std::string RAm = str.substr( 77, 2 );
			std::string RAs = str.substr( 79, 4 );
			std::string DEsegn = str.substr( 83, 1 );
			std::string DEd = str.substr( 84, 2 );
			std::string DEm = str.substr( 86, 2 );
			std::string DEs = str.substr( 88, 2 );
			std::string Vmag = utils::trim( str.substr( 102, 5 ) );
			std::string SpType = str.substr( 127, 20 );
			std::string B_V = str.substr( 109, 5 );

			std::string Flamsteed = utils::trim(Name.substr( 0, 3 ));
			std::string Bayer = utils::trim(Name.substr( 3, 3 )) +
				Name.substr( 6, 1 );
			std::string costellazione = utils::trim(Name.substr( 7, 3 ));

			float ar = atof(RAh.c_str()) + atof(RAm.c_str()) / 60 + atof(RAs.c_str()) / 3600;
			float dec = atof(DEd.c_str()) + atof(DEm.c_str()) / 60 + atof(DEs.c_str()) / 3600;

			float b_v = atof( B_V.c_str() ) ;

			if ( DEsegn == "-" )
				dec = -dec;

/*
			std::string costellazione = utils::trim(values.at(1));
			std::string genitivo = utils::trim(values.at(2));
*/


			if ( HD != "" && Vmag != "" ) {

				if ( !first_value ) {
					query << ",";
				}
				first_value = false;

				query << "(" <<
					"\"" << HR << "\"," <<
					"\"" << Flamsteed << "\"," <<
					"\"" << Bayer << "\"," <<
					"\"" << HD << "\"," <<
					"\"" << ADS << "\"," <<
					"\"" << costellazione << "\"," <<
					Vmag << "," <<
					"\"" << SpType << "\"," <<
					"\"" << b_v << "\"," <<
					"\"" << ar  << "\"," <<
					"\"" << dec  << "\"" <<
				")" << std::endl;
			}

		}

		ifs.close();


		// bsc5
		mysql.Open( "drop table if exists bsc5" );

		std::stringstream mo ;
		mo << "create table if not exists bsc5 (" <<
		"id MEDIUMINT NOT NULL AUTO_INCREMENT," <<
		"HR varchar(5)," <<
		"Flamsteed varchar(3)," <<
		"Bayer varchar(4)," <<
		"HD varchar(6)," <<
		"ADS varchar(7)," <<
		"costellazione varchar(4)," <<
		"Vmag FLOAT(5,2)," <<	// Visual magnitude
		"SpType varchar(20)," <<	// Spectral type code
		"B_V FLOAT(5,2)," <<	// B-V color in the UBV system
		"AR FLOAT(10,2)," <<
		"DECL FLOAT(10,2)," <<
		"primary key ( id )," <<
		"index (Flamsteed)" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;


		try {

			mysql.Open( mo.str() );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

		try {

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

	}	// fine bsc5



}

void  SolarDb::createMoon ( )
{


	std::stringstream m ;
	m << "create table if not exists moon (" <<
		"nome varchar(30)," <<
		"tipo varchar(30)," <<
		"dia FLOAT(10,2)," <<
		"lon FLOAT(10,2)," <<
		"lat FLOAT(10,2)," <<
		"primary key ( nome, tipo )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;

	try {
		mysql.Open( "drop table if exists moon" );
		mysql.Open( m.str() );
	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

		std::string data = DATADIR + "moon/moon_crater_1" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::string str;
		std::getline( ifs, str ) ;	// header
		std::stringstream query;
		query << "insert into moon " <<
			"(" <<
			"nome," <<
			"tipo," <<
			"dia," <<
			"lon," <<
			"lat" <<
			") " <<
			"values ";

		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			std::vector< std::string > values ;
			size_t size = utils::split ( str, ",", values );
			if ( size == 0 ) continue;

//#nome, diametro, latitudine, longitudine
			std::string nome = utils::trim(values.at(0));
			std::string diametro = utils::trim(values.at(1));
			std::string latitudine = utils::trim(values.at(2));
			std::string longitudine = utils::trim(values.at(3));


			std::string type = "";
			std::vector < std::string > v;
			utils::split( nome, " ", v );
			if ( v.size() > 1 ) {
				std::string tp = utils::trim(v[0]);
				if (
					tp == "Crater"  ||
					tp == "Mons" ||
					tp == "Montes" ||
					tp == "Mare" ||
					tp == "Lacus" ||
					tp == "Oceanus" ||
					tp == "Landing" ||
					tp == "Impact"
					) {

						type = tp;
						nome = "";
						for ( size_t i = 1; i < v.size(); ++i )
							nome += v[i];

				}
				else
					type = "Crater";

			}
			else
				type = "Crater";

			if ( !first_value ) {
				query << ",";
			}
			first_value = false;

			query << "(" <<
				"\"" << nome << "\"," <<
				"\"" << type << "\"," <<
				"\"" << diametro << "\"," <<
				"\"" << latitudine  << "\"," <<
				"\"" << longitudine  << "\"" <<
			")" << std::endl;

		}

		ifs.close();


		try {

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }


}


void  SolarDb::createAsterism ( )
{


	std::stringstream m ;
	m << "create table if not exists asterism (" <<
		"id MEDIUMINT NOT NULL AUTO_INCREMENT," <<
		"costellazione varchar(4)," <<
		"stella varchar(15)," <<
		"primary key ( id )," <<
		"index ( costellazione )" <<
		") ENGINE=InnoDB CHARACTER SET=utf8" ;

	try {
		mysql.Open( "drop table if exists asterism" );
		mysql.Open( m.str() );
	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

		std::string data = DATADIR + "stars/asterismi" ;
		std::ifstream ifs ( data.c_str(), std::ifstream::in );

		std::stringstream query;
		query << "insert into asterism " <<
			"(" <<
			"costellazione," <<
			"stella" <<
			") " <<
			"values ";

		bool first_value = true;
		while ( !ifs.eof () ) {

			std::string str;

			std::getline( ifs, str ) ;
			str = utils::trim(str);
			if ( str == "" ) continue;
			if ( str[0] == '#' ) continue;
			if ( str[0] == '-' ) continue;

			// la prima occorrenza di :
			size_t pos = str.find(":");

			std::string aste, costellazione;
			if ( pos != std::string::npos ) {
				costellazione = utils::trim(str.substr( 0, pos-1 ));
				aste = utils::trim(str.substr( pos+1, str.size()-1));
			}
			else {
				continue;
			}

			std::vector< std::string > v ;
			size_t size = utils::split ( aste, ",", v );
			if ( size == 0 ) continue;


			for ( size_t i = 0; i < v.size(); ++i ) {

				if ( first_value )
					first_value = false;
				else
					query << ",";

				std::string no = utils::trim(v.at(i));
				query << "(" <<
					"\"" << costellazione << "\"," <<
					"\"" << no << "\"" <<
				")" << std::endl;


			}

			query << ",(" <<
				"\"" << costellazione << "\"," <<
				"\"-1\"" <<
				")" << std::endl;


		}


		ifs.close();


		try {

			mysql.Open( query.str()  );

		}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }


}

void  SolarDb::createStarTexture ( )
{

	bitmap_image bmp ( 4096, 2048 );
	bmp.clear();

	std::string data = DATADIR + "stars/bsc5/bsc5.dat" ;
	std::ifstream ifs ( data.c_str(), std::ifstream::in );

	std::string str;
	std::getline( ifs, str ) ;	// header

	while ( !ifs.eof () ) {

		std::string str;

		std::getline( ifs, str ) ;
//			str = utils::trim(str);
		if ( str == "" ) continue;
		if ( str[0] == '#' ) continue;
		if ( str[0] == '-' ) continue;

		std::vector< std::string > values ;
		size_t size = utils::split ( str, "\t", values );
		if ( size == 0 ) continue;

//	M	NGC	Type	Mag.	Size arcmin	Distance (ly) 	Right Ascension	Declination	Con	Viewing Season	Common Name _______________
//	M6	6405	Oc	4.2	25	1600 	17h 40.1m	-32° 13′	Sco	summer	Butterfly Cluster
		std::string HR = str.substr( 0, 4 );
		std::string Name = str.substr( 4, 10 );
		std::string ADS = str.substr( 44, 7 );
		std::string RAh = str.substr( 75, 2 );
		std::string RAm = str.substr( 77, 2 );
		std::string RAs = str.substr( 79, 4 );
		std::string DEsegn = str.substr( 83, 1 );
		std::string DEd = str.substr( 84, 2 );
		std::string DEm = str.substr( 86, 2 );
		std::string DEs = str.substr( 88, 2 );
		std::string Vmag = str.substr( 102, 5 );
		std::string SpType = str.substr( 127, 20 );
		std::string B_V = str.substr( 109, 5 );

		std::string Flamsteed = utils::trim(Name.substr( 0, 3 ));
		std::string Bayer = utils::trim(Name.substr( 3, 3 )) +
				Name.substr( 6, 1 );
		std::string costellazione = utils::trim(Name.substr( 7, 3 ));

//		float ar = atof(RAh.c_str()) + atof(RAm.c_str()) / 60 + atof(RAs.c_str()) / 3600;
		float dec = atof(DEd.c_str()) + atof(DEm.c_str()) / 60 + atof(DEs.c_str()) / 3600;

//		float b_v = atof( B_V.c_str() ) ;

		if ( DEsegn == "-" )
			dec = -dec;

/*
			std::string costellazione = utils::trim(values.at(1));
			std::string genitivo = utils::trim(values.at(2));
*/


		if ( Name.substr( 1, 4) != "NOVA" && utils::trim( Name ) != "" ) {
		}

	}

}

void SolarDb::createStarIndex ()
{

	std::stringstream index_stars ;
	index_stars << "index_stars_" ;

	TQuery my ( connession );
	// corpi celesti
	try {

		std::stringstream can ;
		can << "create table if not exists " << index_stars.str() << " (" <<
			"quadrante MEDIUMINT," <<
			"tipo char(1)," <<
			"id  MEDIUMINT," <<
			"primary key (  quadrante, tipo, id  )," <<
			"index ( quadrante )" <<
			") ENGINE=InnoDB CHARACTER SET=utf8" ;

		my.Open( "drop table if exists " + index_stars.str() );
		my.Open( can.str() );

	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }

	std::stringstream query ;
	query <<
		"select \"S\" as tipo, id, AR, DECL from bsc5 " <<
		"union " <<
		"select \"M\" as tipo, id, AR, DECL from messier "
	;

	try {

		my.Open( query.str()  );

	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }


	std::stringstream q;

	q << "insert into " << index_stars.str() <<
			" (" <<
			"quadrante," <<
			"tipo," <<
			"id" <<
			") " <<
			"values ";

	bool first_value = true;
	while ( !my.Eof() ) {

		std::string tipo = my.FieldByName("tipo");
		int id = atoi(my.FieldByName("id").c_str());
		float AR = atof(my.FieldByName("AR").c_str());
		if ( AR < 0 )
			AR = 24 + AR  ;

 		float DECL = atof(my.FieldByName("DECL").c_str());

		SkyGrid::Grid grid ;
		grid = SkyGrid::SkyCoord ( AR, DECL );

		short quadrante = grid ;

		if ( !first_value ) {
			q << ",";
		}
		first_value = false;

		q << "(" <<
				quadrante << "," <<
				"\"" << tipo << "\"," <<
				id << ")" ;

		my.Next();

	}

	try {

    q << " ON DUPLICATE KEY UPDATE id=id";
    std::string str = q.str();
		my.Open( q.str()  );

	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return ;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return ;
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return ;
      }
/*
		Grid grid  (100, 100);
		grid = SkyCoord ( 20, 20 );

		short quadrante = grid ;

		SkyCoord s = grid;
		std::stringstream ss ;
		ss << s.Dec << " " << s.AR << std::endl;
		srv.print_log( ss.str()) ;
*/

}


