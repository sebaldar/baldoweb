#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>

#include "WSsrv.hpp"
#include "angolo.hpp"
#include <color.hpp>
#include <skymap.hpp>
#include <Utils.h>
#include <map.hpp>
#include <celestial_body.hpp>
#include <solarSystem.hpp>

#ifndef _matrix_
   #include <matrix.hpp>
#endif

#include <isInBounds.hpp>


// implementazione SkyGrid
/**************************************
***************************************
***************************************
**************************************/


SkyGrid::SkyCoord::SkyCoord ( const SkyGrid::SkyCoord & c ) :
	Dec( c.Dec), AR( c.AR)
{
}

SkyGrid::SkyCoord::SkyCoord ( double _AR , double _Dec) :
	Dec ( _Dec ), AR ( _AR )
{
		
}

SkyGrid::SkyCoord::SkyCoord ( const Grid & grid )
{
	//
	*this = grid;
}

SkyGrid::SkyCoord & SkyGrid::SkyCoord::operator = ( const Grid & coordinate )
{

	// converte le coordinate nel corrispettivo quadrante
	float x = 24. * coordinate.ar / coordinate.dAR;
	float y = 90. * coordinate.dec / coordinate.dDEC;
	
	// il centro del quadratino
	Dec = x + 0.5 / coordinate.dDEC;
	AR = y + 0.5 / coordinate.dAR;

	return *this;

}





SkyGrid::Grid::Grid (  ) 
{
}

SkyGrid::Grid::Grid ( short _Dec, short _AR ) :
	dDEC ( _Dec ), dAR ( _AR )
{
}


SkyGrid::Grid & SkyGrid::Grid::operator = ( const SkyCoord & coordinate )
{
	// converte le coordinate nel corrispettivo quadrante
	dec = floor( dDEC * ( coordinate.Dec ) / 90 );
	ar = floor( dAR * ( coordinate.AR ) / 24 );

	return *this;

}



SkyGrid::SkyGrid ( clientData * _client ) :
	host(std::getenv("DB_HOST")),
	user(std::getenv("DB_USER")),
	pass(std::getenv("DB_PASS")),
	db(std::getenv("DB_NAME")),
	srv ( *_client->parent ),
	client  ( _client ),
 	connection(
		std::getenv("DB_HOST"),
		std::getenv("DB_USER"),
		std::getenv("DB_PASS"),
		std::getenv("DB_NAME")
	)
{
}

SkyGrid::~SkyGrid() 
{
}

void  SkyGrid::erasePlanetsFromIndex ( ) 
{


	std::stringstream index_stars ;
	index_stars << "index_stars_"  ;

	TQuery my (  connection  );
	std::stringstream query;
	query << 
		"delete from " << index_stars.str() <<
		" where tipo=\"P\"" ;
	try {
			
		my.Open( query.str()  );
		
	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]"; 
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
  }
  catch (...) {
      // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
  }

}

void SkyGrid::insertPlanetIntoIndex ( short id, double AR, double DECL )
{

			
	if ( AR < 0 )
		AR = 360 + AR  ;

	AR = AR * 24 / 360;

	std::stringstream index_stars ;
	index_stars << "index_stars_"  ;
		
	SkyGrid::Grid grid ;
	grid = SkyGrid::SkyCoord ( AR, DECL );
		
	short quadrante = grid ;

	TQuery my (  connection  );

	std::stringstream query;
	query << "insert into " << index_stars.str() <<
			" (" <<
			"quadrante," <<
			"tipo," <<
			"id" <<
			") " <<
			"values ";
	query << "(" << 
			quadrante << "," <<
			"\"P\"," <<
			id << ")" ;


	try {
    
    query << " ON DUPLICATE KEY UPDATE id=id";
		my.Open( query.str()  );
		
	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
  }
  catch (...) {
      // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " <<  " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
  }

}
 
