#ifndef _json_
#define _json_

#include <iostream>
#include <vector>

class JSON {

		void * doc;
	
	public:

		JSON();
		JSON( const std::string & str);	// carica json dalla stringa
		~JSON();

		void loadFromFile ( const std::string & nome_file);
		void loadFromString ( const std::string & str );

		std::string value ( const std::string & str );
		std::string operator [] ( const std::string & str );
		
		std::vector< std::string > array( const std::string & str ) ;

};

#endif	// if _json_

