#include <sstream>
#include <iostream>
#include <vector>

#ifndef resource_hpp
	#include "resource.hpp"
#endif

#include "xml.hpp"


Resource::Resource ( const std::string & document_root   ) :
	TQuery (), documentRoot( document_root )
{
	
	// se non è già connesto
	if ( documentRoot[ documentRoot.size() - 1 ] != '/' )
		documentRoot += "/";
	
		// legge l'user
		std::string my_user ;	
		try {
			my_user = utils::file_get_contents ( documentRoot + "user" );
		}
		catch ( const std::string & e ) {
			throw ( e ) ;
		}
	
		XMLDocument dati;
		dati.loadFromFile ( documentRoot + my_user + ".xml" );
	
		NODES nodi = dati.getElementsByTagName ( "database" );

		if ( nodi.size() != 1 ) {
			std::string str = "Il record database non è presente o non è unico";
			throw ( str );
		}

		XMLElement * db = nodi.at( 0 );

		std::string host = db->getAttribute( "host" );
		std::string user = db->getAttribute( "user" );
		std::string database = db->getAttribute( "db" );
		std::string pass = db->getAttribute( "psw" );
	
		
		try {
		
			fConnection = new TConnection ( host, user, pass, database );
	
		}
		catch ( const std::string  & e ) {
		
			std::stringstream err ;
		
			err << "Errore nella connessione  : " << e << std::endl;
		
			throw ( err.str() );
	
		}
		
	mysql = &fConnection->_mysql;

	isConnectDirect = false ;

}

Resource::~Resource ( )
{

	if ( fConnection )
		delete fConnection;
		
	fConnection = nullptr;

}