std::string  SkyGrid::find ( double AR, double DECL ) 
{

	std::stringstream index_stars ;
	index_stars << "index_stars_"  ;

	std::stringstream ret;
	TQuery my (  connection  );
	TQuery qu (  connection  );

	Grid grid  ;
	grid = SkyCoord ( AR, DECL );
		
		
	short quadrante = grid ;
	
	std::stringstream query;
	query << 
		"select tipo, id from " << index_stars.str() <<
		" where quadrante=" << quadrante ;
// ret << quadrante << " ";		

	try {
			
		my.Open( query.str()  );
		
	}
	catch ( const std::string & ss ) {

		srv.print_log( ss ) ;

	}
	catch ( const char * ss ) {

		srv.print_log( ss ) ;

	}
  catch (...) {
      // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " <<  " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
  }
	
	bool isFirst = true;
	while ( !my.Eof() ) {
		
		std::string tipo = my.FieldByName("tipo");
		std::string id = my.FieldByName("id");
		
		if ( tipo == "S" ) {

			std::stringstream ss;
			ss <<
				"select Bayer, Flamsteed, HD, ADS, " <<
				"if ( isnull(costellazione),\"\",upper(substr(costellazione,1,3)) ) as conste," <<
				"if ( isnull(genitivo),\"\",genitivo) as genitivo, " <<
				"Vmag," <<
				"SpType," <<
				"B_V " <<
				"from bsc5 as S " <<
				"left join costellazioni as C " <<
				"on upper(costellazione)=substr(codice,1,3) " <<
				"where id=" << id;
				
/*
			ss <<
"select CONCAT_WS(',',lettera,Flamsteed) as nome, soprannome, C.nome as conste, genitivo from bsc5 as S " <<
"join greek " <<
"on sigla = Bayer " <<
"join stars as ST " <<
"on lettera=nome and upper(S.costellazione)=ST.costellazione " <<
"join constellations as C " << 
"on upper(S.costellazione)=codice " <<
				"where S.id=" << id;
*/				

			try {
			
				qu.Open( ss.str() );
			
			}
      catch (const std::runtime_error& e) {
        std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]"; 
          return "";
      }
      catch (const std::exception& e) {
          // intercetta anche altri errori standard
          std::cerr << "Eccezione generica: " << e.what() << "\n";
          return "";
      }
  catch (...) {
      // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " <<  " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return "";
  }
	
			if ( !qu.isEmpty() ) {

				std::string Bayer = utils::trim(qu.FieldByName("Bayer"));
				std::string Flamsteed = utils::trim(qu.FieldByName("Flamsteed"));
				std::string HD = qu.FieldByName("HD");
				std::string ADS = utils::trim(qu.FieldByName("ADS"));
				std::string soprannome ;
				std::string conste = qu.FieldByName("conste");
				std::string genitivo = qu.FieldByName("genitivo");

				std::string Vmag = qu.FieldByName("Vmag");
				std::string B_V = qu.FieldByName("B_V");
				std::string SpType = qu.FieldByName("SpType");


				TQuery q ( connection );

				std::stringstream g;
				g <<
					"select lettera from greek " <<
					"where sigla =trim(\"" << Bayer << "\")"  ;

				try {

					q.Open( g.str() );

					if ( !q.isEmpty() ) {
						Bayer = q.FieldByName("lettera");
					}

				}
        catch (const std::runtime_error& e) {
          std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]"; 
            return "";
        }
        catch (const std::exception& e) {
            // intercetta anche altri errori standard
            std::cerr << "Eccezione generica: " << e.what() << "\n";
            return "";
        }
        catch (...) {
            // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
            return "";
        }
	
				std::stringstream c;
				c <<
				"select nome, soprannome from stars " <<
				"where nome=\"" << Bayer <<
				"\" and costellazione=\"" << conste << "\"";
				
				try {
			
					q.Open( c.str() );
			
					if ( !q.isEmpty() ) {
						soprannome = q.FieldByName("soprannome");
					}

				}
        catch (const std::runtime_error& e) {
          std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]"; 
            return "";
        }
        catch (const std::exception& e) {
            // intercetta anche altri errori standard
            std::cerr << "Eccezione generica: " << e.what() << "\n";
            return "";
        }
        catch (...) {
            // intercetta tutto ciò che non è std::exception
			std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
            return "";
        }
	
				if (!isFirst ) 
					ret <<  ", ";
				else
					isFirst = false;

				if ( soprannome != "" ) {
					ret <<  soprannome  << " ";
				}
				
				if ( Bayer != "" ) {
					
					ret <<  Bayer  << " "   ;
					
					if ( Flamsteed != "" ) {
						ret <<  "(" << Flamsteed  ;
						if ( ADS != "" ) {
							ret <<  " " << ADS ;
						}
						ret <<  ") "   ;
					}

					ret <<  genitivo  ;
				}
				else {
					if ( Flamsteed != "" ) {
						ret <<  Flamsteed << " " << genitivo << std::endl ;
					}
					else {
						ret <<  "HD" << HD << " ";
					}
				}
				
				ret << " [Mag " <<  Vmag << " Classe " << SpType << "]" ;


				
			
			}
			
		}
		else if ( tipo == "M" ) {
			
			std::stringstream ss;
			ss <<
				"select M.nome as nome, ncg, soprannome, C.nome as conste, genitivo, MO.name as obj from messier as M " <<
				"join costellazioni as C " <<
				"on costellazione=codice " <<
				"join messier_obj as MO " <<
				"on MO.id=tipo " <<
				"where M.id=" << id;

			try {
			
				qu.Open( ss.str() );
			
			}
      catch (const std::runtime_error& e) {
        std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]"; 
        return "";
      }
      catch (const std::exception& e) {
          // intercetta anche altri errori standard
          std::cerr << "Eccezione generica: " << e.what() << "\n";
          return "";
      }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return "";
      }
	
			if ( !qu.isEmpty() ) {
				
				std::string nome = qu.FieldByName("nome");
				std::string ncg = qu.FieldByName("ncg");
				std::string soprannome = qu.FieldByName("soprannome");
				std::string conste = qu.FieldByName("conste");
				std::string genitivo = qu.FieldByName("genitivo");
				std::string obj = qu.FieldByName("obj");
				
				if (!isFirst ) 
					ret <<  ", ";
				else
					isFirst = false;

				ret << nome << " (NCG" << ncg << ") " << genitivo << 
						" " << soprannome << " " << obj;

			
			}
			
		}
		else if ( tipo == "P" ) {

			std::string nome ;
			short i = atoi( id.c_str() );
			switch ( i ) {
				case 1 :
					nome = "mercury";
				break;
				case 2 :
					nome = "venus";
				break;
				case 4 :
					nome = "mars";
				break;
				case 5 :
					nome = "jupiter";
				break;
				case 6 :
					nome = "saturn";
				break;
			}

			ret << "planet " << nome << " ";

		}
		
				
		my.Next();

	}
		

	return ret.str();

}

