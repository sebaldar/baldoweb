#ifndef clientHttp_hpp
#define clientHttp_hpp

#include <sstream>
#include <iostream>

#include <vector>
#include <map>

#include "timer.hpp"

class Client {

	std::string the_query_string;   // la query tale e quale

	std::map< std::string, std::string> header;
	std::map< std::string, std::string> query_string ;
	std::map< std::string, std::string> cookies ;

	typedef std::pair< std::string, std::string > pairQuery;

	size_t Split (const std::string & input, const std::string & delimiter, std::vector< std::string > & results) const ;

public:

	Client () ;

	Timer connected_timer;
        
	int socket ;
	void * extra ;		// dati ulteriori specifici dell'applicazione

	void addHeader( std::string h );
	std::string getHeaderContent( const std::string & key ) const ;

	std::string getCookieContent( const std::string & key ) const ;

	void setQueryString ( const std::string & ) ;
	std::string getQueryString ( const std::string & key ) const ;
	std::string getQueryString ( ) const ;

	std::string resource ;

};



#endif