std::string SkyGrid::createSkyStars ( float magnitudine ) const
{
	Sky sky;
	double R = sky.radius * 0.5;
	
	TQuery my ( connection );

	std::stringstream query ;
	query << 
		"select  " <<
		"Flamsteed," <<
		"Bayer," <<
		"costellazione," <<
		"Vmag," <<
		"SpType," <<
		"B_V," <<
		"AR," <<
		"DECL " <<
		"from bsc5 " << 
		"where Vmag <=" << magnitudine;

		
	try {
		my.Open( query.str() );
	}
	catch ( const std::string & ss ) {
		srv.print_log( ss ) ;
	}
	catch ( const char * ss ) {
		srv.print_log( ss ) ;
	}
      catch (...) {
          // intercetta tutto ciò che non è std::exception
		std::cerr << "Errore sconosciuto!\n: " <<  " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      }

    // ── STRUTTURA DI OTTIMIZZAZIONE WEBGL ──
    // Mappa per raggruppare i vertici per "colore_dimensione"
    std::map<std::string, std::vector<std::string>> starGroups;

	while ( !my.Eof() ) {
	
		float Vmag = atof( my.FieldByName("Vmag").c_str() )  ;
		
		float AR = atof( my.FieldByName("AR").c_str() ) * 360 / 24 ;
		float DECL = atof( my.FieldByName("DECL").c_str() );
		float B_V = atof( my.FieldByName("B_V").c_str() );
		
		BV bv = B_V;	// classe spettrale
		RGB rgb = bv;	// colore corrispondente

		Radiant a = Grade( AR  );
		Radiant d = Grade( DECL  );
		
		double z = R * sin( d );
		double r1 = R * cos( d );

		double x = r1 * cos( a );
		double y = r1 * sin( a );
		
		int size = 2;

		HSV hsv = rgb;
		int i_mag = floor( Vmag + 0.5 ) ;
		
		switch ( i_mag ) {
			case 8 : case 7 :  {
				hsv.V = 0.08;
				size = 1;
			}
				break; 
			case 6 :  {
				hsv.V = 0.12;
				size = 1;
			}
				break; 
			case 5 : case 4 : case 3 : {
				float f = pow ( 2.512, Vmag - 2 );
				hsv.V = 1 / f + 0.3;
				size = 1;
			}
				break;
			case 2 : {
				hsv.V = 0.8;
				size = 2;
			}
				break;
			case 1 : {
				hsv.V = 1;
				size = 2;
			}
				break;
			case 0 : {
				hsv.V = 0.9 ;
				size = 3;
			}
				break;
			default :
				size = 3;
				hsv.V = 1 ;
				break;
		}

		if ( hsv.V > 1 ) hsv.V = 1.0;
		if ( hsv.V <= 0 ) hsv.V = 0.1;
		
		rgb = hsv;
		std::string color = rgb;
		
        // ── RAGGRUPPAMENTO VERTICI ──
        // Creiamo la chiave di gruppo (es: "0xffffff_2")
        std::string groupKey = color + "_" + std::to_string(size);
        
        // Creiamo la stringa delle coordinate XYZ
        std::stringstream vertStr;
        vertStr << x << " " << y << " " << z;
        
        // Salviamo il vertice nel suo gruppo di appartenenza
        starGroups[groupKey].push_back(vertStr.str());

		my.Next();
	}

    // ── GENERAZIONE XML FINALE ──
	std::stringstream t ;
	int layer_index = 0;
	
    // Iteriamo sui gruppi creati
    for (auto const& group : starGroups) {
        
        // Separiamo il colore e la dimensione dalla chiave
        std::string key = group.first;
        size_t underscorePos = key.find('_');
        std::string color = key.substr(0, underscorePos);
        std::string size = key.substr(underscorePos + 1);
        
        t << "<primitive type=\"point\" name=\"stars_layer_" << layer_index++ << "\">" << std::endl ;
        
        // Stampiamo TUTTI i vertici che hanno questo stesso colore e dimensione
        for (auto const& vertice : group.second) {
            t << "	<vertice>" << vertice << "</vertice>" << std::endl ;
        }
        
        t << "	<material type=\"point\" color=\"" << color << "\" size=\"" << size << "\" />" << std::endl ;
        t << "</primitive>" << std::endl ;
    }

	return t.str() ;
}

std::string  SkyGrid::asterism ( const std::string & constellation, bool hidden ) const
{

	Sky sky;
	double R = sky.radius * 0.5 ;
	
	TQuery my (  connection  );

	std::stringstream query;
	query << 
		"select stella, AR, DECL "  <<
		"from asterism as A " << 
		"left outer join bsc5 as B " << 
		"on UPPER(substr(A.costellazione,1,3))=UPPER(B.costellazione) " << 
		"and (A.stella=B.Flamsteed or " << 
		"A.stella=B.Bayer) " << 
		"where UPPER(A.costellazione)=UPPER(\"" << constellation << "\") "  <<
		"order by A.id " 
		;
		
	try {
			
		my.Open( query.str()  );
		
	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return "";
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return "";
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return "";
      }
	
std::string is_hidden = hidden ? "<hidden />\n" : "<show />\n" ;
	std::stringstream t ;
	bool begin_asterism = true;
	short no = 0;
    
	while ( !my.Eof() ) {
	
		std::string stella = utils::trim(my.FieldByName("stella"));
        
        // 1. Gestione "Alza la penna" (Rottura linea)
		if ( stella == "-1" ) {
			// Chiudiamo la linea solo se ne avevamo effettivamente aperta una
            if (!begin_asterism) {
			    t << is_hidden 
                  << "<material type=\"line_basic\" size=\"1\" color=\"0x00ff00\" />\n" 
                  << "</primitive>\n";
            }
			begin_asterism = true;
			no++;
			my.Next();
			continue;
		}
        
        float AR = 0.0f, DECL = 0.0f;
        bool star_found = false;

        // 2. Acquisizione Coordinate (Stella Esterna o Interna)
		if ( stella.substr(0,1) == "[" ) {
            // Stella esterna
			size_t pos = stella.find(":");
			if ( pos != std::string::npos ) {
				size_t pos_end = stella.find("]");
				std::string costellazione = utils::trim(stella.substr( 1, pos-1 ));
				std::string star = utils::trim(stella.substr( pos+1, pos_end-pos-1));
			
			    TQuery q( connection );
			    std::stringstream query2;
			    query2 << "select AR, DECL from bsc5 " 
                       << "where costellazione=\"" << costellazione << "\" " 
                       << "and (Flamsteed=\"" << star << "\" or Bayer=\"" << star << "\" )";
			
			    try {
				    q.Open( query2.str() );
                    if ( !q.isEmpty() ) {
                        AR = atof( q.FieldByName("AR").c_str() );
                        DECL = atof( q.FieldByName("DECL").c_str() );
                        star_found = true;
                    }
			    } catch (...) {
                    // Errore silenzioso: se non trova la stella esterna, salta il vertice 
                    // ma non fa crashare l'asterismo intero
			    }
            }
		} else {
            // Stella normale (già ottenuta dalla JOIN iniziale)
            AR = atof( my.FieldByName("AR").c_str() );
		    DECL = atof( my.FieldByName("DECL").c_str() );
            star_found = true;
        }

        // 3. Matematica XYZ e Generazione XML
        if (star_found) {
            AR = AR * 360 / 24;
            Radiant a = Grade( AR );
            Radiant d = Grade( DECL );
            
            double z = R * sin( d );
            double r = R * cos( d );
            double x = r * cos( a );
            double y = r * sin( a );

            // Controlliamo l'apertura UNA SOLA VOLTA, valida per tutte le stelle
            if ( begin_asterism ) {
                t << "<primitive type=\"line\" name=\"ast_" << no << "_" << constellation << "\">\n";
                begin_asterism = false;
            }

            t << "<vertice>" << x << " " << y << " " << z << "</vertice>\n";
        }
		
		my.Next();
	}
	
    // 4. CHIUSURA DI SICUREZZA (Il Fix per il Bug Critico)
    // Se il ciclo finisce ma non c'era un "-1" alla fine, chiudiamo il tag rimasto aperto!
    if (!begin_asterism) {
        t << is_hidden 
          << "<material type=\"line_basic\" size=\"1\" color=\"0x00ff00\" />\n" 
          << "</primitive>\n";
    }

	return t.str() ;


}

std::string SkyGrid::hitPoint ( double AR, double DECL, bool hidden ) const
{

//	float AR = atof( my.FieldByName("AR").c_str() ) * 360 / 24 ;
//	float DECL = atof( my.FieldByName("DECL").c_str() );

	Sky sky;
	double R = sky.radius * 0.8 ;
	
	
	std::string is_hidden = hidden ? "<hidden />" : "<show />" ;
	
	std::stringstream t ;
		
	Radiant a = Grade( AR  );
	Radiant d = Grade( DECL + 0.01 );
		
	double z = R * sin( d );
	double r = R * cos( d );

	double x = r * cos( a );
	double y = r * sin( a );

	t << 
	"<primitive type=\"sphere\" name=\"hit\" radius=\"0.01\">" <<
	"	<clickable />"	<< 
	"	<position x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\" />"	<<
"		<material type=\"basic\" color=\"0xf6e30d\">" <<
	"	</material>"	<<
	"</primitive>"	;
	


	
	return t.str() ;

}

std::string SkyGrid::trackAsterism() const
{

	TQuery my (  connection  );

	std::stringstream query;
	query << 
		"select costellazione "  <<
		"from asterism  " << 
		"group by costellazione "  
		;
		
	try {
			
		my.Open( query.str()  );
		
	}
  catch (const std::runtime_error& e) {
    std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]"; 
    return "";
  }
  catch (const std::exception& e) {
    // intercetta anche altri errori standard
    std::cerr << "Eccezione generica: " << e.what() << "\n";
    return "";
  }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return "";
      }
	
	std::stringstream t ;
	while ( !my.Eof() ) {
	
		std::string costellazione = my.FieldByName("costellazione");
		bool hidden = true;
		t <<	asterism ( costellazione, hidden ); 
		
		my.Next();
		
	}
	
	return t.str();
	
}

std::string SkyGrid::drawBounds ( const std::string & constellation, bool hidden ) const
{
	

	Sky sky;
	double R = sky.radius * 0.5 ;
	
	TQuery my ( connection );
	std::stringstream query;
	query << "select RAJ, DE from const_bounds " <<
			"where costellazione=\"" <<
			constellation << "\" " <<
			"and tipo<>\"I\" " <<
			"order by id " ;
			
    try {
      my.Open( query.str()  );
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Errore runtime: " << e.what() << "\n";
        return "";
    }
    catch (const std::exception& e) {
        // intercetta anche altri errori standard
        std::cerr << "Eccezione generica: " << e.what() << "\n";
        return "";
    }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return "";
      }
	
	if ( my.isEmpty() )
		return "";
	
	std::stringstream t ;
	t << "<primitive type=\"line\" name=\"" << constellation << "\">" ;

	
	std::string is_hidden = hidden ? "<hidden />" : "<show />" ;
	std::string first_point;
	while ( !my.Eof() ) {
		
		double ascensione_retta = atof( my.FieldByName("RAJ").c_str() ); 
		double declinazione = atof( my.FieldByName("DE").c_str() ); 
		
		Radiant a = ascensione_retta * Angle::pi / 180;
		Radiant d = declinazione * Angle::pi / 180;

/*					
		Radiant a = Grade( ascensione_retta  );
		Radiant d = Grade( declinazione  );
*/		
		double z = R * sin( d );
		double r = R * cos( d );

		double x = r * cos( a );
		double y = r * sin( a );

		if ( first_point == "" ) {

			std::stringstream ss ;
			ss << "<vertice>" << x << " " << y << " " << z << "</vertice>" ;
			first_point = ss.str();
			
		}
		
		t << "<vertice>" << x << " " << y << " " << z << "</vertice>" ;

		my.Next();
	
	}
	
	// chiude sul punto iniziale
	t << first_point;
					
	t << is_hidden <<
				"<material type=\"line_basic\" size=\"1\" " <<
				"color=\"0xf6e30d\" />" <<
				"</primitive>" <<
				std::endl;

	return t.str() ;

}

std::string SkyGrid::constellationCodeFromName ( std::string name ) const
{
	
	TQuery my ( connection );
	TQuery bounds( connection );

	std::stringstream query;
	query << "select codice " <<
			"from costellazioni where nome=\"" << name << "\"" ;
      
			
	my.Open( query.str()  );
	if ( !my.Eof() ) {
		return my.FieldByName("codice"); 
	}
	else {
		return "";
	}
}

std::string SkyGrid::whichConstellation_old(double AR, double Dec) const
{
    TQuery my(connection);
    TQuery bounds(connection);

    try {
      my.Open("SELECT codice, nome FROM costellazioni");
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Errore runtime: " << e.what() << "\n";
        return "";
    }
    catch (const std::exception& e) {
        // intercetta anche altri errori standard
        std::cerr << "Eccezione generica: " << e.what() << "\n";
        return "";
    }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return "";
      }
    
    std::pair<double,double> point = { AR, Dec };

    while (!my.Eof()) {
        std::string codice = my.FieldByName("codice");
        std::string nome   = my.FieldByName("nome");

        std::stringstream query;
        query << "SELECT RAJ, DE FROM const_bounds "
              << "WHERE costellazione=\"" << codice << "\" "
              << "AND tipo<>\"I\" "
              << "ORDER BY id";
        bounds.Open(query.str());

        std::vector<std::pair<double,double>> polygon;
        while (!bounds.Eof()) {
            double ra  = atof(bounds.FieldByName("RAJ").c_str());
            double dec = atof(bounds.FieldByName("DE").c_str());
            polygon.emplace_back(ra, dec);
            bounds.Next();
        }

        // Normalizza per RA=0
        auto testPoint = point; // copia
        normalizeRA(polygon, testPoint);

        if (isInBounds(testPoint, polygon)) {
            return nome;
        }

        my.Next();
    }

    return "";
}


std::string SkyGrid::whichConstellation(double AR, double Dec) const
{
	TQuery my(connection);
	TQuery bounds(connection);

	std::stringstream query;
	query << "select codice, nome from costellazioni";
			
    try {
        my.Open(query.str());
    } catch (const std::exception& e) {
        std::cerr << "Eccezione generica: " << e.what() << "\n";
        return "";
    } catch (...) { return ""; }
	
	// per ogni costellazione
	while (!my.Eof()) {
		
		std::string codice = my.FieldByName("codice"); 
		std::string nome = my.FieldByName("nome"); 

		std::stringstream b_query;
		b_query << "select RAJ, DE from const_bounds "
                << "where costellazione=\"" << codice << "\" "
                << "and tipo<>\"I\" "
                << "order by id";
			
		std::vector<Point> c_polig;
        
        try {
		    bounds.Open(b_query.str());
        } catch (...) {
            my.Next();
            continue; // Se fallisce una costellazione, passiamo alla prossima
        }
		
		bool ar_correction = false;
		double ar_prev = 0;
		bool first_time = true;
        
		while (!bounds.Eof()) {
			double ar = atof(bounds.FieldByName("RAJ").c_str()); 
			double dec = atof(bounds.FieldByName("DE").c_str());
			
			if (!first_time) {
				if (fabs(ar - ar_prev) > 180) {
					ar_correction = true;
				}
			} else {
				first_time = false;
            }
			
			ar_prev = ar;
			c_polig.push_back(Point(ar, dec)); 
			bounds.Next();
		}

        // Usiamo una copia locale di AR per questa singola verifica
        double test_AR = AR;

		if (ar_correction) {
			// corregge il poligono
			for (auto& p : c_polig) {
				if (p.x > 180) p.x -= 360;
			}
            
            // CORREZIONE CRITICA: correggiamo anche il punto di test!
            if (test_AR > 180) test_AR -= 360;
		}
		
        // Passiamo test_AR invece di AR
		auto intersections = YisInBounds(test_AR, Dec, c_polig);

		if (intersections % 2 != 0) /* is odd */
			return nome;

		my.Next();
	}

	return "";
}

short int SkyGrid::YisInBounds ( double x, double y, const std::vector< Point > & c_polig ) const
{

	// x=AR 0-360
	// y=DEC -90-90
	size_t intersections = 0;
	for ( auto it = c_polig.begin(); it != c_polig.end(); it++ ) {
		
		Point p1 = *it;
		// next point
		Point p2;
		if ( it + 1 != c_polig.end() )
			p2 = *(it + 1);
		else
			p2 = *(c_polig.begin());	// if array ended than take first point
		
		if ( y > 70 ) {
			if ( std::fabs(p2.x-p1.x) > 180 ) {
				if ( p1.x < 180 ) {
					p1.x=(p1.x+360);
	//				if ( x < 180 ) x = x+360 ;
				}
				else if ( p2.x < 180 ) {
					p2.x=(p2.x+360);
	//				if ( x < 180 ) x =x+360 ;
				}
			}
		}

		// if x is out bound not intersect
		if ( ( x < p1.x && x < p2.x ) ||
				( x > p1.x && x > p2.x ) )
					continue;
					
		// where intersect at point y ?
		double y_int = p1.y + ( p2.y - p1.y ) * ( x - p1.x) / ( p2.x - p1.x );
		
		// if x_int is higher than x do intersect
		if ( y < y_int ) intersections++;
	
	}

	return intersections;
	
}

std::string SkyGrid::whichZodiac ( double AR ) const 
{
	
	short n = (12 * AR) / 360 ;
	std::string sigla = zodiaco[n];

	TQuery my ( connection );
	std::stringstream query;
	query << "select codice, nome " <<
			"from costellazioni " <<
			"where codice=\"" << sigla << "\"";
			
    try {
      my.Open( query.str()  );
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Errore runtime: " << e.what() << "\n";
        return "";
    }
    catch (const std::exception& e) {
        // intercetta anche altri errori standard
        std::cerr << "Eccezione generica: " << e.what() << "\n";
        return "";
    }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
          return "";
      }
	
	std::string nome ; 
	// per ogni costellazione
	if ( !my.Eof() ) {
		nome = my.FieldByName("nome"); 
	}
	
	return nome;
	
}

bool SkyGrid::isInBounds_old ( double x, double y, const std::vector< Point > & c_polig ) const
{

	size_t intersections = 0;
	for ( auto it = c_polig.begin(); it != c_polig.end(); it++ ) {
		
		Point p1 = *it;
		// next point
		Point p2;
		if ( it + 1 != c_polig.end() )
			p2 = *(it + 1);
		else
			// if array ended than take first point
			p2 = *(c_polig.begin());
			
		// if x is out bound not intersect
		if ( ( x < p1.x && x < p2.x ) ||
				( x > p1.x && x > p2.x ) )
					continue;
					
		
		// where intersect at point x ?
		double y_int = p1.y + ( p2.y - p1.y ) * ( x - p1.x) / ( p2.x - p1.x );
		
		
		// if y_int is higher than y do intersect
		if ( ( y < y_int  ) )
			intersections++;
	
	}
	
	if (intersections % 2)  /* is odd */
		return true;
	else
		return false;
	
}

std::vector < double > SkyGrid::equ2ecli ( const std::vector < double > & v_e ) const 
{

//	clientData * data = reinterpret_cast < clientData * > ( client->extra ); 
	short simulation = client->simulation;
	Date &d = client->ref_date;

	UT uut = d;
	DT ddt = uut;
	double JD = uut.now( simulation );
	
	Earth earth;
	Radiant epsilon = Grade( earth.obliquity(JD) );
	
	B_MATRIX m ( 3, 3 );
	VECTOR v_cli ( 3 );
	VECTOR v_equ ( 3 );
	
	m(0,0)=1;
	m(0,1)=0;
	m(0,2)=0;

	m(1,0)=0;
	m(1,1)=cos(epsilon);
	m(1,2)=-sin(epsilon);

	m(2,0)=0;
	m(2,1)=sin(epsilon);
	m(2,2)=cos(epsilon);

	v_equ[0] = v_e[0];
	v_equ[1] = v_e[1];
	v_equ[2] = v_e[2];
	

	v_cli = m * v_equ;
	
	std::vector< double > v;
	v.push_back( v_cli(0) );
	v.push_back( v_cli(1) );
	v.push_back( v_cli(2) );

	return v;

}  

std::string SkyGrid::ecliptic( short year ) const
{
	// giorno giuliano inizio anno
	std::stringstream ss1 ;
	ss1 << "01-01" << "-" << year << " 00:00:00" ;
	Date date1 ( ss1.str());
	double JD1 = date1.julianDay();	// giorno giuliano corrispondente alla data date1
	// giorno giliano fine anno
	std::stringstream ss2 ;
	ss2 << "31-01" << "-" << year+1 << " 00:00:00" ;
	Date date2 ( ss2.str());
	double JD2 = date2.julianDay();	// giorno giuliano corrispondente alla data date2
	std::string is_hidden = "" ;	// non mostrare l'eclittica
	std::stringstream t ;
	t << "<primitive type=\"line\" name=\"" << "eclittica" << "\">" << std::endl;

	for ( double jd = JD1; jd <= JD2; jd += 10 ) {

		Sun & sun = client->system->sun ;
		// orbita eclittica (lambda e beta)
		OrbitEcli orbit_sun = sun.orbitLBR( jd );
		
		OrbitEqu   o_sun ( jd );
		// converte le coordinate eclittiche in equatoriali
		o_sun = orbit_sun;
		
		Grade alfa_sun = o_sun.alfa;	// AR
		Grade delta_sun = o_sun.delta; // declinazione
		
		// converte in coordinate rettangolari
		OrbitXYZ o_sun_xyz = o_sun;
		double x = o_sun_xyz.x ;
		double y = o_sun_xyz.y ;
		double z = o_sun_xyz.z ;

		// il vertice della linea
		t << "<vertice>" << x << " " << y << " " << z << "</vertice>"  << std::endl;

	}
	
	t << is_hidden <<
				"<material type=\"line_basic\" size=\"1\" " <<
				"color=\"0xff0000\" />" << std::endl <<
				"</primitive>" <<
				std::endl;
	return t.str();

}

std::string SkyGrid::trackEquatorial ( bool hidden ) const
{
	
	std::string is_hidden = hidden ? "<hidden />" : "<show />" ;

	std::stringstream t ;
	// tutta l'equatoriale ogni 15 gradi
	t << "<primitive type=\"line\" name=\"" << "equatoriale" << "\">" ;
	
	double R = 1;
	double rad = 0;
	while ( rad <= Angle::pi * 2 ) {
		
		double x = R * sin( rad );
		double y = R * cos( rad );
		double z = 0;
		
		
		t << "<vertice>" << x << " " << y << " " << z << "</vertice>" ;

		rad += Angle::pi / 24 ;
	
	}

	t << is_hidden <<
				"<material type=\"line_basic\" size=\"1\" " <<
				"color=\"0x00ff00\" />" <<
				"</primitive>" <<
				std::endl;

	return t.str();
	
}

std::string SkyGrid::trackEcliptic( bool hidden ) const
{
	
	std::string is_hidden = hidden ? "<hidden />" : "<show />" ;

	std::stringstream t ;
	t << "<primitive name=\"" << "eclittica" << "\">" ;
	
	t << is_hidden <<
				"</primitive>" <<
				std::endl;
	

	return t.str();
	
}

std::vector< std::string> SkyGrid::constellationStars ( const std::string & constellation_code ) const 
{

	std::vector<std::string> v;
	
	TQuery my ( connection );

	std::stringstream query;
	query << 
		"select S.nome as nome, soprannome, genitivo, AR, DECL " << 
		"from stars as S " << 
		"join costellazioni as C " <<
		"on codice=costellazione " <<
		"where costellazione=\"" << constellation_code << "\""
		"union (" <<
		"select S.nome as nome, soprannome, genitivo, AR, DECL " << 
		"from messier as S " << 
		"join costellazioni as C " <<
		"on codice=costellazione " <<
		"where costellazione=\"" << constellation_code << "\""
		
		<< ")";
			
	try {
			
		my.Open( query.str()  );
		
	}
  catch (const std::runtime_error& e) {
      std::cerr << "Errore runtime: " << e.what() << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return v;
    
  }
  catch (const std::exception& e) {
      // intercetta anche altri errori standard
      std::cerr << "Eccezione generica: " << e.what() << "\n";
      return v;
  }
  catch (...) {
      // intercetta tutto ciò che non è std::exception
      std::cerr << "Errore sconosciuto!\n: " << " [file: " << __FILE__ << ", line: " << __LINE__ << "]\n"; 
      return v;
  }
	
	// per ogni costellazione
	while ( !my.Eof() ) {
		
		std::string nome = my.FieldByName("nome");
		std::string soprannome = my.FieldByName("soprannome");
		std::string genitivo = my.FieldByName("genitivo");
		std::string AR = my.FieldByName("AR");
		std::string DECL = my.FieldByName("DECL");

		std::stringstream stars;
			
		stars <<
			nome << " " <<
			soprannome << " " <<
			genitivo << "\n" <<
			AR << " " <<
			DECL ;
				
			v.push_back( stars.str() );

		my.Next();
	
	}

	return v;
		
}

